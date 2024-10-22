#include "IndexBuilder.h"
#include "QueryProcessor.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

// Define a global instance of IndexBuilder and QueryProcessor
IndexBuilder index_builder;
QueryProcessor query_processor;


// Function to handle client queries and send responses
void handleRequest(tcp::socket &socket) {
    try {
        // Read the query request sent by the client
        boost::asio::streambuf requestBuffer;
        boost::asio::read_until(socket, requestBuffer, "\n");

        // Convert the request to a string
        istream requestStream(&requestBuffer);
        string query;
        getline(requestStream, query);

        // Split query and mode
        size_t separator = query.find('|');
        string actualQuery = query.substr(0, separator);
        int queryMode = stoi(query.substr(separator + 1));

        // Output the received query and mode
        cout << "Received query: " << actualQuery << " with mode: " << queryMode << endl;

        // Process the query and get the result
        string result = query_processor.processQuery(actualQuery, queryMode);  // Pass the query mode

        // Send the query result back to the client
        boost::asio::write(socket, boost::asio::buffer(result + "\n"));
    } catch (exception &e) {
        cerr << "Exception while handling request: " << e.what() << endl;
    }
}


// Function to start the server on a specified port
void startServer(short port) {
    boost::asio::io_context io_context;

    // Listen for incoming connections
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));

    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);  // Accept client connection

        cout << "Client connected." << endl;
        handleRequest(socket);  // Process client query
    }
}


void parseIndex() {
    cout << "Building postings and intermediate inverted index. Timing started... " << endl;
    clock_t index_start = clock();
    index_builder.readData(DATA_SOURCE_PATH);
    clock_t index_end = clock();
    double index_time = double(index_end - index_start) / 1000000;
    cout << "Building postings and intermediate inverted index DONE." << endl;
    cout << "Time elapsed: " << fixed << setprecision(2) << index_time << " Seconds" << endl;
}


void mergeIndex() {
    cout << "Merging inverted index. Timing started..." << endl;
    clock_t merge_start = clock();
    index_builder.mergeIndex();
    clock_t merge_end = clock();
    double merge_time = double(merge_end - merge_start) / 1000000;
    cout << "Merging inverted index DONE." << endl;
    cout << "Time elapsed: " << fixed << setprecision(2) << merge_time << " Seconds" << endl;
}


void buildLexicon() {
    cout << "Building Lexicon and Final Compressed Index. Timing started..." << endl;
    clock_t lexicon_build_start = clock();
    index_builder.lexicon.build(MERGED_INDEX_PATH);  // Pass the final merged index file
    index_builder.writeLexicon();
    clock_t lexicon_build_end = clock();
    double lexicon_build_time = double(lexicon_build_end - lexicon_build_start) / 1000000;
    cout << "Building Lexicon and Final Index DONE." << endl;
    cout << "Time elapsed: " << fixed << setprecision(2) << lexicon_build_time << " Seconds" << endl;
}


// Function to load PageTable and Lexicon into memory
void load() {
    cout << "Loading PageTable and Lexicon Into Main Memory. Timing Started..." << endl;
    clock_t load_start = clock();
    query_processor.pageTable.load();
    query_processor.lexicon.load();
    clock_t load_end = clock();
    double load_time = double(load_end - load_start) / 1000000;
    cout << "Loading PageTable and Lexicon Done." << endl;
    cout << "Time elapsed: "<< fixed << setprecision(2) << load_time <<" Seconds"<< endl;
}


// Function to run the query loop
void queryLoop() {
    query_processor.queryLoop();
}


// Main function still exists for standalone running
int main() {

    if (PARSE_INDEX_FLAG) {
        parseIndex();
    }

    if (MERGE_FLAG) {
        mergeIndex();
    }

    if (LEXICON_FLAG) {
        buildLexicon();
    }

    if (LOAD_FLAG) {
        load();  // Use the defined load function
    }

//    if (QUERY_FLAG) {
//        queryLoop();  // Use the defined query loop
//    }

    // Check if we should start the server
    if (FRONTEND_FLAG) {
        cout << "Frontend mode: please use web server on port 9001..." << endl;
        startServer(9001);  // Run the server on port 12345
    }

    // If server is not started, run query loop
    if (!FRONTEND_FLAG && QUERY_FLAG) {
        cout << "Console mode: please use query loop in console..." << endl;
        query_processor.queryLoop();  // Run the query loop for standard input
    }

    return 0;
}
