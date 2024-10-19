//
// Created by Dong Li on 10/15/24.
//
#include "PageTable.h"
using namespace std;

void Document::print() const {
    cout << "docID:" << docID << " docNO:" << docNO << " dataLen:" << dataLen
//    << " url:" << url <<
    << " wordnums:" << wordnums << endl;
}

PageTable::PageTable(/* args */) {
    _totalDoc = 0;
    _PageTable.empty();
}

PageTable::~PageTable() {
}

void PageTable::add(Document docIDitem) {
    _PageTable.push_back(docIDitem);
}

void PageTable::Write() {
    ofstream outfile;
    string path;

    if (FILEMODE_BIN) {  // FILEMODE == BIN
        path = "BIN_" + string(PAGE_TABLE_PATH);
        outfile.open(path,ofstream::binary);
    }
    else {  // FILEMODE == ASCII
        path = "ASCII_" + string(PAGE_TABLE_PATH);
        outfile.open(path);
    }

//    for (vector<Document>::iterator iter = _PageTable.begin();
//         iter != _PageTable.end(); ++iter) {
//        outfile << iter->docID << " " << iter->docNO << " " << iter->dataLen << " "
//                << iter->wordnums << " " << iter->url << " " << endl;
//    }
    for (const auto& doc : _PageTable) {
        outfile << doc.docID << " " << doc.docNO << " " << doc.dataLen << " " << doc.wordnums << " "
//                << doc.url << " "
                << endl;
    }

    outfile.close();
}

void PageTable::print() {
//    for (vector<Document>::iterator it = _PageTable.begin(); it != _PageTable.end(); ++it) {
//        it->print();
//    }
    for (const auto& doc : _PageTable) {
        doc.print();
    }
}