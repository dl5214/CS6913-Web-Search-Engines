//
// Created by Dong Li on 10/15/24.
//

#ifndef SEARCHSYSTEM_PAGETABLE_H
#define SEARCHSYSTEM_PAGETABLE_H

#include "config.h"
using namespace std;

class Document {
public:
    uint32_t docId; // doc id
    string docNo; // doc no
    uint32_t dataLength; // the number of data file containing this page
    uint32_t wordCount; // number of words
//    std::string url; // url
    void print() const;
};


// assign document IDs (doc IDs) to pages
class PageTable {
private:
    /* data */
    void _getAvgDataLen();

public:
    uint32_t totalDoc;
    vector<Document> pageTable;
    uint32_t avgDataLen;

    PageTable(/* args */);
    ~PageTable();
    void add(Document docIDitem);
    void write();
    void print();
    void load();
};


#endif //SEARCHSYSTEM_PAGETABLE_H
