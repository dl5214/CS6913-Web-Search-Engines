//
// Created by Dong Li on 10/15/24.
//

#ifndef SEARCHSYSTEM_PAGETABLE_H
#define SEARCHSYSTEM_PAGETABLE_H

#include "config.h"
using namespace std;

class Document {
public:
    uint32_t docID; // doc id
    string docNO; // doc no
    uint32_t dataLen; // the number of data file containing this page
    uint32_t wordnums; // number of words
//    std::string url; // url
    void print() const;
};


// assign document IDs (doc IDs) to pages
class PageTable {
private:
    /* data */
    uint32_t _totalDoc;
    vector<Document> _PageTable;

public:
    PageTable(/* args */);
    ~PageTable();
    void add(Document docIDitem);
    void Write();
    void print();
};


#endif //SEARCHSYSTEM_PAGETABLE_H
