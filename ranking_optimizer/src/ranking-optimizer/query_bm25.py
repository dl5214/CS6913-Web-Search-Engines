import socket
import csv
import time


# Define C++ server connection details
SERVER_HOST = '127.0.0.1'
SERVER_PORT = 9001


# Function to send a query to the C++ server and receive the result
def query_cpp_server(query, query_mode="1"):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((SERVER_HOST, SERVER_PORT))
            s.sendall(f"{query}|{query_mode}\n".encode('utf-8'))
            result = s.recv(8192).decode('utf-8')  # Adjust buffer size if needed
            return result
    except Exception as e:
        print(f"Error querying server: {e}")
        return None


# Function to process and format the server response
# Function to process and format the server response
def format_results(query_id, raw_result):
    result_list = []
    rank = 1

    # Process each line in the result
    for line in raw_result.strip().splitlines():
        # Assuming the format of each line is "DocId: <doc_id>, Score: <score>, Content: "
        parts = line.split(', ')
        if len(parts) == 3:
            doc_id = parts[0].split(': ')[1]  # Get DocId from "DocId: <doc_id>"
            score = parts[1].split(': ')[1]   # Get Score from "Score: <score>"
            result_list.append((doc_id, score))

    # Reverse the list to have the highest scores at the top
    result_list = result_list[::-1]

    # Format the output with rank
    formatted_results = []
    for doc_id, score in result_list:
        formatted_results.append(f"{query_id}\tQ0\t{doc_id}\t{rank}\t{score}\trun1")
        rank += 1

    return formatted_results


# Main function to process the TSV file and save the output
def main(input_file, output_file):
    print(f"Processing queries in {input_file}...")
    with open(input_file, 'r', encoding='utf-8') as infile, open(output_file, 'w', encoding='utf-8') as outfile:
        reader = csv.reader(infile, delimiter='\t')
        query_count = 0

        for row in reader:
            query_count += 1
            if len(row) < 2:
                print(f"Skipping row {query_count} due to missing data")
                continue
            query_id, query_text = row[0], row[1]
            print(f"Processing query {query_count}: queryID={query_id}, queryText='{query_text}'")

            # Send query to server
            raw_result = query_cpp_server(query_text, "1")  # Disjunctive mode
            if raw_result is None:
                print(f"Failed to retrieve results for queryID={query_id}")
                continue

            # Format and write results to output file
            formatted_results = format_results(query_id, raw_result)
            for result in formatted_results:
                outfile.write(result + '\n')

            print(f"Query {query_count} completed and results written for queryID={query_id}\n")

    print("All queries processed.")


# Run the main function with the input and output file paths
if __name__ == '__main__':

    start_time = time.time()

    input_file = '../../data/query.content.eval.one.tsv'
    output_file = '../../data/system1_results_eval_one.tsv'
    main(input_file, output_file)

    input_file = '../../data/query.content.eval.two.tsv'
    output_file = '../../data/system1_results_eval_two.tsv'
    main(input_file, output_file)

    input_file = '../../data/query.content.dev.tsv'
    output_file = '../../data/system1_results_dev.tsv'
    main(input_file, output_file)

    end_time = time.time()
    elapsed_time = end_time - start_time
    print(f"Time taken to run the main function: {elapsed_time:.2f} seconds")