import faiss
import numpy as np
import h5py
import time
import os


def load_h5_embeddings(file_path, id_key='id', embedding_key='embedding'):
    """
    Load IDs and embeddings from an HDF5 file.

    Parameters:
    - file_path: Path to the HDF5 file.
    - id_key: Dataset name for the IDs inside the HDF5 file.
    - embedding_key: Dataset name for the embeddings inside the HDF5 file.

    Returns:
    - embeddings_dict: Dictionary with query_id as keys and embeddings as values.
    """
    print(f"Loading data from {file_path}...")
    with h5py.File(file_path, 'r') as f:
        ids = np.array(f[id_key]).astype(str)
        embeddings = np.array(f[embedding_key]).astype(np.float32)
    embeddings_dict = {q_id: emb for q_id, emb in zip(ids, embeddings)}
    print(f"Loaded {len(embeddings_dict)} embeddings.")
    return embeddings_dict


def load_query_ids(tsv_file_path):
    """
    Load query IDs from a TSV file in the order they appear.

    Parameters:
    - tsv_file_path: Path to the TSV file.

    Returns:
    - query_ids: List of query IDs (as strings), preserving file order.
    """
    print(f"Loading query IDs from {tsv_file_path}...")
    query_ids = []
    seen_ids = set()
    with open(tsv_file_path, 'r') as file:
        for line in file:
            columns = line.strip().split('\t')
            query_id = columns[0]
            if query_id not in seen_ids:
                query_ids.append(query_id)
                seen_ids.add(query_id)
    print(f"Loaded {len(query_ids)} unique query IDs in original order.")
    return query_ids


def save_retrieved_results(output_path, query_id, retrieved_passages, distances, run_name="run1"):
    """
    Save retrieved passages for a query to a TSV file in the TREC format.

    Parameters:
    - output_path: Path to the output results file.
    - query_id: The ID of the query.
    - retrieved_passages: List of retrieved passage IDs.
    - distances: List of distances corresponding to each retrieved passage.
    - run_name: Name for this retrieval run.
    """
    with open(output_path, "a") as f:
        for rank, (passage_id, distance) in enumerate(zip(retrieved_passages, distances), start=1):
            score = -distance  # Use negative distance as score
            f.write(f"{query_id}\tQ0\t{passage_id}\t{rank}\t{score}\t{run_name}\n")


def main(query_file_path, embedding_file_path, output_file, k=100, ef_search=120):
    # Load HNSW index
    index = faiss.read_index('../../data/hnsw_index.faiss')
    print("Index loaded from disk.")

    # Set query parameters
    index.hnsw.efSearch = ef_search

    # Load embeddings into a dictionary
    embeddings_dict = load_h5_embeddings(embedding_file_path)

    # Load query IDs from the qrels file, maintaining file order
    query_ids = load_query_ids(query_file_path)

    # Check if all query IDs in qrels file are in embeddings
    missing_query_ids = [q_id for q_id in query_ids if q_id not in embeddings_dict]
    if missing_query_ids:
        print(f"Warning: {len(missing_query_ids)} query IDs in {query_file_path} not found in embeddings.")
        print(f"Missing query IDs: {missing_query_ids}")

    # Perform queries and save results
    print(f"Saving results for queries from {query_file_path} to {output_file}")
    for query_id in query_ids:
        if query_id in embeddings_dict:
            query_embedding = embeddings_dict[query_id].reshape(1, -1)  # Reshape to match search input
            distances, indices = index.search(query_embedding, k)

            # Save the top-k nearest passages in the required format
            retrieved_passages = indices[0]
            save_retrieved_results(output_file, query_id, retrieved_passages, distances[0], run_name="run1")
        else:
            print(f"Warning: Embedding for query ID {query_id} not found.")


if __name__ == "__main__":
    start_time = time.time()  # Record start time

    # Output directory
    output_dir = "../../data/"
    os.makedirs(output_dir, exist_ok=True)

    # Perform queries and save results separately for each file
    main(
        '../../data/qrels.eval.one.tsv',
        '../../data/msmarco_queries_dev_eval_embeddings.h5',
        os.path.join(output_dir, "retrieved_results_eval_one.tsv"),
    )
    main(
        '../../data/qrels.eval.two.tsv',
        '../../data/msmarco_queries_dev_eval_embeddings.h5',
        os.path.join(output_dir, "retrieved_results_eval_two.tsv"),
    )
    main(
        '../../data/qrels.dev.tsv',
        '../../data/msmarco_queries_dev_eval_embeddings.h5',
        os.path.join(output_dir, "retrieved_results_dev.tsv"),
    )

    end_time = time.time()  # Record end time
    elapsed_time = end_time - start_time  # Calculate elapsed time
    print(f"Total execution time: {elapsed_time:.2f} seconds")