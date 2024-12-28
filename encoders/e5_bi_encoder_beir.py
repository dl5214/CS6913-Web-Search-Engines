import logging
import torch
from transformers import AutoTokenizer, AutoModel
from beir.datasets.data_loader import GenericDataLoader
from beir.retrieval.search.dense import DenseRetrievalExactSearch
from beir.retrieval.evaluation import EvaluateRetrieval
from torch.cuda.amp import autocast
from tqdm import tqdm
import numpy as np
import torch.nn.functional as F


class E5DenseRetriever:
    """
    A custom dense retriever that uses the E5 model to encode passages and queries.
    It is critical to ensure that the embeddings returned maintain the same ordering
    as the input data. Otherwise, BEIR may map incorrect embeddings to doc IDs.
    """

    def __init__(self, model_name, device="cuda"):
        """
        Initialize the E5 retriever with a given Hugging Face model name and device.
        """
        self.tokenizer = AutoTokenizer.from_pretrained(model_name)
        self.model = AutoModel.from_pretrained(model_name).to(device)
        self.device = device

    # def encode(self, texts, batch_size=32, normalize_embeddings=True, show_progress_bar=False, **kwargs):
    #     """
    #     General method to encode both queries and corpus using the E5 model.
    #
    #     :param texts: A list of raw text strings to encode.
    #     :param batch_size: How many texts to process at once (important for GPU memory usage).
    #     :param normalize_embeddings: Whether to L2-normalize the CLS token embeddings.
    #     :param show_progress_bar: Whether to show a tqdm progress bar during encoding.
    #     :param kwargs: Additional parameters passed to the tokenizer/model if needed.
    #     :return: A single torch.Tensor of shape (len(texts), embedding_dim).
    #     """
    #     embeddings = []
    #
    #     for start_index in tqdm(range(0, len(texts), batch_size),
    #                             desc="Encoding", disable=not show_progress_bar):
    #         batch_texts = texts[start_index : start_index + batch_size]
    #         inputs = self.tokenizer(batch_texts, padding=True, truncation=True, return_tensors="pt").to(self.device)
    #
    #         with torch.no_grad():
    #             # E5 model outputs: (batch_size, seq_len, hidden_dim)
    #             # We grab the CLS token at index [:, 0, :]
    #             batch_embeddings = self.model(**inputs).last_hidden_state[:, 0, :]
    #
    #             if normalize_embeddings:
    #                 batch_embeddings = F.normalize(batch_embeddings, p=2, dim=1)
    #
    #             embeddings.append(batch_embeddings.cpu())
    #
    #     # Concatenate all batch embeddings
    #     all_embeddings = torch.cat(embeddings, dim=0)
    #     return all_embeddings
    def encode(self, texts, batch_size=32, normalize_embeddings=True, show_progress_bar=False, **kwargs):
        """
        General method to encode both queries and corpus using the E5 model.

        :param texts: A list of raw text strings to encode.
        :param batch_size: How many texts to process at once (important for GPU memory usage).
        :param normalize_embeddings: Whether to L2-normalize the mean-pooled embeddings.
        :param show_progress_bar: Whether to show a tqdm progress bar during encoding.
        :param kwargs: Additional parameters passed to the tokenizer/model if needed.
        :return: A single torch.Tensor of shape (len(texts), embedding_dim).
        """
        embeddings = []

        for start_index in tqdm(range(0, len(texts), batch_size),
                                desc="Encoding", disable=not show_progress_bar):
            batch_texts = texts[start_index: start_index + batch_size]
            inputs = self.tokenizer(batch_texts, padding=True, truncation=True, return_tensors="pt").to(self.device)

            with torch.no_grad():
                # Get model outputs (batch_size, seq_len, hidden_dim)
                outputs = self.model(**inputs).last_hidden_state  # Shape: (batch_size, seq_len, hidden_dim)

                # Apply mean pooling over the sequence length dimension
                # Exclude padding tokens from the mean pooling
                attention_mask = inputs["attention_mask"]  # Shape: (batch_size, seq_len)
                input_mask_expanded = attention_mask.unsqueeze(-1).expand(
                    outputs.size()).float()  # Expand to hidden_dim

                sum_embeddings = torch.sum(outputs * input_mask_expanded, dim=1)  # Sum over seq_len
                sum_mask = torch.clamp(input_mask_expanded.sum(dim=1), min=1e-9)  # Avoid division by zero
                batch_embeddings = sum_embeddings / sum_mask  # Mean pooling

                if normalize_embeddings:
                    batch_embeddings = F.normalize(batch_embeddings, p=2, dim=1)

                embeddings.append(batch_embeddings.cpu())

        # Concatenate all batch embeddings
        all_embeddings = torch.cat(embeddings, dim=0)
        return all_embeddings

    def encode_queries(self, queries, batch_size=32, normalize_embeddings=True, show_progress_bar=False, **kwargs):
        """
        Encode queries with the required E5 prefix, e.g. "query: ...".

        :param queries: A list of query strings.
        """
        # Add "query: " prefix to each query
        prefixed_queries = ["query: " + q for q in queries]
        return self.encode(prefixed_queries, batch_size=batch_size,
                           normalize_embeddings=normalize_embeddings,
                           show_progress_bar=show_progress_bar, **kwargs)

    def encode_corpus(self, corpus_texts, batch_size=32, normalize_embeddings=True, show_progress_bar=False, **kwargs):
        """
        Encode a corpus of documents. BEIR typically passes a list of passage texts.

        :param corpus_texts: A list of passage strings.
        """
        # Ensure all texts are strings
        corpus_texts = [str(text) if text is not None else "" for text in corpus_texts]

        # Optional: Log the types of the first few texts to verify
        if show_progress_bar:
            print(f"Types of first 5 corpus texts: {[type(text) for text in corpus_texts[:5]]}")

        # Encode the corpus texts in the same order
        embeddings = self.encode(
            corpus_texts,
            batch_size=batch_size,
            normalize_embeddings=normalize_embeddings,
            show_progress_bar=show_progress_bar,
            **kwargs
        )
        return embeddings


def main():
    """
    1) Load the dataset (e.g., SciFact).
    2) Use E5 as a Bi-Encoder model to perform dense retrieval.
    3) Evaluate the performance via BEIR.
    """

    ##############################
    # Step 1: Load the dataset
    ##############################
    dataset_path = "datasets/scifact"  # Adjust this to your dataset folder
    corpus, queries, qrels = GenericDataLoader(data_folder=dataset_path).load(split="test")

    print(f"Corpus type: {type(corpus)}, Queries type: {type(queries)}, qrels type: {type(qrels)}")
    print(f"Number of documents in the corpus: {len(corpus)}")
    print(f"Number of queries: {len(queries)}")
    print(f"Number of qrels (query relevance judgments): {len(qrels)}")
    print()

    # Sample Corpus (first 3 documents)
    print("=== Sample Corpus (first 3 documents) ===")
    print(f"Total Corpus Count: {len(corpus)}\n")  # Print total corpus count
    for i, (doc_id, doc_data) in enumerate(list(corpus.items())[:3]):
        print(f"Document ID: {doc_id}")
        print(f"Document Text (first 100 chars): {doc_data['text'][:100]}...")
        if "title" in doc_data and doc_data["title"]:
            print(f"Document Title: {doc_data['title']}")
        print()
        if i >= 2:
            break

    # Sample Queries (first 5 queries)
    print("=== Sample Queries (first 5 queries) ===")
    print(f"Total Query Count: {len(queries)}\n")  # Print total query count
    for i, (query_id, query_text) in enumerate(list(queries.items())[:5]):
        print(f"Query ID: {query_id}")
        print(f"Query Text: {query_text}")
        print()
        if i >= 4:
            break

    # Sample qrels (first 5 query relevance judgments)
    print("=== Sample Qrels (first 5 relevance judgments) ===")
    print(f"Total Qrels Count: {len(qrels)}\n")  # Print total qrels count
    for i, (query_id, relevance_docs) in enumerate(list(qrels.items())[:5]):
        print(f"Query ID: {query_id}")
        print(f"Relevance Judgments: {relevance_docs}")  # Print up to 5 relevant documents per query
        print()
        if i >= 4:
            break

    ##############################
    # Step 2: Setup E5 Retriever
    ##############################
    # You can use a variety of E5 models, e.g., intfloat/e5-small, e5-base, e5-large, etc.
    model_name = "intfloat/e5-base-v2"
    device = "cuda" if torch.cuda.is_available() else "cpu"
    e5_retriever = E5DenseRetriever(model_name, device)

    ##############################
    # Step 3: Perform retrieval using E5
    ##############################
    # Create the dense retrieval object.
    # Note that we explicitly set score_function="dot" to reflect typical E5 training.
    bi_encoder_retriever = DenseRetrievalExactSearch(
        model=e5_retriever,
        batch_size=128,
        score_function="dot"  # "dot" is typical if E5 embeddings are normalized
    )

    # Wrap retriever with EvaluateRetrieval for metric evaluation
    bi_encoder_evaluator = EvaluateRetrieval(bi_encoder_retriever)

    print("Starting retrieval with E5 Bi-Encoder...")

    # **Fix FutureWarning:** Update autocast usage as per latest PyTorch recommendations
    # Replace 'dtype' with 'device_type' and specify 'dtype' within context manager
    if device == "cuda":
        autocast_context = torch.amp.autocast(device_type="cuda", dtype=torch.float16)
    else:
        autocast_context = torch.amp.autocast(device_type="cpu", dtype=torch.float32)  # Use float32 for CPU

    with autocast_context:
        results = bi_encoder_evaluator.retrieve(corpus, queries)

    # Evaluate the retrieval performance
    k_values = [5, 10, 15, 20, 30, 100]
    bi_ndcg, bi_map, bi_recall, bi_precision = bi_encoder_evaluator.evaluate(qrels, results, k_values)

    print("Retrieval performance with E5:")
    print(f"NDCG@k: {bi_ndcg}")
    print(f"MAP@k: {bi_map}")
    print(f"Recall@k: {bi_recall}")
    print(f"Precision@k: {bi_precision}\n")


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    main()