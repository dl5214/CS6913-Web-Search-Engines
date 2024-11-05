import faiss
import numpy as np
import h5py
import time


def load_h5_embeddings(file_path, id_key='id', embedding_key='embedding'):
    """
    Load IDs and embeddings from an HDF5 file.

    Parameters:
    - file_path: Path to the HDF5 file.
    - id_key: Dataset name for the IDs inside the HDF5 file.
    - embedding_key: Dataset name for the embeddings inside the HDF5 file.

    Returns:
    - ids: Numpy array of IDs (as strings).
    - embeddings: Numpy array of embeddings (as float32).
    """
    print(f"Loading data from {file_path}...")
    with h5py.File(file_path, 'r') as f:
        ids = np.array(f[id_key]).astype(str)
        embeddings = np.array(f[embedding_key]).astype(np.float32)

    print(f"Loaded {len(ids)} embeddings.")
    return ids, embeddings


def main(m=6, ef_construction=120):
    # Load passage embeddings
    passages_file_path = '../../data/msmarco_passages_embeddings_subset.h5'
    # passage_ids, passages_embeddings = load_h5_embeddings(passages_file_path)
    _, passages_embeddings = load_h5_embeddings(passages_file_path)

    print(f"Constructing HNSW index with m = {m} and ef_construction = {ef_construction}...")

    # Create HNSW index
    embedding_dim = passages_embeddings.shape[1]
    index = faiss.IndexHNSWFlat(embedding_dim, m)
    index.hnsw.efConstruction = ef_construction

    # Add passages embeddings to the index
    index.add(passages_embeddings)
    print("Passages embeddings added to HNSW index.")

    # Save index and IDs
    faiss.write_index(index, '../../data/hnsw_index.faiss')
    # np.save('../../data/passage_ids.npy', passage_ids)
    print("Index and IDs saved to disk.")


if __name__ == "__main__":
    start_time = time.time()  # Record start time
    main()
    end_time = time.time()  # Record end time
    elapsed_time = end_time - start_time  # Calculate elapsed time
    print(f"Total execution time: {elapsed_time:.2f} seconds")