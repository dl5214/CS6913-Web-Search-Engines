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
#include <numeric>  // For iota and accumulate
using namespace std;


// Score DocID Pair, with comparator on Score, to create a minHeap and maintain top K BM25 score
struct DocScoreEntry {
    double score;
    uint32_t docId;

    DocScoreEntry(double s, uint32_t i) : score(s), docId(i) {}

    // Override: Comparator for priority queue (minHeap)
    bool operator<(const DocScoreEntry &rhs) const {
        return -score < -rhs.score;
    }
};


class QueryProcessor {
private:
    SearchResultList _searchResultList;

    double _getBM25(string term, uint32_t docID, uint32_t freq); // BM25 scoring function
    vector<pair<uint32_t, uint32_t>> _getPostingsList(string term);
    uint32_t _getFreq(string term, uint32_t docID); // Get term frequency for a specific document
    string readDocContent(uint32_t docId);  // read the doc Content by docId

    void _openList(uint32_t termID, uint32_t& df, vector<uint32_t>& docIDs, vector<uint32_t>& freqs, vector<uint32_t>& positions);
    vector<string> _splitQuery(const string& query); // Split the query into terms

    uint32_t _getMetaSize(uint32_t termID, vector<uint32_t>& docIDs, vector<uint32_t>& freqs, vector<uint32_t>& positions); // Calculate metadata size for a term

    void _getListTopK(vector<double>& scoreList, int K); // Find top K scores
    void _getMapTopK(map<uint32_t, double>& scoreMap, int K); // Find top K scores in a hash map

    vector<uint32_t> _decodeChunkToIntList(uint32_t chunkSize, uint32_t blockSize); // Decode chunks of data, either DocId or Freq

    void _decodeOneBlock(string term, uint32_t& beginPos, vector<uint32_t>& docIdList, vector<uint32_t>& freqList); // Decode a single block
    void _decodeBlocks(string term, vector<uint32_t>& docIdList, vector<uint32_t>& freqList); // Decode blocks

    // TAAT DISJUNCTIVE
    void _decodeOneBlockToList(string term, uint32_t& beginPos, vector<double>& scoreList); // Decode a single block
    void _decodeBlocksToList(string term, vector<double>& scoreList); // Decode blocks for compressed scores

    // TAAT CONJUNCTIVE
    void _decodeOneBlockToMap(string term, uint32_t& beginPos, map<uint32_t, double>& scoreMap); // Decode block to map
    void _decodeBlocksToMap(string term, map<uint32_t, double>& scoreMap); // Decode blocks to hash map
    void _updateScoreMap(string term, map<uint32_t, double>& scoreMap); // Update scores using a hash map

    void _queryTAAT(vector<string> word_list, int queryMode);  // Term-at-a-time query
    void _queryDAAT(vector<string> word_list, int queryMode);  // Document-at-a-time query

    uint32_t _nextGEQ(const vector<uint32_t>& docIDList, uint32_t currentDocID);
    // Helper function for DAAT Disjunctive (OR) query processing using Top-K MaxScore Algorithm
    void _maxScoreTopK(const vector<string>& wordList, const vector<vector<uint32_t>>& docIDLists, const vector<vector<uint32_t>>& freqLists);

    // Helper functions for MaxScore Algorithm
    void _outputTopKResults(priority_queue<DocScoreEntry>& topKHeap);  // Output top-k results


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
