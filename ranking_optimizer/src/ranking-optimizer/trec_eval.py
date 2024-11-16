import pandas as pd
import pytrec_eval
import json


# Function to read qrel file and convert to pytrec_eval format
def read_qrel(file_path):
    df = pd.read_csv(file_path, sep='\t', header=None, names=['query_id', 'placeholder', 'doc_id', 'relevance'])
    qrel = {}
    for _, row in df.iterrows():
        query_id = str(row['query_id'])  # Ensure query_id is a string
        doc_id = str(row['doc_id'])  # Ensure doc_id is a string
        relevance = int(row['relevance'])

        if query_id not in qrel:
            qrel[query_id] = {}
        qrel[query_id][doc_id] = relevance

    return qrel


# Function to read run file and convert to pytrec_eval format
def read_run(file_path):
    df = pd.read_csv(file_path, sep='\t', header=None,
                     names=['query_id', 'placeholder', 'doc_id', 'rank', 'score', 'run_name'])
    run = {}
    for _, row in df.iterrows():
        query_id = str(row['query_id'])  # Ensure query_id is a string
        doc_id = str(row['doc_id'])  # Ensure doc_id is a string
        score = float(row['score'])

        if query_id not in run:
            run[query_id] = {}
        run[query_id][doc_id] = score

    return run


# Function to calculate and print results
def calculate_results(qrel, run, metrics, output_per_query=True):
    evaluator = pytrec_eval.RelevanceEvaluator(qrel, metrics)
    results = evaluator.evaluate(run)

    if output_per_query:
        # Output results per query with 4 decimal places
        formatted_results = {
            query_id: {metric: "{:.4f}".format(value) for metric, value in query_results.items()}
            for query_id, query_results in results.items()
        }
        print(json.dumps(formatted_results, indent=2))
    else:
        # Output overall average scores with 4 decimal places
        avg_results = {}
        query_count = len(results)
        for metric in metrics:
            avg_results[metric] = "{:.4f}".format(
                sum(results[query_id][metric] for query_id in results) / query_count
            )
        print(json.dumps(avg_results, indent=2))


def main(qrel_file_path, run_file_path, output_per_query=False, metrics=None):
    if metrics is None:
        metrics = {'map', 'ndcg'}  # Default metrics

    # Debug information
    print(f"Processing: QREL File = {qrel_file_path}, RUN File = {run_file_path}")

    # Read files
    qrel = read_qrel(qrel_file_path)
    run = read_run(run_file_path)

    # Calculate and print results
    calculate_results(qrel, run, metrics, output_per_query)


if __name__ == "__main__":
    # Define test cases with system and file information
    test_cases = [
        ("System 1", '../../data/qrels.eval.one.tsv', '../../data/system1_results_eval_one_100.tsv', {'map', 'ndcg_cut_10', 'ndcg_cut_100'}),
        ("System 1", '../../data/qrels.eval.two.tsv', '../../data/system1_results_eval_two_100.tsv', {'map', 'ndcg_cut_10', 'ndcg_cut_100'}),
        ("System 1", '../../data/qrels.dev.tsv', '../../data/system1_results_dev_100.tsv', {'map', 'recip_rank', 'recall_100'}),
        ("System 2", '../../data/qrels.eval.one.tsv', '../../data/system2_results_eval_one.tsv', {'map', 'ndcg_cut_10', 'ndcg_cut_100'}),
        ("System 2", '../../data/qrels.eval.two.tsv', '../../data/system2_results_eval_two.tsv', {'map', 'ndcg_cut_10', 'ndcg_cut_100'}),
        ("System 2", '../../data/qrels.dev.tsv', '../../data/system2_results_dev.tsv', {'map', 'recip_rank', 'recall_100'}),
        ("System 3", '../../data/qrels.eval.one.tsv', '../../data/system3_results_eval_one_300.tsv', {'map', 'ndcg_cut_10', 'ndcg_cut_100'}),
        ("System 3", '../../data/qrels.eval.two.tsv', '../../data/system3_results_eval_two_300.tsv', {'map', 'ndcg_cut_10', 'ndcg_cut_100'}),
        ("System 3", '../../data/qrels.dev.tsv', '../../data/system3_results_dev_300.tsv', {'map', 'recip_rank', 'recall_100'})
    ]

    # Run all test cases
    for system_name, qrel_file_path, run_file_path, metrics in test_cases:
        print(f"\n=== Evaluating {system_name} ===")
        main(qrel_file_path, run_file_path, output_per_query=False, metrics=metrics)