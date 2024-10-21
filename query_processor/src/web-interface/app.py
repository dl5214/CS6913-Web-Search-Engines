from flask import Flask, render_template, request
import socket

app = Flask(__name__)

# Define C++ server connection details
SERVER_HOST = '127.0.0.1'  # Use the correct IP address of your C++ server
SERVER_PORT = 9001        # Port number where your C++ server is running


# Function to send query to C++ server and receive the result
def query_cpp_server(query):
    try:
        # Create a socket and connect to the C++ server
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((SERVER_HOST, SERVER_PORT))
            s.sendall(f"{query}\n".encode('utf-8'))  # Send query to C++ server

            # Receive response from C++ server
            result = s.recv(1024).decode('utf-8')  # Adjust buffer size if needed
            # return result
            # Reverse the result lines before returning them
            result_lines = result.strip().splitlines()
            reversed_result = '\n'.join(result_lines[::-1])
            return reversed_result
    except Exception as e:
        return f"Error: {str(e)}"


# Route for the search page
@app.route('/')
def index():
    return render_template('search.html')  # Renders an HTML page with a search box


# Route to handle form submission and display query results
@app.route('/search', methods=['POST'])
def search():
    query = request.form.get('query')  # Get query from form submission
    query_mode = request.form.get('query_mode')  # Get query mode (CONJUNCTIVE or DISJUNCTIVE)

    if query and query_mode:
        # Send query and mode to C++ server and get the result
        result = query_cpp_server(f"{query}|{query_mode}")  # Pass both query and mode
        return render_template('result.html', query=query, result=result)
    else:
        return render_template('search.html', error="Please enter a query and select a mode")


if __name__ == '__main__':
    app.run(debug=True, port=5001)