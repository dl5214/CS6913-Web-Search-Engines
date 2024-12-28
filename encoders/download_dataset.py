from beir import util
#
# url = "https://public.ukp.informatik.tu-darmstadt.de/thakur/BEIR/datasets/msmarco.zip"
# data_path = util.download_and_unzip(url, "datasets/")
# print("Dataset downloaded at:", data_path)


# from beir.datasets.data_loader import GenericDataLoader
# from beir.util.download import download_and_unzip

# Name of dataset, e.g. "scifact"
# dataset_name = "msmarco"
dataset_name = "scifact"
# dataset_name = "scidocs"
# dataset_name = "nq"

# Download and unzip dataset
url = f"https://public.ukp.informatik.tu-darmstadt.de/thakur/BEIR/datasets/{dataset_name}.zip"
data_path = util.download_and_unzip(url, "datasets/")
print("Dataset downloaded at:", data_path)