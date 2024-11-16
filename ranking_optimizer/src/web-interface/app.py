from flask import Flask, render_template, request
import socket

app = Flask(__name__)

# Define C++ server connection details
SERVER_HOST = '127.0.0.1'  # Use the correct IP address of your C++ server
SERVER_PORT = 9001         # Port number where your C++ server is running


# Function to send query to C++ server and receive the result
def query_cpp_server(query):
    try:
        # Create a socket and connect to the C++ server
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((SERVER_HOST, SERVER_PORT))
            s.sendall(f"{query}\n".encode('utf-8'))  # Send query to C++ server

            # Adjust buffer size to receive full response (increased to handle content)
            result = s.recv(65536).decode('utf-8')  # Adjust buffer size if needed
            return result
    except Exception as e:
        return f"Error: {str(e)}"


# Preprocess the result and split it into a list of dictionaries for each search result
def preprocess_result(result):
    result_list = []
    for line in result.strip().splitlines():
        # Split each line into three parts: DocId, Score, and Content
        parts = line.split(', ', 2)
        if len(parts) == 3:
            doc_id = parts[0].split(': ')[1]
            score = parts[1].split(': ')[1]
            # Check if Content: is followed by actual content or is empty
            content_parts = parts[2].split(': ', 1)
            content = content_parts[1] if len(content_parts) > 1 else ''
            result_list.append({
                'doc_id': doc_id,
                'score': score,
                'content': content
            })
        elif len(parts) == 2:
            doc_id = parts[0].split(': ')[1]
            score = parts[1].split(': ')[1]
            result_list.append({
                'doc_id': doc_id,
                'score': score,
                'content': ''
            })
        else:
            result_list.append({
                'doc_id': 'Invalid',
                'score': 'Invalid',
                'content': line
            })
    return result_list


# Route for the search page
@app.route('/')
def index():
    return render_template('search.html')  # Renders an HTML page with a search box


# Route to handle form submission and display query results
@app.route('/search', methods=['POST'])
def search():
    query = request.form.get('query')  # Get query from form submission
    query_mode = request.form.get('query_mode')  # Get query mode (CONJUNCTIVE or DISJUNCTIVE)

    # Translate query_mode to a readable form
    if query_mode == '0':
        mode_text = "Conjunctive (AND)"
    elif query_mode == '1':
        mode_text = "Disjunctive (OR)"
    else:
        mode_text = "Unknown"

    if query and query_mode:
        # Send query and mode to C++ server and get the result
        raw_result = query_cpp_server(f"{query}|{query_mode}")  # Pass both query and mode

        # Preprocess the result to extract DocId, Score, and Content
        processed_result = preprocess_result(raw_result)

        return render_template('result.html', query=query, query_mode=mode_text, results=processed_result)
    else:
        return render_template('search.html', error="Please enter a query and select a mode")


if __name__ == '__main__':
    app.run(debug=True, port=5001)