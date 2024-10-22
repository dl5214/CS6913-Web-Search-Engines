//
// Created by Dong Li on 10/19/24.
//

#ifndef SEARCHSYSTEM_QUERYPROCESSOR_H
#define SEARCHSYSTEM_QUERYPROCESSOR_H

#include "config.h"
#include "PageTable.h"
#include "InvertedList.h"
#include "Lexicon.h"
#include "SearchResult.h"
#include <string>
#include <vector>
#include <map>
#include <sys/mman.h>  // For mmap and munmap
#include <fcntl.h>     // For open
#include <unistd.h>    // For close, sysconf
#include <sys/types.h> // For types used by mmap (off_t)
#include <sys/stat.h>  // For file status (e.g., used with open)
using namespace std;


// ListPointer structure for DAAT query processing
struct ListPointer {
    vector<uint32_t> docIDs;  // List of document IDs for this term
    vector<uint32_t> freqs;   // Corresponding term frequencies in each document
    size_t index = 0;         // Current position in the posting list
    size_t listSize = 0;      // Size of the posting list
    bool isOpen = false;      // Indicates if the list is currently open
};


class QueryProcessor {
private:
    SearchResultList _searchResultList;

    double _getBM25(string term, uint32_t docID, uint32_t freq); // BM25 scoring function
    vector<pair<uint32_t, uint32_t>> _getPostingsList(const string &term);
    uint32_t _getFreq(string term, uint32_t docID); // Get term frequency for a specific document
    uint32_t _nextGEQ(ListPointer &lp, uint32_t docID);
    ListPointer _openList(string term);  // for DAAT use
    void _closeList(ListPointer &lp);
    void _openList(uint32_t termID, uint32_t& df, vector<uint32_t>& docIDs, vector<uint32_t>& freqs, vector<uint32_t>& positions);
    vector<string> _splitQuery(string query); // Split the query into terms

    uint32_t _getMetaSize(uint32_t termID, vector<uint32_t>& docIDs, vector<uint32_t>& freqs, vector<uint32_t>& positions); // Calculate metadata size for a term

    void _getListTopK(vector<double>& scoreList, int K); // Find top K scores
    void _getMapTopK(map<uint32_t, double>& scoreMap, int K); // Find top K scores in a hash map

    vector<uint32_t> _decodeChunkToIntList(uint32_t chunkSize, uint32_t blockSize); // Decode chunks of data, either DocId or Freq
    void _updateScoreMap(string term, map<uint32_t, double>& scoreMap); // Update scores using a hash map
    void _decodeOneBlockToMap(string block, uint32_t& docID, map<uint32_t, double>& scores); // Decode block to map
    void _decodeBlocksToMap(string block, map<uint32_t, double>& decodedMap); // Decode blocks to hash map
    void _decodeOneBlockToList(string block, uint32_t& docID, vector<double>& scores); // Decode a single block
    void _decodeBlocksToList(string block, vector<double>& decodedScores); // Decode blocks for compressed scores

    void _queryTAAT(vector<string> word_list, int queryMode);  // Term-at-a-time query
    void _queryDAAT(vector<string> word_list, int queryMode);  // Document-at-a-time query

public:
    PageTable pageTable;  // Reference to document table
    InvertedList invertedList;  // Reference to inverted index
    Lexicon lexicon;  // Reference to lexicon

    QueryProcessor();  // Constructor
    ~QueryProcessor();  // Destructor

    void queryLoop();  // Main loop for processing queries
    void testQuery();  // Function to test queries
    string processQuery(const string &query, int queryMode);
};

#endif //SEARCHSYSTEM_QUERYPROCESSOR_H
