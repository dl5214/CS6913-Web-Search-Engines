//
// Created by Dong Li on 10/15/24.
//
#include "PageTable.h"
using namespace std;

void Document::print() const {
    cout << "docID:" << docID << " dataLen:" << dataLen
//    << " docNO:" << docNO << " url:" << url
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

    // Adjust the path to include the "_BIN" or "_ASCII" suffix before the file name, not within the directory structure
    if (FILEMODE_BIN) {  // FILEMODE == BIN
        path = string(PAGE_TABLE_PATH).substr(0, string(PAGE_TABLE_PATH).find_last_of('/')) + "/BIN_" + string(PAGE_TABLE_PATH).substr(string(PAGE_TABLE_PATH).find_last_of('/') + 1);
        outfile.open(path, ofstream::binary);
    }
    else {  // FILEMODE == ASCII
        path = string(PAGE_TABLE_PATH).substr(0, string(PAGE_TABLE_PATH).find_last_of('/')) + "/ASCII_" + string(PAGE_TABLE_PATH).substr(string(PAGE_TABLE_PATH).find_last_of('/') + 1);
        outfile.open(path);
    }

    // Check for errors while opening the file
    if (!outfile.is_open()) {
        cerr << "Error opening output file: " << path << endl;
        cerr << "Errno: " << errno << ", Error: " << strerror(errno) << endl;
        return;
    }

//    for (vector<Document>::iterator iter = _PageTable.begin();
//         iter != _PageTable.end(); ++iter) {
//        outfile << iter->docID << " " << iter->docNO << " " << iter->dataLen << " "
//                << iter->wordnums << " " << iter->url << " " << endl;
//    }
    for (const auto& doc : _PageTable) {
        outfile << doc.docID << " " << doc.dataLen << " " << doc.wordnums << " "
//               << " " << doc.docNO << doc.url << " "
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