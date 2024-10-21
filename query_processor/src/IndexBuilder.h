//
// Created by Dong Li on 10/16/24.
//

#ifndef SEARCHSYSTEM_INDEXBUILDER_H
#define SEARCHSYSTEM_INDEXBUILDER_H

#include "config.h"
#include "zlib.h"
#include "PageTable.h"
#include "InvertedList.h"
#include "Lexicon.h"
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <fstream>
#include <queue>
#include <tuple>
#include <sstream>

using namespace std;

class SortedPosting {
public:
    map<string, uint32_t> sortedList;
    void print();
};

class IndexBuilder {
private:
    /* Helper functions for the merging process */
    string _extractContent(string org, string bstr, string estr);
    string _getFirstLine(string);
    uint32_t _calcWordFreq(string, uint32_t);  // Calculate (word,Freq) in TEXT

    vector<pair<uint32_t, uint32_t>> _parsePostings(const string& postingsStr);  // Parse postings from a string
    void _mergePostingLists(vector<pair<uint32_t, uint32_t>>& base, const vector<pair<uint32_t, uint32_t>>& newPostings, bool ordered);  // Merge posting lists for a given word
    void _writeMergedPostings(ofstream& outfile, const string& word, const vector<pair<uint32_t, uint32_t>>& postings);  // Write merged postings to file

public:
    PageTable pageTable;
    InvertedList invertedList;
    Lexicon lexicon;

    IndexBuilder();
    ~IndexBuilder();

    /* Public functions */
    void readData(const char *filepath);  // Read data from the file
    void mergeIndex();  // Perform multi-way merge of index files into one
    void buildLexicon();
    void writePageTable();  // Write page table to disk
    void writeLexicon();  // Write lexicon to disk
};

#endif //SEARCHSYSTEM_INDEXBUILDER_H