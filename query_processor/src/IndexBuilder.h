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
using namespace std;

class SortedPosting {
public:
    map<string, uint32_t> _sortedList;
    void print();
};

class IndexBuilder {
private:
    /* data */
    string extractContent(string org, string bstr, string estr);
    string getFirstLine(string);
    uint32_t calcWordFreq(string, uint32_t); // calc (word,Freq) in TEXT
    void mergeIndex(uint32_t, uint32_t);

public:
    PageTable _PageTable;
    InvertedList _InvertedList;
    Lexicon _Lexicon;

    IndexBuilder();
    ~IndexBuilder();
    void read_data(const char *filepath);
    void mergeIndexToOne();
    void WritePageTable();
    void WriteLexicon();
};

#endif //SEARCHSYSTEM_INDEXBUILDER_H
