import json
import logging
import torch
import faiss
import pytrec_eval
import numpy as np
from sentence_transformers import SentenceTransformer
from tqdm import tqdm

def load_scifact_corpus(corpus_file):
    """
    Loads the Scifact corpus from a JSONL file.
    Each line is a JSON object containing '_id' and 'text'.
    Returns lists of document IDs and document texts.
    """
    doc_ids = []
    doc_texts = []
    with open(corpus_file, "r", encoding="utf-8") as f:
        for line in f:
            data = json.loads(line)
            doc_id = data["_id"]
            text_str = data.get("text", "")
            doc_ids.append(doc_id)
            doc_texts.append(text_str)
    return doc_ids, doc_texts


def load_scifact_queries(queries_file):
    """
    Loads the Scifact queries from a JSONL file.
    Each line is a JSON object containing '_id' and 'text'.
    Returns lists of query IDs and query texts.
    """
    q_ids = []
    q_texts = []
    with open(queries_file, "r", encoding="utf-8") as f:
        for line in f:
            data = json.loads(line)
            q_ids.append(data["_id"])
            q_texts.append(data.get("text", ""))
    return q_ids, q_texts


def load_qrels(qrels_file):
    """
    Loads the qrels from a TSV file.
    The first line is a header: 'query-id    corpus-id    score'.
    Returns a dictionary: qrels[query_id][doc_id] = relevance_score.
    """
    qrels = {}
    with open(qrels_file, "r", encoding="utf-8") as f:
        # Skip the header line:
        next(f)
        for line in f:
            line = line.strip()
            if not line:
                continue
            qid, did, rel = line.split()
            rel = int(rel)
            if qid not in qrels:
                qrels[qid] = {}
            qrels[qid][did] = rel
    return qrels


def compute_metrics(qrels, results, k_values=None):
    """
    Computes retrieval metrics using pytrec_eval.
    By default, evaluates MAP, NDCG@k, Recall@k, P@k.
    """
    evaluator = pytrec_eval.RelevanceEvaluator(qrels, {"map", "ndcg_cut", "recall", "P"})
    scores = evaluator.evaluate(results)

    if k_values is None:
        # Extract all available k values from the scores
        all_k_values = set()
        for query_scores in scores.values():
            for metric in query_scores.keys():
                if metric.startswith("ndcg_cut_") or metric.startswith("recall_") or metric.startswith("P_"):
                    all_k_values.add(int(metric.split("_")[-1]))
        k_values = sorted(all_k_values)

    ndcg_res = {}
    recall_res = {}
    prec_res = {}

    # Compute mean average precision (MAP)
    map_vals = [scores[qid]["map"] for qid in scores]
    map_res = np.mean(map_vals)

    # Compute NDCG@k, Recall@k, and Precision@k
    for k in k_values:
        ndcg_vals = []
        recall_vals = []
        prec_vals = []
        for qid in scores:
            ndcg_key = f"ndcg_cut_{k}"
            recall_key = f"recall_{k}"
            prec_key = f"P_{k}"
            if ndcg_key in scores[qid]:
                ndcg_vals.append(scores[qid][ndcg_key])
            if recall_key in scores[qid]:
                recall_vals.append(scores[qid][recall_key])
            if prec_key in scores[qid]:
                prec_vals.append(scores[qid][prec_key])

        ndcg_res[f"NDCG@{k}"] = np.mean(ndcg_vals) if ndcg_vals else 0.0
        recall_res[f"Recall@{k}"] = np.mean(recall_vals) if recall_vals else 0.0
        prec_res[f"P@{k}"] = np.mean(prec_vals) if prec_vals else 0.0

    return ndcg_res, map_res, recall_res, prec_res


def main():
    """
    Main function:
    1) Loads Scifact corpus, queries, and qrels
    2) Encodes documents and queries with SentenceTransformer (E5 model)
    3) Builds a FAISS HNSW index
    4) Retrieves top-k documents for each query
    5) Evaluates the results with pytrec_eval
    """
    # File paths
    corpus_file = "datasets/scifact/corpus.jsonl"
    queries_file = "datasets/scifact/queries.jsonl"
    qrels_file = "datasets/scifact/qrels/test.tsv"

    # Load corpus and queries
    doc_ids, doc_texts = load_scifact_corpus(corpus_file)
    query_ids, query_texts = load_scifact_queries(queries_file)
    qrels = load_qrels(qrels_file)

    print(f"Total documents to encode: {len(doc_texts)}")
    print(f"Total queries to encode: {len(query_texts)}")

    # Filter query IDs to only include those present in qrels
    filtered_query_ids = [qid for qid in query_ids if qid in qrels]
    filtered_query_texts = [query_texts[query_ids.index(qid)] for qid in filtered_query_ids]

    print(f"Filtered queries for evaluation: {len(filtered_query_ids)}")

    # Choose device (GPU if available)
    device = "cuda" if torch.cuda.is_available() else "cpu"

    # Initialize the E5 model via SentenceTransformer
    # Note: E5 models expect a prefix like "query: " or "passage: "
    model = SentenceTransformer("intfloat/e5-base-v2", device=device)

    # Encode corpus with the "passage: " prefix
    corpus_emb = model.encode(
        ["passage: " + p for p in doc_texts],
        batch_size=64,
        show_progress_bar=True,
        convert_to_numpy=True,
        normalize_embeddings=True
    )
    # Save passage embeddings to a text file
    with open("./data/passage_embeddings.txt", "w", encoding="utf-8") as f:
        f.write("=== Passage Embeddings ===\n")
        for i in range(len(doc_ids)):  # Save all passages
            f.write(f"ID: {doc_ids[i]}\n")
            f.write(f"Text: {doc_texts[i]}\n")
            f.write(f"Embedding: {corpus_emb[i].tolist()}\n")
            f.write("\n")

    print("Passage embeddings saved to './data/passage_embeddings.txt'.")

    # Determine embedding dimension
    dimension = corpus_emb.shape[1]
    print(f"Shape of corpus_emb: {corpus_emb.shape}")

    # Configure HNSW parameters
    M = 15  # Maximum number of connections per node
    efConstruction = 300  # Candidate list size during index construction
    # Configure search parameter
    efSearch = 1000


    logging.info("Building FAISS HNSW index...")
    index_hnsw = faiss.IndexHNSWFlat(dimension, M)  # HNSW index
    index_hnsw.hnsw.efConstruction = efConstruction
    index_hnsw.hnsw.efSearch = efSearch

    # Add corpus embeddings to the index
    index_hnsw.add(corpus_emb)

    # If GPU is available, transfer the index to GPU
    if torch.cuda.is_available():
        logging.info("Transferring HNSW index to GPU...")
        faiss_res = faiss.StandardGpuResources()
        index_hnsw = faiss.index_cpu_to_gpu(faiss_res, 0, index_hnsw)

    # Encode queries with the "query: " prefix
    query_emb = model.encode(
        ["query: " + q for q in filtered_query_texts],
        batch_size=64,
        show_progress_bar=True,
        convert_to_numpy=True,
        normalize_embeddings=True
    )

    # Save query embeddings to a text file
    with open("./data/query_embeddings.txt", "w", encoding="utf-8") as f:
        f.write("=== Query Embeddings ===\n")
        for i in range(len(filtered_query_ids)):  # Save all queries
            f.write(f"ID: {filtered_query_ids[i]}\n")
            f.write(f"Text: {filtered_query_texts[i]}\n")
            f.write(f"Embedding: {query_emb[i].tolist()}\n")
            f.write("\n")

    print("Query embeddings saved to './data/query_embeddings.txt'.")

    # Retrieve top_k documents
    top_k = 100
    logging.info("Searching query embeddings in FAISS index...")
    D, I = index_hnsw.search(query_emb, top_k)

    # Build results dict for pytrec_eval
    results = {}
    for q_idx, qid in enumerate(filtered_query_ids):
        results[qid] = {}
        for rank in range(top_k):
            doc_idx = I[q_idx, rank]
            score = 1/float(D[q_idx, rank])
            doc_id = doc_ids[doc_idx]
            results[qid][doc_id] = score

    # Evaluate metrics
    k_values = [5, 10, 15, 20, 30, 100]
    ndcg, mAP, recall, precision = compute_metrics(qrels, results, k_values)

    # Print metrics
    print("=== FAISS-GPU + E5 Bi-Encoder (SentenceTransformer) ===")
    print("NDCG@k:", ndcg)
    print("MAP:", mAP)
    print("Recall@k:", recall)
    print("Precision@k:", precision)


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    main()