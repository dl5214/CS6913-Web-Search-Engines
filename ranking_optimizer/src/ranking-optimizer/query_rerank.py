import socket
import csv
import h5py
import numpy as np
import os
import time

# Configuration
SERVER_HOST = '127.0.0.1'
SERVER_PORT = 9001
TOP_K_DOCS = 300  # Number of top documents to retrieve using BM25; can be adjusted as needed
FINAL_TOP_K = 100  # Final number of top documents after re-ranking

QUERY_BM25 = True  # Set to True to use BM25 API, False to use file data


# Function to send a query to the C++ server and receive the result (top doc IDs)
def query_cpp_server(query, query_mode="1"):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((SERVER_HOST, SERVER_PORT))
            s.sendall(f"{query}|{query_mode}\n".encode('utf-8'))

            result = s.recv(16384).decode('utf-8')  # Adjust buffer size if needed
            result_list = []

            for line in result.strip().splitlines():
                parts = line.split(', ')
                if len(parts) >= 1:
                    doc_id = parts[0].split(': ')[1]
                    result_list.append(doc_id)

            # Limit results to the defined TOP_K_DOCS
            return result_list[:TOP_K_DOCS]
    except Exception as e:
        print(f"Error querying server: {e}")
        return []


# Function to load embeddings from HDF5 file
def load_h5_embeddings(file_path, id_key='id', embedding_key='embedding'):
    print(f"Loading data from {file_path}...")
    with h5py.File(file_path, 'r') as f:
        ids = np.array(f[id_key]).astype(str)
        embeddings = np.array(f[embedding_key]).astype(np.float32)
    print(f"Loaded {len(ids)} embeddings.")
    return ids, embeddings


# Function to retrieve the embedding of a specific query ID
def get_query_embedding(query_id, query_embeddings_dict):
    return query_embeddings_dict.get(query_id)


# Function to retrieve embeddings for a list of document IDs
def get_doc_embeddings(doc_ids, doc_embeddings_dict):
    # Convert IDs to strings and check for missing IDs
    doc_ids = [str(doc_id) for doc_id in doc_ids]  # Ensure IDs are strings
    missing_ids = [doc_id for doc_id in doc_ids if doc_id not in doc_embeddings_dict]

    if missing_ids:
        print(f"Warning: {len(missing_ids)} document IDs not found in the embeddings dictionary.")
        print("Sample missing IDs:", missing_ids[:5])  # Print first few missing IDs for debugging

    # Retrieve embeddings for available IDs
    return np.array([doc_embeddings_dict[doc_id] for doc_id in doc_ids if doc_id in doc_embeddings_dict])


# Function to perform re-ranking based on cosine similarity using numpy
def rerank_by_similarity(query_embedding, doc_ids, doc_embeddings):
    # Calculate cosine similarities (dot products since embeddings are normalized)
    similarities = np.dot(doc_embeddings, query_embedding)

    # Pair each document ID with its similarity score
    doc_similarity_pairs = list(zip(doc_ids, similarities))

    # Sort by similarity in descending order and keep the top results
    doc_similarity_pairs.sort(key=lambda x: x[1], reverse=True)
    return doc_similarity_pairs[:min(FINAL_TOP_K, len(doc_similarity_pairs))]


# Function to save re-ranked results in TREC format
def save_reranked_results(output_path, query_id, reranked_results, run_name="run3"):
    with open(output_path, "a") as f:
        for rank, (doc_id, score) in enumerate(reranked_results, start=1):
            f.write(f"{query_id}\tQ0\t{doc_id}\t{rank}\t{score}\t{run_name}\n")


# Function to load BM25 results from file
def load_bm25_results_from_file(file_path):
    bm25_results = {}
    with open(file_path, 'r', encoding='utf-8') as f:
        reader = csv.reader(f, delimiter='\t')
        for row in reader:
            if len(row) >= 3:
                query_id, doc_id, score = row[0], row[2], row[4]
                if query_id not in bm25_results:
                    bm25_results[query_id] = []
                if len(bm25_results[query_id]) < TOP_K_DOCS:
                    bm25_results[query_id].append(doc_id)
    return bm25_results


# Function to rerank using BM25 results from the API
def rerank_from_bm25_api(query_text, query_id, query_embedding, doc_embeddings_dict):
    top_doc_ids = query_cpp_server(query_text, query_mode="1")
    doc_count = min(len(top_doc_ids), TOP_K_DOCS)
    # print(f"BM25 server returned {len(top_doc_ids)} doc IDs for queryID={query_id}, using top {doc_count}")
    doc_embeddings = get_doc_embeddings(top_doc_ids, doc_embeddings_dict)
    if len(doc_embeddings) == 0:
        print(f"Warning: No embeddings found for top documents of query ID {query_id}")
        return []
    reranked_results = rerank_by_similarity(query_embedding, top_doc_ids, doc_embeddings)
    print(f"Re-ranked top {len(reranked_results)} doc IDs from top {doc_count} BM25 scores for queryID={query_id}")
    return reranked_results


# Function to rerank using BM25 results from a file
def rerank_from_bm25_file(query_id, query_embedding, bm25_results, doc_embeddings_dict):
    top_doc_ids = bm25_results.get(query_id, [])
    doc_count = min(len(top_doc_ids), TOP_K_DOCS)
    # print(f"File-based BM25 returned {len(top_doc_ids)} doc IDs for queryID={query_id}, using top {doc_count}")
    doc_embeddings = get_doc_embeddings(top_doc_ids, doc_embeddings_dict)
    if len(doc_embeddings) == 0:
        print(f"Warning: No embeddings found for top documents of query ID {query_id}")
        return []
    reranked_results = rerank_by_similarity(query_embedding, top_doc_ids, doc_embeddings)
    print(f"Re-ranked top {len(reranked_results)} doc IDs from top {doc_count} BM25 scores for queryID={query_id}")
    return reranked_results


# Main function to process and re-rank queries
def main(query_bm25_flag, query_file_path, query_embedding_file_path, doc_embedding_file_path, output_file, bm25_file=None):
    print(f"Starting processing queries in file: {query_file_path}")

    # Load query embeddings
    query_ids, query_embeddings = load_h5_embeddings(query_embedding_file_path)
    query_embeddings_dict = dict(zip(query_ids, query_embeddings))

    # Load document embeddings
    doc_ids, doc_embeddings = load_h5_embeddings(doc_embedding_file_path)
    doc_embeddings_dict = dict(zip(doc_ids, doc_embeddings))

    # Load BM25 results from file if not querying the BM25 API
    bm25_results = load_bm25_results_from_file(bm25_file) if not QUERY_BM25 and bm25_file else {}

    if os.path.exists(output_file):
        os.remove(output_file)

    query_count = 0

    with open(query_file_path, 'r', encoding='utf-8') as infile:
        reader = csv.reader(infile, delimiter='\t')
        for row in reader:
            if len(row) < 2:
                continue
            query_id, query_text = row[0], row[1]
            query_count += 1
            print(f"Processing query #{query_count} with queryID={query_id} and text '{query_text}'")

            query_embedding = get_query_embedding(query_id, query_embeddings_dict)
            if query_embedding is None:
                print(f"Warning: Embedding for query ID {query_id} not found.")
                continue

            if QUERY_BM25:
                reranked_results = rerank_from_bm25_api(query_text, query_id, query_embedding, doc_embeddings_dict)
            else:
                reranked_results = rerank_from_bm25_file(query_id, query_embedding, bm25_results, doc_embeddings_dict)

            if reranked_results:
                save_reranked_results(output_file, query_id, reranked_results, run_name="run3")

    print(f"Completed processing for file: {query_file_path}. Total queries processed: {query_count}")


if __name__ == "__main__":

    QUERY_BM25 = False

    start_time = time.time()
    output_dir = "../../data/"
    os.makedirs(output_dir, exist_ok=True)

    main(
        query_bm25_flag=QUERY_BM25,
        query_file_path='../../data/query.content.eval.one.tsv',
        query_embedding_file_path='../../data/msmarco_queries_dev_eval_embeddings.h5',
        doc_embedding_file_path='../../data/msmarco_passages_embeddings_subset.h5',
        output_file=os.path.join(output_dir, "system3_results_eval_one.tsv"),
        bm25_file='../../data/system1_results_eval_one_500.tsv'  # Path to system1 output file
    )

    main(
        query_bm25_flag=QUERY_BM25,
        query_file_path='../../data/query.content.eval.two.tsv',
        query_embedding_file_path='../../data/msmarco_queries_dev_eval_embeddings.h5',
        doc_embedding_file_path='../../data/msmarco_passages_embeddings_subset.h5',
        output_file=os.path.join(output_dir, "system3_results_eval_two.tsv"),
        bm25_file='../../data/system1_results_eval_two_500.tsv'  # Path to system1 output file
    )

    main(
        query_bm25_flag=QUERY_BM25,
        query_file_path='../../data/query.content.dev.tsv',
        query_embedding_file_path='../../data/msmarco_queries_dev_eval_embeddings.h5',
        doc_embedding_file_path='../../data/msmarco_passages_embeddings_subset.h5',
        output_file=os.path.join(output_dir, "system3_results_dev.tsv"),
        bm25_file='../../data/system1_results_dev_500.tsv'  # Path to system1 output file
    )

    end_time = time.time()
    print(f"Total execution time: {end_time - start_time:.2f} seconds")