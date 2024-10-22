//
// Created by Dong Li on 10/20/24.
//
// calculate BM25
#include "QueryProcessor.h"
#include "config.h"
using namespace std;


// Constructor for QueryProcessor class
QueryProcessor::QueryProcessor() = default;


// Destructor for IndexBuilder class
QueryProcessor::~QueryProcessor() = default;


// Calculates the BM25 score for a given term in a specific document.
double QueryProcessor::_getBM25(string queryTerm, uint32_t docId, uint32_t freq) {
    double k1 = 1.2;
    double b = 0.75;
    Document doc = pageTable.pageTable[docId]; // Retrieve document information
    // BM25 scaling factor
    double K = k1 * ((1 - b) + b * doc.dataLength / pageTable.avgDataLen);
    // Total number of documents in the corpus
    int N = (int)pageTable.totalDoc;
    // Number of documents containing the term
    double f_t = lexicon.lexiconList[queryTerm].docNum;
    // Frequency of the term in the current document
    uint32_t f_dt = freq;
    // BM25 formula
    double score = log((N - f_t + 0.5) / (f_t + 0.5)) * (k1 + 1) * f_dt / (K + f_dt);
    return score;
}


// Calculate the total size of the metadata (docID and frequency lists) for a block
uint32_t QueryProcessor::_getMetaSize(uint32_t metadataSize, vector<uint32_t> &lastDocIdList,
                                      vector<uint32_t> &docIdSizeList, vector<uint32_t> &freqSizeList) {
    uint32_t size = 0;
    // Add size of the metadata (number of blocks and the sizes of docID/frequency lists)
    size += sizeof(metadataSize) + 3 * sizeof(uint32_t) * metadataSize;
    // Add the size of each docID and frequency block
    for (int i = 0; i < metadataSize; i++){
        size += docIdSizeList[i] + freqSizeList[i];
    }
    return size;  // Return the total size of the block
}


// Splits a query string into individual words using delimiters
vector<string> QueryProcessor::_splitQuery(const string& query) {
    vector<string> queryWordList;
    string sep = " :;,.\t\v\r\n\f[]{}()<>+-=*&^%$#@!~`\'\"|\\/?·\"：“”";
    string word;

    for (char ch : query) {
        if (sep.find(ch) == string::npos) {
            word += ch;  // If not a separator, add the char to the current word
        }
        else {
            if (word.length()) {
                queryWordList.push_back(word);  // Add the word to the list
            }
            word.clear();
        }
    }
    if (word.length()) {
        queryWordList.push_back(word);  // Add the last word if necessary
    }
    return queryWordList;
}


// Reads metadata from the index file using mmap for efficient I/O
void QueryProcessor::_openList(uint32_t offset,
                               uint32_t &metadataSize,
                               vector<uint32_t> &lastDocIdList,
                               vector<uint32_t> &docIdSizeList,
                               vector<uint32_t> &freqSizeList) {
    lastDocIdList.clear();
    docIdSizeList.clear();
    freqSizeList.clear();
    int index_fd = open(lexicon.indexPath.c_str(), O_RDONLY);  // Open index file

    off_t pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);  // Align offset to the page boundary
    size_t length = MAX_META_SIZE;
    struct stat fileStats;
    // Check if the file size is valid before proceeding with mmap
    if (fstat(index_fd, &fileStats) == -1) {
        perror("Error getting file size with fstat");
        close(index_fd);
        return;
    }

    // Adjust the length if it exceeds the file size
    if (offset + length > fileStats.st_size)
        length = fileStats.st_size - offset;
    // Execute mmap: Memory-map the file for efficient data access
    char *mapped_memory = (char *)mmap(NULL, length + offset - pa_offset, PROT_READ, MAP_PRIVATE, index_fd, pa_offset);
    if (mapped_memory == MAP_FAILED) {
        close(index_fd);
        perror("mmap failed");
        return;
    }

    // Read metadata for the current block
    int startIndex = offset - pa_offset;
    memcpy(&metadataSize, mapped_memory + startIndex, sizeof(metadataSize));  // Read metadata size

    startIndex += sizeof(metadataSize);

    // Extract last document IDs, document sizes, and frequency sizes for the block
    for (int i = 0; i < metadataSize; i++) {
        uint32_t lastDocId;
        memcpy(&lastDocId, mapped_memory + startIndex, sizeof(lastDocId));
        lastDocIdList.push_back(lastDocId);
        startIndex += sizeof(lastDocId);
    }
    for (int i = 0; i < metadataSize; i++) {
        uint32_t docIdSize;
        memcpy(&docIdSize, mapped_memory + startIndex, sizeof(docIdSize));
        docIdSizeList.push_back(docIdSize);
        startIndex += sizeof(docIdSize);
    }
    for (int i = 0; i < metadataSize; i++) {
        uint32_t freqSize;
        memcpy(&freqSize, mapped_memory + startIndex, sizeof(freqSize));
        freqSizeList.push_back(freqSize);
        startIndex += sizeof(freqSize);
    }
    // Unmap the memory-mapped file and close it
    munmap(mapped_memory, length + offset - pa_offset);
    close(index_fd);
}


vector<pair<uint32_t, uint32_t>> QueryProcessor::_getPostingsList(string term) {
    vector<pair<uint32_t, uint32_t>> postingsList;
    // Extract term's block data from the lexicon
    uint32_t beginPos = lexicon.lexiconList[term].beginPos;
    uint32_t endPos = lexicon.lexiconList[term].endPos;
    uint32_t blockNum = lexicon.lexiconList[term].blockNum;

    // Loop through the blocks to decode postings
    for (int i = 0; i < blockNum; i++) {
        uint32_t metadataSize;
        vector<uint32_t> lastDocIdList, docIdSizeList, freqSizeList;

        // Open the block and get metadata
        _openList(beginPos, metadataSize, lastDocIdList, docIdSizeList, freqSizeList);

        // For each block, decode the postings list
        for (int j = 0; j < metadataSize; j++) {
            vector<uint32_t> docIDs = _decodeChunkToIntList(beginPos, beginPos + docIdSizeList[j]);
            vector<uint32_t> freqs = _decodeChunkToIntList(beginPos + docIdSizeList[j], beginPos + docIdSizeList[j] + freqSizeList[j]);

            // Combine docIDs and freqs into pairs and add to postings list
            for (int k = 0; k < docIDs.size(); k++) {
                postingsList.emplace_back(docIDs[k], freqs[k]);
            }
        }

        // Move to the next block
        beginPos += _getMetaSize(metadataSize, lastDocIdList, docIdSizeList, freqSizeList);
    }

    return postingsList;
}


// Frequency retrieval of a term in a specific document
uint32_t QueryProcessor::_getFreq(string term, uint32_t docId) {
    // Get the postings list for the term
    vector<pair<uint32_t, uint32_t>> postingsList = _getPostingsList(term);

    // Search for the document ID in the postings list
    for (auto &posting : postingsList) {
        if (posting.first == docId) {
            return posting.second;  // Return the frequency
        }
    }

    // If docID is not found, return 0
    return 0;
}


// Decodes a chunk of varbyte-encoded data and returns a list of integers
// This function is used to decode either DocId or Freq chunks from the index file
vector<uint32_t> QueryProcessor::_decodeChunkToIntList(uint32_t offset, uint32_t endPos) {
    int index_fd = open(lexicon.indexPath.c_str(), O_RDONLY);  // Open the index file in read-only mode
    // page align offset
    off_t pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);  // Align offset to the system's page size
    size_t length = endPos - offset;  // Calculate the length to be read from the file
    struct stat fileStats;

    // Adjust the length if it exceeds the file size
    if (offset + length > fileStats.st_size)
        length = fileStats.st_size - offset;

    // Memory-map the file for efficient I/O to avoid reading large blocks directly into memoryMemory
    char *mapped_memory = (char *)mmap(NULL, length + offset - pa_offset, PROT_READ, MAP_PRIVATE, index_fd, pa_offset);
    if (mapped_memory == MAP_FAILED) {
        close(index_fd);
        perror("Memory mapping failed");
    }

    vector<uint32_t> decodedIntegers;  // To store decoded integers
    uint32_t currentInt = 0;
    int shift = 0;
    uint8_t byte;
    int startIndex = offset - pa_offset;

    // Varbyte decoding logic, reconstructing integers from encoded bytes
    for (uint32_t i = offset; i < endPos; i++) {
        memcpy(&byte, mapped_memory + startIndex, sizeof(byte));  // Copy each byte
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

    // Unmap the memory-mapped file and close the file descriptor
    munmap(mapped_memory, length + offset - pa_offset);
    close(index_fd);

    return decodedIntegers;  // Return the list of decoded integers
}


// Finds the top-K scores from the score array using a priority queue
void QueryProcessor::_getMapTopK(map<uint32_t, double> &docScoreMap, int k) {

    priority_queue<DocScoreEntry> minHeap;  // Priority queue to store the top-k scores

    // Iterate through the score array and maintain the top-k elements in the priority queue
    for (const auto& [docId, score] : docScoreMap) {
        minHeap.emplace(score, docId);  // Add the current score and its docID
        if (minHeap.size() > k) {
            minHeap.pop();  // Remove the lowest score when we exceed k elements
        }
    }

    // Extract the top-k elements from the priority queue and insert them into the result list
    while (!minHeap.empty()) {
        uint32_t docId = minHeap.top().docId;
        double score = minHeap.top().score;

//        _searchResultList.insert(docID, pageTable.pageTable[docID].url, score, "");  // Add the result
        _searchResultList.insert(docId, score);  // Add the result
        minHeap.pop();  // Remove the top element
    }
}


// Finds the top-K scores from the score array
void QueryProcessor::_getListTopK(vector<double> &scoreList, int k) {

    priority_queue<DocScoreEntry> minHeap;

    // Maintain a priority queue to track the top-k elements
    for (int i = 0; i < scoreList.size(); i++) {
        minHeap.emplace(scoreList[i], i);
        if (minHeap.size() > k) {
            minHeap.pop();
        }
    }

    // // Store the top-k results into the result list
    while (!minHeap.empty()){

        uint32_t docId = minHeap.top().docId;
        double score = minHeap.top().score;
//        _searchResultList.insert(docID, pageTable.pageTable[docID].url, score, "");
        _searchResultList.insert(docId, score);
        minHeap.pop();
    }
}


// Decodes a single block of postings for a term and updates the score hash map
void QueryProcessor::_decodeOneBlockToMap(string minTerm, uint32_t &beginPos, map<uint32_t, double> &docScoreMap) {
    uint32_t metadataSize;  // Number of documents in the block
    vector<uint32_t> lastDocIdList, docIdSizeList, freqSizeList;
    // Retrieve metadata (document IDs, sizes, and frequencies for the block)
    _openList(beginPos, metadataSize, lastDocIdList, docIdSizeList, freqSizeList);

    // Prepare to decode docID and frequency chunks
    vector<uint32_t> docId64, freq64;
    uint32_t metaByte = 4 + 3 * (metadataSize) * 4;  // Offset for metadata in the block
    uint32_t docIdPos = beginPos + metaByte;  // Starting position for docID chunk
    uint32_t freqPos = docIdPos + docIdSizeList[0];  // Starting position for frequency chunk

    // Loop through each document in the block and decode its docID and frequency
    for (int i = 0; i < metadataSize; i++) {
        // Decode the chunk of docIDs and frequencies
        docId64 = _decodeChunkToIntList(docIdPos, docIdPos + docIdSizeList[i]);  // Decode docIDs for the block
        freq64 = _decodeChunkToIntList(freqPos, freqPos + freqSizeList[i]);  // Decode frequencies for the block
        // If not the last document, update pointers for the next docID and frequency chunks
        if (i != metadataSize - 1) {
            docIdPos += docIdSizeList[i] + freqSizeList[i];  // Move docID pointer to the next block
            freqPos += freqSizeList[i] + docIdSizeList[i + 1];  // Move frequency pointer
        }

        // init score
        // For each document in this chunk, accumulate the BM25 score using the decoded docID and frequency
        uint32_t originDocId = 0;  // Used to reconstruct the original docIDs from deltas
        for (int j = 0; j < docId64.size(); j++) {
            originDocId += docId64[j];  // Reconstruct original document IDs from deltas
            // Insert score for each document
            docScoreMap[originDocId] = _getBM25(minTerm, originDocId, freq64[j]);
        }
    }

    // Move to the next block
    beginPos += _getMetaSize(metadataSize, lastDocIdList, docIdSizeList, freqSizeList);
}


void QueryProcessor::_decodeBlocksToMap(string minTerm, map<uint32_t, double> &docScoreMap) {
    uint32_t beginPos = lexicon.lexiconList[minTerm].beginPos;
//    uint32_t endPos = lexicon.lexiconList[minTerm].endPos;
    uint32_t blockNum = lexicon.lexiconList[minTerm].blockNum;

    for (int i = 0; i < blockNum; i++) {
        _decodeOneBlockToMap(minTerm, beginPos, docScoreMap);
    }
}


// Updates the score hash for the given term, ensuring only documents common to all terms remain
void QueryProcessor::_updateScoreMap(string term, map<uint32_t, double> &docScoreMap) {
    uint32_t beginPos = lexicon.lexiconList[term].beginPos;  // Starting position of the postings list for the term
    uint32_t endPos = lexicon.lexiconList[term].endPos;      // End position of the postings list for the term

    uint32_t metadataSize;
    vector<uint32_t> lastDocIdList, docIdSizeList, freqSizeList;
    _openList(beginPos, metadataSize, lastDocIdList, docIdSizeList, freqSizeList);  // Load metadata for the current block

    vector<uint32_t> docId64, freq64;
    bool decodeRequired = true;
    int docIdPos = 0;  // Initialize docIdPos here, outside the decode block

    // Iterate through the documents in docScoreMap
    auto it = docScoreMap.begin();
    while (beginPos < endPos && it != docScoreMap.end()) {
        uint32_t nextDocId = (next(it) == docScoreMap.end()) ? MAX_DOC_ID : next(it)->first;
        uint32_t docId = it->first;

        // Skip blocks that do not contain the current document
        if (lastDocIdList.back() < docId) {
            beginPos += _getMetaSize(metadataSize, lastDocIdList, docIdSizeList, freqSizeList);
            if (beginPos < endPos) {
                _openList(beginPos, metadataSize, lastDocIdList, docIdSizeList, freqSizeList);
                decodeRequired = true;
            } else {
                break;  // Exit the loop if no more blocks remain
            }
            continue;
        }

        // Decode the block if necessary
        if (decodeRequired) {
            uint32_t metabyte = 4 + 3 * metadataSize * sizeof(uint32_t);  // Calculate metadata offset
            uint32_t docIdPosStart = beginPos + metabyte;  // Start position of docIDs
            uint32_t freqPosStart = docIdPosStart + docIdSizeList[0];  // Start position of frequencies

            // Decode document IDs and frequencies
            docId64 = _decodeChunkToIntList(docIdPosStart, docIdPosStart + docIdSizeList[0]);
            freq64 = _decodeChunkToIntList(freqPosStart, freqPosStart + freqSizeList[0]);

            // Document IDs delta decoding
            uint32_t originDocId = 0;
            for (auto &id : docId64) {
                originDocId += id;  // Reconstruct the original document ID
                id = originDocId;   // Modify the docId64 element
            }

            docIdPos = 0;  // Reset docIdPos to 0 for the new block
            decodeRequired = false;
        }

        // Check if the document is found in the current term
        bool found = false;
        while (docIdPos < docId64.size()) {
            if (docId64[docIdPos] == docId) {
                // Document found, update its score and continue
                docScoreMap[docId] += _getBM25(term, docId, freq64[docIdPos]);
                found = true;
                break;
            }
            // Stop early if the current document ID exceeds the next document in the map
            if (docId64[docIdPos] >= nextDocId) {
                break;
            }
            ++docIdPos;
        }

        if (!found) {
            // If the document is not found in the current term, remove it from the map
            it = docScoreMap.erase(it);
        } else {
            ++it;  // Move to the next document in the map
        }
    }

    // Remove any remaining documents from docScoreMap that weren't matched
    while (it != docScoreMap.end()) {
        it = docScoreMap.erase(it);
    }
}


// Decodes a single block of postings and updates the score array
void QueryProcessor::_decodeOneBlockToList(string term, uint32_t &beginPos, vector<double> &scoreList) {
    uint32_t metadataSize;
    vector<uint32_t> lastDocIdList, docIdSizeList, freqSizeList;
    // Read the metadata
    _openList(beginPos, metadataSize, lastDocIdList, docIdSizeList, freqSizeList);

    // Decode docID and frequency chunks
    vector<uint32_t> docId64, freq64;
    uint32_t metaByte = 4 + 3 * (metadataSize)*4;
    uint32_t docIdPos = beginPos + metaByte;
    uint32_t freqPos = docIdPos + docIdSizeList[0];

    for (int i = 0; i < metadataSize; i++) {
        docId64 = _decodeChunkToIntList(docIdPos, docIdPos + docIdSizeList[i]);  // Decode docID list
        freq64 = _decodeChunkToIntList(freqPos, freqPos + freqSizeList[i]);  // Decode frequency list
        if (i != metadataSize - 1) {
            docIdPos += docIdSizeList[i] + freqSizeList[i];
            freqPos += freqSizeList[i] + docIdSizeList[i + 1];
        }

        // Increment document IDs and update scores using BM25
        uint32_t originDocId = 0;
        for (int j = 0; j < docId64.size(); j++) {
            originDocId += docId64[j];  // Reconstruct DocID for delta encoding
            scoreList[originDocId] += _getBM25(term, originDocId, freq64[j]);
        }
    }

    // Update the start position for the next block
    beginPos += _getMetaSize(metadataSize, lastDocIdList, docIdSizeList, freqSizeList);
}


// Decodes posting blocks for a given term and updates the score array
void QueryProcessor::_decodeBlocksToList(string term, vector<double> &docScoreList) {
    uint32_t beginPos = lexicon.lexiconList[term].beginPos;
    uint32_t endPos = lexicon.lexiconList[term].endPos;
    uint32_t blockNum = lexicon.lexiconList[term].blockNum;
    for (int i = 0; i < blockNum; i++) {
        _decodeOneBlockToList(term, beginPos, docScoreList);  // Decode each block
    }
}


// Handles Term-At-A-Time (TAAT) query processing, supporting disjunctive (OR) and conjunctive (AND) queries
void QueryProcessor::_queryTAAT(vector<string> wordList, int queryMode) {
    _searchResultList.clear();  // Clear previous search results

    if (queryMode == DISJUNCTIVE) {  // OR query
        vector<double> scoreList(pageTable.totalDoc, 0);  // Array to hold BM25 scores, same size as pageTable
        // Iterate through each term and calculate its score
        for (const auto& queryTerm : wordList) {
            try {
                _decodeBlocksToList(queryTerm, scoreList);  // Update score array using the term's contribution
            }
            catch (...) {
                cerr << "exception" << endl;
            }
        }
        // Select the top-k highest scores
        _getListTopK(scoreList, NUM_TOP_RESULT);
    }
    else if (queryMode == CONJUNCTIVE) {  // AND query
        // find the query term with least docNum
        string minTerm = wordList[0];  // Initialize with the first term
        uint32_t minDocNum = lexicon.lexiconList[minTerm].docNum;  // Number of documents containing the term
        // Find the term with the smallest number of documents
        for (int i = 1; i < wordList.size(); i++) {
            string term = wordList[i];
            if (lexicon.lexiconList[term].docNum < minDocNum) {
                minTerm = term;
                minDocNum = lexicon.lexiconList[term].docNum;
            }
        }
//        cout << "Term with least docNum: " << minTerm << endl;

        // Build the initial score hash map based on the smallest term
        map<uint32_t, double> docScoreMap;
        _decodeBlocksToMap(minTerm, docScoreMap);

        // Update score hash with other terms
        for (const string& queryTerm : wordList) {
            if (queryTerm == minTerm)
                continue;
            try {
                _updateScoreMap(queryTerm, docScoreMap);
            }
            catch (...) {
                cerr << "exception" << endl;
            }
        }

        // Retrieve the top-k results
        _getMapTopK(docScoreMap, NUM_TOP_RESULT);
    }
    // index_infile.close();
}


void QueryProcessor::_decodeOneBlock(string term, uint32_t& beginPos, vector<uint32_t>& docIdList, vector<uint32_t>& freqList) {
    uint32_t metadataSize;
    vector<uint32_t> lastDocIdList, docIdSizeList, freqSizeList;
    // Read the metadata
    _openList(beginPos, metadataSize, lastDocIdList, docIdSizeList, freqSizeList);

    // Decode docID and frequency chunks
    vector<uint32_t> docId64, freq64;
    uint32_t metaByte = 4 + 3 * (metadataSize)*4;
    uint32_t docIdPos = beginPos + metaByte;
    uint32_t freqPos = docIdPos + docIdSizeList[0];

    for (int i = 0; i < metadataSize; i++) {
        docId64 = _decodeChunkToIntList(docIdPos, docIdPos + docIdSizeList[i]);  // Decode docID list
        freq64 = _decodeChunkToIntList(freqPos, freqPos + freqSizeList[i]);  // Decode frequency list
        if (i != metadataSize - 1) {
            docIdPos += docIdSizeList[i] + freqSizeList[i];
            freqPos += freqSizeList[i] + docIdSizeList[i + 1];
        }

        // Increment document IDs and update docIdList and freqList
        uint32_t originDocId = 0;
        for (int j = 0; j < docId64.size(); j++) {
            originDocId += docId64[j];  // Reconstruct DocID for delta encoding

            docIdList.push_back(originDocId);  // Append the decoded docID to docIdList
            freqList.push_back(freq64[j]);  // Append the corresponding frequency to freqList
        }
    }

    // Update the start position for the next block
    beginPos += _getMetaSize(metadataSize, lastDocIdList, docIdSizeList, freqSizeList);
}


void QueryProcessor::_decodeBlocks(string term, vector<uint32_t>& docIdList, vector<uint32_t>& freqList) {
    uint32_t beginPos = lexicon.lexiconList[term].beginPos;
    uint32_t endPos = lexicon.lexiconList[term].endPos;
    uint32_t blockNum = lexicon.lexiconList[term].blockNum;
    for (int i = 0; i < blockNum; i++) {
        _decodeOneBlock(term, beginPos, docIdList, freqList);  // Decode each block
    }
}


uint32_t QueryProcessor::_nextGEQ(const vector<uint32_t>& docIDList, uint32_t currentDocID) {
    // Traverse the docID list and return the first docID >= currentDocID
    for (uint32_t docID : docIDList) {
        if (docID >= currentDocID) {
            return docID;  // Return the next document that meets the condition
        }
    }
    return MAX_DOC_ID;  // If no docID is >= currentDocID, return a sentinel value
}


void QueryProcessor::_queryDAAT(vector<string> wordList, int queryMode) {
    _searchResultList.clear();  // Clear previous results

    vector<uint32_t> beginPositions(wordList.size());  // Store the start positions for each term
    vector<uint32_t> endPositions(wordList.size());
    vector<uint32_t> blockNums(wordList.size());
    vector<vector<uint32_t>> docIDLists(wordList.size()), freqLists(wordList.size());

    // Initialize begin positions from the lexicon
    for (int i = 0; i < wordList.size(); ++i) {
        beginPositions[i] = lexicon.lexiconList[wordList[i]].beginPos;
        endPositions[i] = lexicon.lexiconList[wordList[i]].endPos;
        blockNums[i] = lexicon.lexiconList[wordList[i]].blockNum;
    }

    // Decode the docID and frequency lists for each term in wordList
    for (int i = 0; i < wordList.size(); ++i) {
        _decodeBlocks(wordList[i], docIDLists[i], freqLists[i]);  // Decode each term's blocks
    }

    if (queryMode == CONJUNCTIVE) {
        // Conjunctive (AND) query processing
        map<uint32_t, double> docScoreMap;  // Map to store document scores for conjunctive queries
        vector<uint32_t> currentDocIDs(wordList.size(), 0);

        // Initialize currentDocIDs with the first docID from each list
        for (int i = 0; i < wordList.size(); ++i) {
            currentDocIDs[i] = docIDLists[i][0];
        }

        uint32_t furthestDocID = *max_element(currentDocIDs.begin(), currentDocIDs.end());  // Start with the maximum docID

        while (furthestDocID != MAX_DOC_ID) {

            bool allMatch = true;

            // Iterate through each list and advance the pointers
            for (int i = 0; i < wordList.size(); ++i) {
                // Move each list to the next docID >= furthestDocID
                uint32_t previousDocID = currentDocIDs[i];  // Save the previous docID for debug purposes
                currentDocIDs[i] = _nextGEQ(docIDLists[i], furthestDocID);

                // If any docID is greater than furthestDocID, update furthestDocID
                if (currentDocIDs[i] > furthestDocID) {
                    furthestDocID = currentDocIDs[i];  // Update to the new maximum docID
                    allMatch = false;  // The terms don't match at this docID
                    break;  // Stop the current round of checking since docIDs don't match yet
                }
            }

            if (allMatch) {
                // All terms match at furthestDocID, so calculate and store the score
                double totalScore = 0.0;
                for (int i = 0; i < wordList.size(); ++i) {
                    totalScore += _getBM25(wordList[i], furthestDocID, freqLists[i][0]);  // Calculate BM25 score
                }
                docScoreMap[furthestDocID] = totalScore;  // Store the score in the map

                // Move to the next document for all lists
                furthestDocID = _nextGEQ(docIDLists[0], furthestDocID + 1);  // Get the next docID for the first list
            }
        }

        // Retrieve the top-k results
        _getMapTopK(docScoreMap, NUM_TOP_RESULT);  // Rank and retrieve the top K results
    }

    else if (queryMode == DISJUNCTIVE) {
        // Disjunctive (OR) query processing
        _maxScoreTopK(wordList, docIDLists, freqLists);
    }
}


void QueryProcessor::_maxScoreTopK(const vector<string>& wordList,
                                   const vector<vector<uint32_t>>& docIDLists,
                                   const vector<vector<uint32_t>>& freqLists) {
    priority_queue<DocScoreEntry> topKHeap;  // Priority queue to store top-K scores
    vector<double> maxScores(wordList.size());  // Max score for each term
    double topKThreshold = 0.0;  // Threshold for pruning

    // Step 1: Compute max BM25 score for each term
    for (int i = 0; i < wordList.size(); ++i) {
        uint32_t maxFreq = *max_element(freqLists[i].begin(), freqLists[i].end());
        maxScores[i] = _getBM25(wordList[i], 0, maxFreq);  // Calculate BM25 for max frequency
    }

    // Step 2: Process docIDs by term
    vector<uint32_t> currentDocIDs(wordList.size(), 0);  // Store current docID for each term
    vector<size_t> termIndices(wordList.size(), 0);  // Track the position in each docID list

    // While there are still documents to process
    while (true) {
        // Step 3: Find the smallest docID across all lists
        uint32_t minDocID = MAX_DOC_ID;
        for (int i = 0; i < wordList.size(); ++i) {
            if (termIndices[i] < docIDLists[i].size()) {
                minDocID = min(minDocID, docIDLists[i][termIndices[i]]);
            }
        }
        if (minDocID == MAX_DOC_ID) {
            break;  // No more documents to process
        }

        // Step 4: Calculate score for this docID across all terms
        double totalScore = 0.0;
        for (int i = 0; i < wordList.size(); ++i) {
            if (termIndices[i] < docIDLists[i].size() && docIDLists[i][termIndices[i]] == minDocID) {
                totalScore += _getBM25(wordList[i], minDocID, freqLists[i][termIndices[i]]);
                termIndices[i]++;  // Move to the next docID in this list
            }
        }

        // Step 5: Insert the score into top-K heap if it exceeds the threshold
        if (topKHeap.size() < NUM_TOP_RESULT || totalScore > topKThreshold) {
            topKHeap.emplace(totalScore, minDocID);
            if (topKHeap.size() > NUM_TOP_RESULT) {
                topKHeap.pop();  // Maintain only top-K results
                topKThreshold = topKHeap.top().score;  // Update top-K threshold
            }
        }

        // Step 6: Early termination if remaining max score cannot beat top-K threshold
        double maxRemainingScore = accumulate(maxScores.begin(), maxScores.end(), 0.0);
        if (maxRemainingScore <= topKThreshold) {
            break;  // Stop processing further as max remaining score can't exceed top-K
        }
    }

    // Output top-K results
    _outputTopKResults(topKHeap);
}


void QueryProcessor::_outputTopKResults(priority_queue<DocScoreEntry>& topKHeap) {
    vector<pair<uint32_t, double>> topKResults;

    while (!topKHeap.empty()) {
        topKResults.emplace_back(topKHeap.top().docId, topKHeap.top().score);
        topKHeap.pop();
    }

    // Reverse the results to get them in descending score order
    reverse(topKResults.begin(), topKResults.end());

    // Output the results
    for (const auto& [docId, score] : topKResults) {
//        cout << "DocID: " << docId << " Score: " << score << endl;
        _searchResultList.insert(docId, score);
    }

}


// Main query loop for interactive querying
void QueryProcessor::queryLoop() {
    cout << "Welcome to CS6913 Web Search Engine!" << endl;
    cout << "input 'exit' to exit." << endl;
    cout << "Please input a query, e.g. 'hello world'." << endl;
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
        cout << "conjunctive (0) or disjunctive (1)>>";
        getline(cin, queryModeStr);   // Get query type

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
        vector<string> queryWordList = _splitQuery(query);  // Split the query

        if (!queryWordList.size()) {
            cout << "unlegal query" << endl;
            continue;
        }

        if (DAAT_FLAG) {
            _queryDAAT(queryWordList, queryMode);
        }
        else {
            _queryTAAT(queryWordList, queryMode);
        }

        clock_t query_end = clock();
        clock_t query_time = query_end - query_start;
        cout << "search using " << double(query_time) / 1000000 << "s" << endl;

        _searchResultList.printToConsole();
    }
}


// For front-end Communication
string QueryProcessor::processQuery(const string &query, int queryMode) {
    vector<string> queryWordList = _splitQuery(query);
    ostringstream resultStream;

    if (queryWordList.empty()) {
        resultStream << "Invalid query format." << endl;
        return resultStream.str();
    }
    cout << "queryMode = " << queryMode << endl;

    // Based on queryMode, perform the appropriate query (CONJUNCTIVE or DISJUNCTIVE)
    if ((queryMode == CONJUNCTIVE) || (queryMode == DISJUNCTIVE)) {
        if (DAAT_FLAG) {
            _queryDAAT(queryWordList, queryMode);
        }
        else {
            _queryTAAT(queryWordList, queryMode);
        }
    }
    else {
        resultStream << "Invalid query mode. Please use 0 for CONJUNCTIVE or 1 for DISJUNCTIVE." << endl;
        return resultStream.str();
    }

    // Print results to the result stream
    _searchResultList.printToServer(resultStream);
    return resultStream.str();
}