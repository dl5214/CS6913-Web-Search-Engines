//
// Created by Dong Li on 10/15/24.
//
#include "PageTable.h"
using namespace std;


void PageTable::_getAvgWordCount() {
    double allCount = 0;
    for (const auto& doc : pageTable) {
        allCount += doc.wordCount;
    }
    avgWordCount = allCount/pageTable.size();
}


void Document::print() const {
    cout << "docId:" << docId << " dataLength:" << dataLength
    << " wordCount:" << wordCount << " docPosition: "<< docPos << endl;
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
        << doc.docPos << " " << endl;
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
        infile >> newDoc.docId >> newDoc.dataLength >> newDoc.wordCount >> newDoc.docPos;
        pageTable.push_back(newDoc);
    }
    totalDoc = pageTable.size();
    if (DEBUG_MODE) {
        cout<< "totalDoc of pageTable: " << totalDoc << endl;
    }
    _getAvgWordCount();
}


// Binary search function to find the index of the given docId
int PageTable::findDocIndex(uint32_t docId) const {
    int left = 0;
    int right = pageTable.size() - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (pageTable[mid].docId == docId) {
            return mid;  // Found the docId, return the index
        }
        else if (pageTable[mid].docId < docId) {
            left = mid + 1;  // Search the right half
        }
        else {
            right = mid - 1;  // Search the left half
        }
    }

    return -1;  // docId not found
}