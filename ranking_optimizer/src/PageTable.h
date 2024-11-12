//
// Created by Dong Li on 10/15/24.
//

#ifndef SEARCHSYSTEM_PAGETABLE_H
#define SEARCHSYSTEM_PAGETABLE_H

#include "config.h"
using namespace std;

class Document {
public:
    uint32_t docId; // doc ID
    uint32_t dataLength; // the number of data file containing this page, used for content retrieval
    uint32_t wordCount; // number of words
    streamoff docPos;  // document position as stream offset
    void print() const;
};


// assign document IDs (doc IDs) to pages
class PageTable {
private:
    /* data */
    void _getAvgWordCount();

public:
    uint32_t totalDoc;
    vector<Document> pageTable;
    uint32_t avgWordCount;

    PageTable(/* args */);
    ~PageTable();
    void add(Document docIDitem);
    void write();
    void print();
    void load();
    int findDocIndex(uint32_t docId) const;
};


#endif //SEARCHSYSTEM_PAGETABLE_H
