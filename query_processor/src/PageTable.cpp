//
// Created by Dong Li on 10/15/24.
//
#include "PageTable.h"
using namespace std;


void PageTable::_getAvgDataLen() {
    double allLen = 0;
    for (const auto& doc : pageTable) {
        allLen += doc.dataLength;
    }
    avgDataLen = allLen/pageTable.size();
}


void Document::print() const {
    cout << "docId:" << docId << " dataLength:" << dataLength
//    << " docNO:" << docNO << " url:" << url
    << " wordCount:" << wordCount << endl;
}


PageTable::PageTable(/* args */) {
    totalDoc = 0;
    pageTable.empty();
}


PageTable::~PageTable() {
}


void PageTable::add(Document docIdItem) {
    pageTable.push_back(docIdItem);
}


void PageTable::write() {
    ofstream outfile;
    string path;

    // Adjust the path to include the "_BIN" or "_ASCII" suffix before the file name, not within the directory structure
    if (FILE_MODE_BIN) {  // FILEMODE == BIN
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

    for (const auto& doc : pageTable) {
        outfile << doc.docId << " " << doc.dataLength << " " << doc.wordCount << " "
//               << " " << doc.docNO << doc.url << " "
                << endl;
    }

    outfile.close();
}


void PageTable::print() {
    for (const auto& doc : pageTable) {
        doc.print();
    }
}


void PageTable::load() {

    ifstream infile;
    string path;
    if (FILE_MODE_BIN) {  // FILEMODE == BIN
        path = string(PAGE_TABLE_PATH).substr(0, string(PAGE_TABLE_PATH).find_last_of('/')) + "/BIN_" + string(PAGE_TABLE_PATH).substr(string(PAGE_TABLE_PATH).find_last_of('/') + 1);
        infile.open(path, ofstream::binary);
    }
    else {  // FILEMODE == ASCII
        path = string(PAGE_TABLE_PATH).substr(0, string(PAGE_TABLE_PATH).find_last_of('/')) + "/ASCII_" + string(PAGE_TABLE_PATH).substr(string(PAGE_TABLE_PATH).find_last_of('/') + 1);
        infile.open(path);
    }
    if (DEBUG_MODE) {
        cout << "page table file path: " << path << endl;
    }
    if(!infile){
        cout << "can not read " << path << endl;
        exit(0);
    }
    pageTable.clear();

    while(!infile.eof()) {
        Document newDoc;
//        if (DEBUG_MODE and newDoc.docId % 100000 == 0) {
//            cout << newDoc.docId << " "
////            <<  newDoc.docNo << " "
//            << newDoc.dataLength << " " << newDoc.wordCount << endl;
//        }
        infile >> newDoc.docId
//        >> newDoc.docNo
        >> newDoc.dataLength >> newDoc.wordCount;
//        >> newdoc.gzp;
//        >>newdoc.url>>newdoc.gzp;
        pageTable.push_back(newDoc);
    }
    totalDoc = pageTable.size();
    if (DEBUG_MODE) {
        cout<< "totalDoc of pageTable: " << totalDoc << endl;
    }
    _getAvgDataLen();
}