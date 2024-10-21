//
// Created by Dong Li on 10/20/24.
//
// calculate BM25
#include "QueryProcessor.h"
#include "config.h"
using namespace std;

// Constructor for QueryProcessor class
QueryProcessor::QueryProcessor() {
}

// Destructor for IndexBuilder class
QueryProcessor::~QueryProcessor() {
}

// Calculates the BM25 score for a given term in a specific document.
double QueryProcessor::_getBM25(string term, uint32_t docID, uint32_t freq) {
    double k1 = 1.2;
    double b = 0.75;
    Document doc = pageTable.pageTable[docID]; // Retrieve document information
    // BM25 scaling factor
    double K = k1 * ((1 - b) + b * doc.dataLength / pageTable.avgDataLen);
    // Total number of documents in the corpus
    int N = (int)pageTable.totalDoc;
    // Number of documents containing the term
    double f_t = lexicon.lexiconList[term].docNum;
    // Frequency of the term in the current document
    uint32_t f_dt = freq;
    // BM25 formula
    double score = log((N - f_t + 0.5) / (f_t + 0.5)) * (k1 + 1) * f_dt / (K + f_dt);
    return score;
}


// Reads metadata from the index file using mmap for efficient I/O
void QueryProcessor::_openList(uint32_t offset, uint32_t &metadata_size,
                          vector<uint32_t> &lastdocID_list, vector<uint32_t> &docIDsize_list,
                          vector<uint32_t> &freqSize_list) {
    lastdocID_list.clear();
    docIDsize_list.clear();
    freqSize_list.clear();
    int index_fd = open(lexicon.indexPath.c_str(), O_RDONLY);  // Open index file
    off_t pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);  // Align offset to the page boundary
    size_t length = MAX_META_SIZE;
    struct stat sb;
    if (offset + length > sb.st_size)
        length = sb.st_size - offset;
    // Execute mmap: Memory-map the file for efficient data access
    char *ret = (char *)mmap(NULL, length + offset - pa_offset, PROT_READ, MAP_PRIVATE, index_fd, pa_offset);
    if (ret == MAP_FAILED) {
        close(index_fd);
        perror("mmap");
        return;
    }

    // Read metadata for the current block
    int startIndex = offset - pa_offset;
    memcpy(&metadata_size, ret + startIndex, sizeof(metadata_size));  // Read metadata size
    startIndex += sizeof(metadata_size);
    // Extract last document IDs, document sizes, and frequency sizes for the block
    for (int i = 0; i < metadata_size; i++) {
        uint32_t lastdocID;
        memcpy(&lastdocID, ret + startIndex, sizeof(lastdocID));
        lastdocID_list.push_back(lastdocID);
        startIndex += sizeof(lastdocID);
    }
    for (int i = 0; i < metadata_size; i++) {
        uint32_t docIDsize;
        memcpy(&docIDsize, ret + startIndex, sizeof(docIDsize));
        docIDsize_list.push_back(docIDsize);
        startIndex += sizeof(docIDsize);
    }
    for (int i = 0; i < metadata_size; i++) {
        uint32_t freqSize;
        memcpy(&freqSize, ret + startIndex, sizeof(freqSize));
        freqSize_list.push_back(freqSize);
        startIndex += sizeof(freqSize);
    }
    // Unmap the memory-mapped file and close it
    munmap(ret, length + offset - pa_offset);
    close(index_fd);
}


// Placeholder for frequency retrieval of a term in a specific document
uint32_t QueryProcessor::_getFreq(string term, uint32_t docID) {
    return 0;
}


// Handles Term-At-A-Time (TAAT) query processing, supporting disjunctive (OR) and conjunctive (AND) queries
void QueryProcessor::_queryTAAT(vector<string> word_list, int queryMode) {
    _searchResultList.clear();  // Clear previous search results

    if (queryMode == DISJUNCTIVE) {  // OR query
        vector<double> score_array(pageTable.totalDoc, 0);  // Array to hold BM25 scores
        // Iterate through each term and calculate its score
        for (int i = 0; i < word_list.size(); i++) {
            string term = word_list[i];
            try {
                _decodeBlocks(term, score_array);  // Update score array using the term's contribution
            }
            catch (...) {
                cerr << "exception" << endl;
            }
        }
        // Select the top-k highest scores
        _getTopK(score_array, NUM_TOP_RESULT);
    }
    else if (queryMode == CONJUNCTIVE) {  // AND query
        // find minterm
        string minterm = word_list[0];  // Initialize with the first term
        uint32_t mindocNum = lexicon.lexiconList[minterm].docNum;  // Number of documents containing the term
        // Find the term with the smallest number of documents
        for (int i = 1; i < word_list.size(); i++) {
            string term = word_list[i];
            if (lexicon.lexiconList[term].docNum < mindocNum) {
                minterm = term;
                mindocNum = lexicon.lexiconList[term].docNum;
            }
        }

        // Build the initial score hash based on the smallest term
        map<uint32_t, double> score_hash;
        _decodeBlocks(minterm, score_hash, true);

        // Update score hash with other terms
        for (string word : word_list) {
            if (word == minterm)
                continue;
            try {
                _updateScoreHash(word, score_hash, false);
            }
            catch (...) {
                cerr << "exception" << endl;
            }
        }

        // Retrieve the top-k results
        _getTopK(score_hash, NUM_TOP_RESULT);
    }
    // index_infile.close();
}


// Splits a query string into individual words using delimiters
vector<string> QueryProcessor::_splitQuery(string query) {
    vector<string> word_list;
    string sep = " :;,.\t\v\r\n\f[]{}()<>+-=*&^%$#@!~`\'\"|\\/?·\"：“”";
    string word;

    for (size_t i = 0; i < query.length(); i++) {
        if (sep.find(query[i]) == string::npos) {
            word += query[i];  // If not a separator, add to the current word
        }
        else {
            if (word.length()) {
                word_list.push_back(word);  // Add the word to the list
            }
            word.clear();
        }
    }
    if (word.length()) {
        word_list.push_back(word);  // Add the last word if necessary
    }
    return word_list;
}


// Test query function to perform a sample search and output results
void QueryProcessor::testQuery() {
    string query = "cat dog mouse";
    clock_t query_start = clock();
    vector<string> word_list = _splitQuery(query);  // Split the query into words
    if (!word_list.size()) {
        cout << "unlegal query" << endl;
        return;
    }
    _queryTAAT(word_list, 0);  // Perform the query
    clock_t query_end = clock();
    clock_t query_time = query_end - query_start;
    cout << "search using " << setiosflags(ios::fixed)
              << setprecision(2) << double(query_time) / 1000000 << "s" << endl;

//    // Find and update snippets for each result
//    clock_t snippet_begin = clock();
//    _updateSnippets(word_list,vector<uint32_t>());
//    clock_t snippet_end = clock();
//    clock_t snippet_time = snippet_end - snippet_begin;
//    cout << "find snippets using " << double(snippet_time) / 1000000 << "s" << endl;
//    _searchResultList.print();
}


// Main query loop for interactive querying
void QueryProcessor::queryLoop() {
    cout << "Welcome to my search engine!" << endl;
    cout << "input 'exit' to exit." << endl;
    cout << "you can input a query like 'hello world'." << endl;
    cout << "you can select search type, conjunctive(0) or disjunctive(1) queries." << endl;
    cout << setiosflags(ios::fixed) << setprecision(2);

    // cout<<_DocTable._avg_data_len<<" "<<_DocTable._totalDoc<<endl;

    while (true) {
        cout << "query>>";
        string query;
        getline(cin, query);  // Get user input
        if (query == "exit") {
            break;
        }
        string queryModeStr;
        int queryMode;
        cout << "conjunctive(0) or disjunctive(1)>>";
        getline(std::cin, queryModeStr);   // Get query type

        if (queryModeStr == "0" || queryModeStr == "conjunctive" || queryModeStr == "and") {
            queryMode = CONJUNCTIVE;
        }
        else if (queryModeStr == "1" || queryModeStr == "disjunctive" || queryModeStr == "or") {
            queryMode = DISJUNCTIVE;
        }
        else {
            cout << "cannot recognize query type" << endl;
            continue;
        }

        clock_t query_start = clock();
        vector<string> word_list = _splitQuery(query);  // Split the query

        for(int i=0;i<word_list.size();i++) {
            cout<<lexicon.lexiconList[word_list[i]].docNum<<endl;
        }

        if (!word_list.size()) {
            cout << "unlegal query" << endl;
            continue;
        }
        _queryTAAT(word_list, queryMode);  // Perform the query
        clock_t query_end = clock();
        clock_t query_time = query_end - query_start;
        cout << "search using " << double(query_time) / 1000000 << "s" << endl;

//        // Find and update snippets for each result
//        clock_t snippet_begin = clock();
//        _updateSnippets(word_list, vector<uint32_t>());
//        clock_t snippet_end = clock();
//        clock_t snippet_time = snippet_end - snippet_begin;
//        cout << "find snippets using " << double(snippet_time) / 1000000 << "s" << endl;

        _searchResultList.print();
    }
}


// Decodes posting blocks for a given term and updates the score array
void QueryProcessor::_decodeBlocks(string term, vector<double> &score_array) {
    uint32_t beginp = lexicon.lexiconList[term].beginPos;
    uint32_t endp = lexicon.lexiconList[term].endPos;
    uint32_t block_num = lexicon.lexiconList[term].blockNum;
    for (int i = 0; i < block_num; i++) {
        _decodeBlock(term, beginp, score_array);  // Decode each block
    }
}


// Decodes a single block of postings and updates the score array
void QueryProcessor::_decodeBlock(string term, uint32_t &beginp, vector<double> &score_array) {
    uint32_t metadata_size;
    vector<uint32_t> lastdocID_list, docIDsize_list, freqSize_list;
    // Read the metadata
    _openList(beginp, metadata_size, lastdocID_list, docIDsize_list, freqSize_list);

    // Decode docID and frequency chunks
    vector<uint32_t> docID64, freq64;
    uint32_t metabyte = 4 + 3 * (metadata_size)*4;
    uint32_t docIDp = beginp + metabyte;
    uint32_t freqp = docIDp + docIDsize_list[0];

    for (int i = 0; i < metadata_size; i++) {
        docID64 = _decodeChunk(docIDp, docIDp + docIDsize_list[i]);  // Decode docID list
        freq64 = _decodeChunk(freqp, freqp + freqSize_list[i]);  // Decode frequency list
        if (i != metadata_size - 1) {
            docIDp += docIDsize_list[i] + freqSize_list[i];
            freqp += freqSize_list[i] + docIDsize_list[i + 1];
        }

        // Increment document IDs and update scores using BM25
        uint32_t prev_docID = 0;
        for (int i = 0; i < docID64.size(); i++) {
            prev_docID += docID64[i];
            score_array[prev_docID] += _getBM25(term, prev_docID, freq64[i]);
        }
    }

    // Update the start position for the next block
    beginp += _getMetaSize(metadata_size, lastdocID_list, docIDsize_list, freqSize_list);
}


// Decodes a chunk of varbyte-encoded data and returns a list of integers
vector<uint32_t> QueryProcessor::_decodeChunk(uint32_t offset, uint32_t endp) {
    int index_fd = open(lexicon.indexPath.c_str(), O_RDONLY);  // Open index file
    off_t pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);  // Align offset to page boundary
    size_t length = endp - offset;  // Calculate length
    struct stat sb;
    if (offset + length > sb.st_size)
        length = sb.st_size - offset;
    // Execute mmap: Memory-map the file for efficient I/O
    char *ret = (char *)mmap(NULL, length + offset - pa_offset, PROT_READ, MAP_PRIVATE, index_fd, pa_offset);
    if (ret == MAP_FAILED) {
        close(index_fd);
        perror("mmap");
    }

    vector<uint32_t> decodedIntegers;
    uint32_t currentInt = 0;
    int shift = 0;
    uint8_t byte;
    int startIndex = offset - pa_offset;

    // Decode varbyte-encoded data
    for (uint32_t i = offset; i < endp; i++) {
        memcpy(&byte, ret + startIndex, sizeof(byte));
        startIndex += sizeof(byte);
        currentInt |= (byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) { // Check the high bit
            decodedIntegers.push_back(currentInt);
            currentInt = 0;
            shift = 0;
        }
        else {
            shift += 7;
        }
    }

    // Unmap the memory and close the file
    munmap(ret, length + offset - pa_offset);
    close(index_fd);

    return decodedIntegers;
}


// Finds the top-K scores from the score array
void QueryProcessor::_getTopK(vector<double> &score_array, int k) {
    struct QueueDouble {
        double value;
        uint32_t index;
        QueueDouble(double v, uint32_t i) : value(v), index(i) {}

        // Override the comparison operator for the priority queue
        bool operator<(const QueueDouble &other) const {
            return -value < -other.value;
        }
    };

    priority_queue<QueueDouble> maxQueue;

    // Maintain a priority queue to track the top-k elements
    for (int i = 0; i < score_array.size(); i++) {
        maxQueue.push(QueueDouble(score_array[i], i));
        if (maxQueue.size() > k) {
            maxQueue.pop();
        }
    }

    // // Store the top-k results into the result list
    while (!maxQueue.empty()){

        uint32_t docID = maxQueue.top().index;
        double score = maxQueue.top().value;
//        _searchResultList.insert(docID, pageTable.pageTable[docID].url, score, "");
        _searchResultList.insert(docID, score);
        maxQueue.pop();
    }
}


void QueryProcessor::_decodeBlocks(string term, map<uint32_t, double> &score_hash, bool is_init) {
    uint32_t beginp = lexicon.lexiconList[term].beginPos;
    uint32_t endp = lexicon.lexiconList[term].endPos;
    uint32_t block_num = lexicon.lexiconList[term].blockNum;

    for (int i = 0; i < block_num; i++) {
        _decodeBlock(term, beginp, score_hash, is_init);
    }
}


// Decodes a single block of postings for a term and updates the score array
void QueryProcessor::_decodeBlock(string term, uint32_t &beginp,
                             map<uint32_t, double> &score_hash, bool is_init)
{
    uint32_t metadata_size;  // Number of documents in the block
    vector<uint32_t> lastdocID_list, docIDsize_list, freqSize_list;
    // Retrieve metadata about the block (last document IDs, sizes of docID and frequency data)
    _openList(beginp, metadata_size, lastdocID_list, docIDsize_list, freqSize_list);

    // Prepare to decode docID and frequency chunks
    vector<uint32_t> docID64, freq64;
    uint32_t metabyte = 4 + 3 * (metadata_size)*4;  // Offset for metadata in the block
    uint32_t docIDp = beginp + metabyte;  // Starting position for docID chunk
    uint32_t freqp = docIDp + docIDsize_list[0];  // Starting position for frequency chunk

    // Loop through each document in the block and decode its docID and frequency
    for (int i = 0; i < metadata_size; i++) {
        // Decode the chunk of docIDs and frequencies
        docID64 = _decodeChunk(docIDp, docIDp + docIDsize_list[i]);  // Decode docIDs for the block
        freq64 = _decodeChunk(freqp, freqp + freqSize_list[i]);  // Decode frequencies for the block
        // If not the last document, update pointers for the next docID and frequency chunks
        if (i != metadata_size - 1) {
            docIDp += docIDsize_list[i] + freqSize_list[i];  // Move docID pointer to the next block
            freqp += freqSize_list[i] + docIDsize_list[i + 1];  // Move frequency pointer
        }

        // init score
        if (is_init) {
            // For each document in this chunk, accumulate the BM25 score using the decoded docID and frequency
            uint32_t prev_docID = 0;  // Used to reconstruct the original docIDs from deltas
            for (int i = 0; i < docID64.size(); i++) {
                prev_docID += docID64[i];  // Accumulate docID from delta encoding
                // Add BM25 score to the array
                score_hash[prev_docID] = _getBM25(term, prev_docID, freq64[i]);
            }
        }
    }

    // Update the starting position for the next block
    beginp += _getMetaSize(metadata_size, lastdocID_list, docIDsize_list, freqSize_list);
}


// Finds the top-K scores from the score array using a priority queue
void QueryProcessor::_getTopK(map<uint32_t, double> &score_hash, int k) {
    struct QueueDouble {
        double value;
        uint32_t index;
        QueueDouble(double v, uint32_t i) : value(v), index(i) {}

        // Override the comparison operator for the priority queue
        bool operator<(const QueueDouble &other) const {
            return -value < -other.value;
        }
    };

    priority_queue<QueueDouble> maxQueue;  // Priority queue to store the top-k scores

    // Iterate through the score array and maintain the top-k elements in the priority queue
    for (const auto& [docID, score] : score_hash) {
        maxQueue.push(QueueDouble(score, docID));  // Add the current score and its docID
        if (maxQueue.size() > k) {
            maxQueue.pop();  // Remove the lowest score when we exceed k elements
        }
    }
//    for (map<uint32_t, double>::iterator it = score_hash.begin(); it != score_hash.end(); ++it) {
//
//        maxQueue.push(QueueDouble(it->second, it->first));  // Add the current score and its docID
//        if (maxQueue.size() > k) {
//            maxQueue.pop();  // Remove the lowest score when we exceed k elements
//        }
//    }

    // Extract the top-k elements from the priority queue and insert them into the result list
    while (!maxQueue.empty()) {
        uint32_t docId = maxQueue.top().index;
        double score = maxQueue.top().value;

//        _searchResultList.insert(docID, pageTable.pageTable[docID].url, score, "");  // Add the result
        _searchResultList.insert(docId, score);  // Add the result
        maxQueue.pop();  // Remove the top element
    }
}


// Calculate the total size of the metadata (docID and frequency lists) for a block
uint32_t QueryProcessor::_getMetaSize(uint32_t metadata_size, vector<uint32_t> &lastdocID_list,
                                  vector<uint32_t> &docIDsize_list, vector<uint32_t> &freqSize_list) {
    uint32_t size = 0;
    // Add size of the metadata (number of blocks and the sizes of docID/frequency lists)
    size += sizeof(metadata_size) + 3 * sizeof(uint32_t) * metadata_size;
    // Add the size of each docID and frequency block
    for (int i = 0; i < metadata_size; i++){
        size += docIDsize_list[i] + freqSize_list[i];
    }
    return size;  // Return the total size of the block
}


// Updates the score hash for the given term, checking if the document exists in the current block
void QueryProcessor::_updateScoreHash(string term, map<uint32_t, double> &score_hash, bool is_init) {
    uint32_t beginp = lexicon.lexiconList[term].beginPos;  // Starting position of the postings list for the term
    uint32_t endp = lexicon.lexiconList[term].endPos;  // End position of the postings list for the term
    uint32_t block_num = lexicon.lexiconList[term].blockNum;  // Number of blocks for the term

    uint32_t metadata_size;
    vector<uint32_t> lastdocID_list, docIDsize_list, freqSize_list;
    // Read metadata for the block
    _openList(beginp, metadata_size, lastdocID_list, docIDsize_list, freqSize_list);
    int index = 0;
    int docIDp = 0;
    vector<uint32_t> docID64, freq64;
    bool needDecode = true;

    // Iterate through the score hash and postings to find matching document IDs
    for (auto it = score_hash.begin(); beginp < endp && it != score_hash.end();) {
        uint32_t nextDocId = (next(it) == score_hash.end()) ? MAX_DOC_ID : next(it)->first;
        uint32_t docID = it->first;

        // Move to the next block if the current block does not contain the document
        if (lastdocID_list.back() < docID) {
            beginp += _getMetaSize(metadata_size, lastdocID_list, docIDsize_list, freqSize_list);

            if (beginp < endp) {
                _openList(beginp, metadata_size, lastdocID_list, docIDsize_list, freqSize_list);
                index = 0;
                needDecode = true;
            } else {
                break;  // Exit the loop if no more blocks remain
            }
            continue;
        }

        // Find the correct block for the current document ID
        if (auto prevIndex = index; prevIndex != index) {
            needDecode = true;
        }

        // Decode the block if necessary
        if (needDecode) {
            uint32_t metabyte = 4 + 3 * metadata_size * sizeof(uint32_t);
            uint32_t docIDp = beginp + metabyte;
            uint32_t freqp = docIDp + docIDsize_list[0];

            for (int i = 0; i < index; ++i) {
                docIDp += docIDsize_list[i] + freqSize_list[i];  // Advance to the next chunk
                freqp += freqSize_list[i] + docIDsize_list[i + 1];
            }

            // Decode document IDs and frequencies
            docID64 = _decodeChunk(docIDp, docIDp + docIDsize_list[index]);
            freq64 = _decodeChunk(freqp, freqp + freqSize_list[index]);

            // Cumulative document IDs decoding
            uint32_t prevdocID = 0;
            for (auto &id: docID64) {
                prevdocID += id;
                id = prevdocID;
            }

            docIDp = 0;
            needDecode = false;
        }

        // Check for the document in the decoded block and update the score hash
        while (docIDp < docID64.size()) {
            if (docID64[docIDp] == docID) {
                score_hash[docID] += _getBM25(term, docID, freq64[docIDp]);  // Update score
                break;
            }
            // Stop early if the current document ID exceeds the next document
            if (docID64[docIDp] >= nextDocId) {
                break;
            }
            ++docIDp;
        }

        ++it;  // Move to the next document ID
    }
}

//// Updates the snippet results for each search result by finding snippets in the document content
//void QueryProcessor::_updateSnippets(vector<string> word_list, vector<uint32_t> word_docNum_list) {
//    for (int i = 0; i < _searchResultList._resultList.size(); i++) {
//        // Find snippets for the current document and update the result list
//        string s = findSnippets(_searchResultList._resultList[i].docID, word_list, word_docNum_list);
//        _searchResultList._resultList[i].snippets = s;
//    }
//}

//// Finds snippets for a given document by reading the document content and extracting relevant portions
//string QueryProcessor::findSnippets(uint32_t docID, vector<string> word_list,
//                                     vector<uint32_t> word_docNum_list) {
//    // Open the snippets source file
//    ifstream infile;
//    infile.open(SNIPPETS_SOURCE_PATH);
//    infile.seekg(_PageTable._DocTable[docID].gzp);  // Seek to the document's position in the file
//    string snippets;
//    // Calculate the size of the document and read its content
//    size_t size = docID + 1 < _PageTable._DocTable.size() ? _PageTable._DocTable[docID + 1].gzp - _PageTable._DocTable[docID].gzp : INDEX_CHUNK;
//    char *buffer = (char *)malloc(size + 1);
//    infile.read(buffer, size + 1);
//    buffer[size] = '\0';
//    string docContent;
//    docContent = buffer;
//    // Extract snippets from the document content based on the query terms
//    snippets = _searchResultList.extractSnippets(docContent, "<TEXT>\n", "</TEXT>", word_list, word_docNum_list);
//
//    free(buffer);  // Free the allocated memory
//    return snippets;
//}
//
//// Test function to load fake results and display the associated snippets
//void QueryProcessor::TestSnippets(string fakefile_path) {
//    vector<string> word_list;
//    vector<uint32_t> word_docNum_list;
//    // Read fake results from the file and generate snippets
//    _searchResultList.ReadFake(fakefile_path, word_list, word_docNum_list);
//    updateSnippets(word_list, word_docNum_list);  // Update the snippets for each result
//    _searchResultList.Print();  // Print the results
//}