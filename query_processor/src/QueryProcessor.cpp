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
vector<string> QueryProcessor::_splitQuery(string query) {
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
//    else if (DEBUG_MODE) {
//        cout << "offset = " << offset << ", length = " << length << ", sb.st_size = " << sb.st_size << endl;
//    }

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
//    else if (DEBUG_MODE) {
//        cout << "mmap successful for offset: " << offset << " with length: " << length << endl;
//    }

    // Read metadata for the current block
    int startIndex = offset - pa_offset;
    memcpy(&metadataSize, mapped_memory + startIndex, sizeof(metadataSize));  // Read metadata size
//    if (DEBUG_MODE) {
//        cout << "startIndex = " << startIndex << ", plus size of metadataSize = " << sizeof(metadataSize) << endl;
//    }

    startIndex += sizeof(metadataSize);
//    if (DEBUG_MODE) {
//        cout << "Updated startIndex = " << startIndex << endl;
//    }

    // Extract last document IDs, document sizes, and frequency sizes for the block
    for (int i = 0; i < metadataSize; i++) {
        uint32_t lastDocId;
        memcpy(&lastDocId, mapped_memory + startIndex, sizeof(lastDocId));
        lastDocIdList.push_back(lastDocId);
        startIndex += sizeof(lastDocId);
//        if (DEBUG_MODE) {
//            cout << "Read lastDocId = " << lastDocId << ", Updated startIndex = " << startIndex << endl;
//        }
    }
    for (int i = 0; i < metadataSize; i++) {
        uint32_t docIdSize;
        memcpy(&docIdSize, mapped_memory + startIndex, sizeof(docIdSize));
        docIdSizeList.push_back(docIdSize);
        startIndex += sizeof(docIdSize);
//        if (DEBUG_MODE) {
//            cout << "Read docIdSize = " << docIdSize << ", Updated startIndex = " << startIndex << endl;
//        }
    }
    for (int i = 0; i < metadataSize; i++) {
        uint32_t freqSize;
        memcpy(&freqSize, mapped_memory + startIndex, sizeof(freqSize));
        freqSizeList.push_back(freqSize);
        startIndex += sizeof(freqSize);
//        if (DEBUG_MODE) {
//            cout << "Read freqSize = " << freqSize << ", Updated startIndex = " << startIndex << endl;
//        }
    }
    // Unmap the memory-mapped file and close it
//    if (DEBUG_MODE) {
//        cout << "Unmapping memory and closing file descriptor." << endl;
//    }
    munmap(mapped_memory, length + offset - pa_offset);
    close(index_fd);
}


vector<pair<uint32_t, uint32_t>> QueryProcessor::_getPostingsList(const string &term) {
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


ListPointer QueryProcessor::_openList(string term) {
    ListPointer lp;

    // Check if the term exists in the lexicon
    if (lexicon.lexiconList.find(term) != lexicon.lexiconList.end()) {
        // Retrieve the LexiconItem for the term
        LexiconItem lexItem = lexicon.lexiconList[term];
        uint32_t beginPos = lexItem.beginPos;
        uint32_t endPos = lexItem.endPos;
        uint32_t blockNum = lexItem.blockNum;

        // Decode the docIDs and frequencies from the index file
        lp.docIDs.clear();
        lp.freqs.clear();

        // Loop through all blocks to decode docIDs and frequencies
        for (uint32_t block = 0; block < blockNum; ++block) {
            uint32_t blockStart = beginPos + block * (endPos - beginPos) / blockNum;
            uint32_t blockEnd = blockStart + (endPos - beginPos) / blockNum;

            // Decode docIDs and frequencies for the current block
            vector<uint32_t> decodedDocIDs = _decodeChunkToIntList(blockStart, blockEnd);
            vector<uint32_t> decodedFreqs = _decodeChunkToIntList(blockEnd, blockEnd + decodedDocIDs.size() * sizeof(uint32_t));

            // Append the decoded data to the list pointer
            lp.docIDs.insert(lp.docIDs.end(), decodedDocIDs.begin(), decodedDocIDs.end());
            lp.freqs.insert(lp.freqs.end(), decodedFreqs.begin(), decodedFreqs.end());
        }

        lp.listSize = lp.docIDs.size();  // Set the size of the list
        lp.isOpen = true;  // Mark the list as open
    } else {
        cerr << "Term not found in lexicon: " << term << endl;
    }

    return lp;
}


void QueryProcessor::_closeList(ListPointer &lp) {
    lp.isOpen = false;  // Mark the list as closed
    lp.index = 0;  // Reset the index
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


// Function to return the next document ID greater than or equal to the given docID
uint32_t QueryProcessor::_nextGEQ(ListPointer &lp, uint32_t docID) {
    // Traverse the docIDs in the ListPointer
    while (lp.index < lp.listSize) {
        if (lp.docIDs[lp.index] >= docID) {
            return lp.docIDs[lp.index];  // Return the docID if it's greater than or equal to the given docID
        }
        lp.index++;  // Move to the next docID
    }
    return MAX_DOC_ID;  // Return a sentinel value if no docID >= docID is found
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
    struct ScoreDocPair {
        double score;
        uint32_t docId;
        ScoreDocPair(double s, uint32_t i) : score(s), docId(i) {}

        // Override the comparator for the priority queue, to define a minHeap
        bool operator<(const ScoreDocPair &rhs) const {
            return -score < -rhs.score;
        }
    };

    priority_queue<ScoreDocPair> minHeap;  // Priority queue to store the top-k scores

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
//    cout << "Size of Score List: " << scoreList.size() << endl;
    struct ScoreDocPair {
        double score;
        uint32_t docId;
        ScoreDocPair(double s, uint32_t i) : score(s), docId(i) {}

        // Override the comparison operator for the priority queue, to define a minHeap
        bool operator<(const ScoreDocPair &rhs) const {
            return -score < -rhs.score;
        }
    };

    priority_queue<ScoreDocPair> minHeap;

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
        for (int i = 0; i < docId64.size(); i++) {
            originDocId += docId64[i];  // Reconstruct original document IDs from deltas
            // Insert score for each document
            docScoreMap[originDocId] = _getBM25(minTerm, originDocId, freq64[i]);
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


//// Updates the score hash for the given term, checking if the document exists in the current block
//void QueryProcessor::_updateScoreMap(string term, map<uint32_t, double> &docScoreMap) {
//    uint32_t beginPos = lexicon.lexiconList[term].beginPos;  // Starting position of the postings list for the term
//    uint32_t endPos = lexicon.lexiconList[term].endPos;  // End position of the postings list for the term
////    uint32_t blockNum = lexicon.lexiconList[term].blockNum;  // Number of blocks for the term
//
//    uint32_t metadataSize;
//    vector<uint32_t> lastDocIdList, docIdSizeList, freqSizeList;
//    // Load metadata for the current block
//    _openList(beginPos, metadataSize, lastDocIdList, docIdSizeList, freqSizeList);
//    int docIdPos = 0;
//    vector<uint32_t> docId64, freq64;
//    bool decodeRequired = true;
//
//    // Iterate through the documents in docScoreMap
//    for (auto it = docScoreMap.begin(); beginPos < endPos && it != docScoreMap.end();) {
//        uint32_t nextDocId = (next(it) == docScoreMap.end()) ? MAX_DOC_ID : next(it)->first;
//        uint32_t docId = it->first;
//
//        // Skip blocks that do not contain the current document
//        if (lastDocIdList.back() < docId) {
//            beginPos += _getMetaSize(metadataSize, lastDocIdList, docIdSizeList, freqSizeList);
//
//            if (beginPos < endPos) {
//                _openList(beginPos, metadataSize, lastDocIdList, docIdSizeList, freqSizeList);
//                decodeRequired = true;
//            } else {
//                break;  // Exit the loop if no more blocks remain
//            }
//            continue;
//        }
//
//        // Decode the block if necessary
//        if (decodeRequired) {
//            uint32_t metabyte = 4 + 3 * metadataSize * sizeof(uint32_t);  // Calculate metadata offset
//            uint32_t docIdPos = beginPos + metabyte;
//            uint32_t freqPos = docIdPos + docIdSizeList[0];
//
//            // Decode document IDs and frequencies
//            docId64 = _decodeChunkToIntList(docIdPos, docIdPos + docIdSizeList[0]);
//            freq64 = _decodeChunkToIntList(freqPos, freqPos + freqSizeList[0]);
//
//            // Document IDs delta decoding
//            uint32_t originDocId = 0;
//            for (auto &id: docId64) {
//                originDocId += id;  // Reconstruct the original document ID
//                id = originDocId;  // Modify the docId64 element
//            }
//
//            docIdPos = 0;
//            decodeRequired = false;
//        }
//
//        // Check for the document in the decoded block and update the score hash
//        while (docIdPos < docId64.size()) {
//            if (docId64[docIdPos] == docId) {
//                docScoreMap[docId] += _getBM25(term, docId, freq64[docIdPos]);  // Update score
//                break;
//            }
//            // Stop early if the current document ID exceeds the next document
//            if (docId64[docIdPos] >= nextDocId) {
//                break;
//            }
//            ++docIdPos;
//        }
//
//        ++it;  // Move to the next document ID
//    }
//}
// Updates the score hash for the given term, ensuring only documents common to all terms remain
// Updates the score hash for the given term, ensuring only documents common to all terms remain
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
        for (int i = 0; i < docId64.size(); i++) {
            originDocId += docId64[i];  // Reconstruct DocID for delta encoding
            scoreList[originDocId] += _getBM25(term, originDocId, freq64[i]);
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
        for (auto queryTerm : wordList) {
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
        for (string queryTerm : wordList) {
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


void QueryProcessor::_queryDAAT(vector<string> wordList, int queryMode) {
    _searchResultList.clear();  // Clear previous results

    // Priority queue to hold top k results based on BM25 score
    priority_queue<pair<double, uint32_t>, vector<pair<double, uint32_t>>, greater<>> pq;  // Min-heap
    unordered_map<uint32_t, double> docScoreMap;  // Store scores for each docID

    vector<ListPointer> lpList;
    // Open posting lists for each term
    for (const string &term : wordList) {
        ListPointer lp = _openList(term);
        if (lp.isOpen) {
            lpList.push_back(lp);
        }
    }

    // Handle conjunctive (AND) mode
    if (queryMode == CONJUNCTIVE) {
        // Find the first docID that exists in all lists
        uint32_t currentDocID = lpList[0].docIDs[0];
        for (ListPointer &lp : lpList) {
            currentDocID = max(currentDocID, lp.docIDs[0]);
        }

        while (currentDocID != MAX_DOC_ID) {
            bool allMatch = true;
            double scoreSum = 0;

            // Check if all terms have this document
            for (ListPointer &lp : lpList) {
                uint32_t nextDocID = _nextGEQ(lp, currentDocID);
                if (nextDocID != currentDocID) {
                    allMatch = false;
                    currentDocID = nextDocID;  // Update to the next docID to check
                    break;
                }
                // Accumulate BM25 score for this term and current docID
                uint32_t freq = lp.freqs[lp.index];
                scoreSum += _getBM25(wordList[&lp - &lpList[0]], currentDocID, freq);
            }

            // If all terms match the current docID, add it to the result map
            if (allMatch) {
                docScoreMap[currentDocID] = scoreSum;
                currentDocID = _nextGEQ(lpList[0], currentDocID + 1);  // Move to next docID
            }
        }
    }
        // Handle disjunctive (OR) mode
    else if (queryMode == DISJUNCTIVE) {
        // Process each list and accumulate scores in the map
        for (ListPointer &lp : lpList) {
            while (lp.index < lp.listSize) {
                uint32_t docID = lp.docIDs[lp.index];
                uint32_t freq = lp.freqs[lp.index];

                // Check if the score for the document was already calculated for this term
                if (docScoreMap.find(docID) == docScoreMap.end()) {
                    // If it's the first time encountering this document, calculate the BM25 score
                    docScoreMap[docID] = _getBM25(wordList[&lp - &lpList[0]], docID, freq);
                }
                else {
                    // Only accumulate BM25 score once per term per document
                    docScoreMap[docID] += _getBM25(wordList[&lp - &lpList[0]], docID, freq);
                }

                lp.index++;  // Move to the next docID in this list
            }
        }
    }

    // Enqueue top results into priority queue
    for (const auto &[docID, score] : docScoreMap) {
        pq.push({score, docID});
        if (pq.size() > NUM_TOP_RESULT) pq.pop();  // Keep only the top results
    }

    // Gather top results from the priority queue
    while (!pq.empty()) {
        _searchResultList.insert(pq.top().second, pq.top().first);  // Insert result: {docID, score}
        pq.pop();
    }
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
        vector<string> queryWordList = _splitQuery(query);  // Split the query

        if (!queryWordList.size()) {
            cout << "unlegal query" << endl;
            continue;
        }
        _queryTAAT(queryWordList, queryMode);  // Perform the TAAT query
//        _queryDAAT(queryWordList, queryMode);  // Perform the DAAT query
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

    // Based on queryMode, perform the appropriate query (CONJUNCTIVE or DISJUNCTIVE)
    if (queryMode == CONJUNCTIVE) {
        _queryTAAT(queryWordList, CONJUNCTIVE);
    } else if (queryMode == DISJUNCTIVE) {
        _queryTAAT(queryWordList, DISJUNCTIVE);
    } else {
        resultStream << "Invalid query mode. Please use 0 for CONJUNCTIVE or 1 for DISJUNCTIVE." << endl;
        return resultStream.str();
    }

    // Print results to the result stream
    _searchResultList.printToServer(resultStream);
    return resultStream.str();
}