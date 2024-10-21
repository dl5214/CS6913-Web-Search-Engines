//
// Created by Dong Li on 10/16/24.
//

#include "InvertedList.h"
using namespace std;


InvertedList::InvertedList() {
    hashWord.clear();
    allIndexSize = 0;
    indexFileCount = 0;

    if (!INDEX_FLAG) {
        _countIndexFiles();
        return;
    }

    // if folder exist,delete old file;
    // else create
    string IndexFoldPath = INTERMEDIATE_INDEX_PATH;

    if (filesystem::exists(IndexFoldPath)) {
        if (!_clearIndexFolder(false)) {
            throw "cannot clear Index Folder!";
        }
    }
    else {
        if (!_creatIndexFolder()) {
            throw "cannot create Index Folder!";
        }
    }
}


InvertedList::~InvertedList() {
    if (DELETE_INTERMEDIATE) {
        _clearIndexFolder(true);
    }
}


bool InvertedList::_creatIndexFolder() {
    string IndexFoldPath = INTERMEDIATE_INDEX_PATH;
    return filesystem::create_directory(IndexFoldPath);
}


bool InvertedList::_clearIndexFolder(bool deleteFolder) {
    string IndexFoldPath = INTERMEDIATE_INDEX_PATH;
    for (const auto &entry : filesystem::directory_iterator(IndexFoldPath)) {
        if (!filesystem::is_directory(entry.path())) {
            if (!filesystem::remove(entry.path())) {
                return false;
            }
        }
    }
    if (deleteFolder)
        return filesystem::remove(IndexFoldPath);

    return true;
}


void InvertedList::_countIndexFiles() {
    indexFileCount = 0;
    string IndexFoldPath = INTERMEDIATE_INDEX_PATH;
    for (const auto &entry : filesystem::directory_iterator(IndexFoldPath)) {
        if (!filesystem::is_directory(entry.path())) {
            indexFileCount += 1;
        }
    }
}


// inset (docID,freq) in wordHash[word]
void InvertedList::insertWord(string word, uint32_t docID, uint32_t freq) {
    // judge if have enough space to put
    bool indexChunkFull = false;
    if (hashWord.count(word)) {
        if (allIndexSize + POST_BYTES > INDEX_CHUNK_SIZE)
            indexChunkFull = true;
        else {
            hashWord[word].push_back(pair<uint32_t, uint32_t>(docID, freq));
            allIndexSize += POST_BYTES;
        }
    }
    else {
        if (allIndexSize + POST_BYTES + AVG_WORD_BYTES > INDEX_CHUNK_SIZE) {
            indexChunkFull = true;
        }
        else {
            vector<pair<uint32_t, uint32_t>> newVec;
//            newVec.push_back(pair<uint32_t, uint32_t>(docID, freq));
            newVec.emplace_back(docID, freq);
            hashWord[word] = newVec;
            allIndexSize += POST_BYTES + AVG_WORD_BYTES;
        }
    }

    // INDEX CHUNK is full, need write out.
    if (indexChunkFull) {
        writeToFile();
        clear();
        vector<pair<uint32_t, uint32_t>> newVec;
//        newVec.push_back(pair<uint32_t, uint32_t>(docID, freq));
        newVec.emplace_back(docID, freq);
        hashWord[word] = newVec;
        allIndexSize += POST_BYTES + AVG_WORD_BYTES;
    }
}


string InvertedList::getIndexFilePath() {
    indexFileCount += 1;

    if (FILE_MODE_BIN){  // FILEMODE == BIN
        return string(INTERMEDIATE_INDEX_PATH) + "BIN_" + to_string(indexFileCount - 1) + ".bin";
    }
    else {  // FILEMODE == ASCII
        return string(INTERMEDIATE_INDEX_PATH) + "ASCII_" + to_string(indexFileCount - 1) + ".txt";
    }
}

string InvertedList::getIndexFilePath(uint32_t fileNum) {
    if (FILE_MODE_BIN){  // FILEMODE == FILEMODE_BIN
        return string(INTERMEDIATE_INDEX_PATH) + "BIN_" + to_string(fileNum) + ".bin";
    }
    else {  // FILEMODE == ASCII
        return string(INTERMEDIATE_INDEX_PATH) + "ASCII_" + to_string(fileNum) + ".txt";
    }

}


//// Function to get the path for the final index file
//string InvertedList::getFinalIndexFilePath() {
//    // Generate final index file path in ../data/ directory
//    if (FILEMODE_BIN) {  // FILEMODE == BIN
//        return string("../data/inverted_index.bin");
//    }
//    else {  // FILEMODE == ASCII
//        return string("../data/inverted_index.txt");
//    }
//}


void InvertedList::writeToFile() {
    string path = getIndexFilePath();
    ofstream outfile;
    if (FILE_MODE_BIN) {  // FILEMODE == BIN
        outfile.open(path, ofstream::binary);
    }
    else {  // FILEMODE == ASCII
        outfile.open(path);
    }

    for (const auto& [word, postings] : hashWord) {
        outfile << word << ":";

        for (size_t i = 0; i < postings.size(); ++i) {
            if (i > 0) {
                outfile << ",";
            }
            outfile << postings[i].first << " " << postings[i].second;
        }
        outfile << endl;
    }
    outfile.close();
}

void InvertedList::clear()
{
    allIndexSize = 0;
    hashWord.clear();
}