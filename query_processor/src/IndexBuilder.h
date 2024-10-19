////
//// Created by Dong Li on 10/16/24.
////
//
//#ifndef SEARCHSYSTEM_INDEXBUILDER_H
//#define SEARCHSYSTEM_INDEXBUILDER_H
//
//#include "config.h"
//#include "zlib.h"
//#include "PageTable.h"
//#include "InvertedList.h"
//#include "Lexicon.h"
//using namespace std;
//
//class SortedPosting {
//public:
//    map<string, uint32_t> _sortedList;
//    void print();
//};
//
//class IndexBuilder {
//private:
//    /* data */
//    string extractContent(string org, string bstr, string estr);
//    string getFirstLine(string);
//    uint32_t calcWordFreq(string, uint32_t); // calc (word,Freq) in TEXT
//    void mergeIndex(uint32_t, uint32_t);
//
//public:
//    PageTable _PageTable;
//    InvertedList _InvertedList;
//    Lexicon _Lexicon;
//
//    IndexBuilder();
//    ~IndexBuilder();
//    void read_data(const char *filepath);
//    void mergeIndex();
//    void WritePageTable();
//    void WriteLexicon();
//};
//
//#endif //SEARCHSYSTEM_INDEXBUILDER_H
//

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
    map<string, uint32_t> _sortedList;
    void print();
};

class IndexBuilder {
private:
    /* Helper functions for the merging process */
    string extractContent(string org, string bstr, string estr);
    string getFirstLine(string);
    uint32_t calcWordFreq(string, uint32_t);  // Calculate (word,Freq) in TEXT

    vector<pair<uint32_t, uint32_t>> parsePostings(const string& postingsStr);  // Parse postings from a string
    void mergePostingLists(vector<pair<uint32_t, uint32_t>>& base, const vector<pair<uint32_t, uint32_t>>& newPostings, bool ordered);  // Merge posting lists for a given word
    void writeMergedPostings(ofstream& outfile, const string& word, const vector<pair<uint32_t, uint32_t>>& postings);  // Write merged postings to file

public:
    PageTable _PageTable;
    InvertedList _InvertedList;
    Lexicon _Lexicon;

    IndexBuilder();
    ~IndexBuilder();

    /* Public functions */
    void read_data(const char *filepath);  // Read data from the file
    void mergeIndex();  // Perform multi-way merge of index files into one
    void WritePageTable();  // Write page table to disk
    void WriteLexicon();  // Write lexicon to disk
};

#endif //SEARCHSYSTEM_INDEXBUILDER_H