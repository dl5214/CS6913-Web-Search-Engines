//
// Created by Dong Li on 10/16/24.
//

#include "InvertedList.h"
using namespace std;

InvertedList::InvertedList() {
    HashWord.clear();
    allIndexSize = 0;
    indexFileNum = 0;

    if (!INDEX_FLAG) {
        countIndexFileNum();
        return;
    }

    // if folder exist,delete old file;
    // else create
    string IndexFoldPath = INDEX_FILE_FOLDER_PATH;

    if (filesystem::exists(IndexFoldPath)) {
        if (!clearIndexFolder(false)) {
            throw "cannot clear Index Folder!";
        }
    }
    else {
        if (!creatIndexFolder()) {
            throw "cannot create Index Folder!";
        }
    }
}

InvertedList::~InvertedList() {
    if (DELETE_INTERMEDIATE) {
        clearIndexFolder(true);
    }
}

bool InvertedList::creatIndexFolder() {
    string IndexFoldPath = INDEX_FILE_FOLDER_PATH;
    return filesystem::create_directory(IndexFoldPath);
}

bool InvertedList::clearIndexFolder(bool deleteFolder) {
    string IndexFoldPath = INDEX_FILE_FOLDER_PATH;
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

void InvertedList::countIndexFileNum() {
    indexFileNum = 0;
    string IndexFoldPath = INDEX_FILE_FOLDER_PATH;
    for (const auto &entry : filesystem::directory_iterator(IndexFoldPath)) {
        if (!filesystem::is_directory(entry.path())) {
            indexFileNum += 1;
        }
    }
}

// inset (docID,freq) in wordHash[word]
void InvertedList::Insert(string word, uint32_t docID, uint32_t freq) {
    // judge if have enough space to put
    bool indexChunkFull = false;
    if (HashWord.count(word)) {
        if (allIndexSize + POST_BYTES > INDEX_CHUNK_SIZE)
            indexChunkFull = true;
        else {
            HashWord[word].push_back(pair<uint32_t, uint32_t>(docID, freq));
            allIndexSize += POST_BYTES;
        }
    }
    else {
        if (allIndexSize + POST_BYTES + AVG_WORD_BYTES > INDEX_CHUNK_SIZE) {
            indexChunkFull = true;
        }
        else {
            vector<pair<uint32_t, uint32_t>> newVec;
            newVec.push_back(pair<uint32_t, uint32_t>(docID, freq));
            HashWord[word] = newVec;
            allIndexSize += POST_BYTES + AVG_WORD_BYTES;
        }
    }

    // INDEX CHUNK is full, need write out.
    if (indexChunkFull) {
        Write();
        Clear();
        vector<pair<uint32_t, uint32_t>> newVec;
        newVec.push_back(pair<uint32_t, uint32_t>(docID, freq));
        HashWord[word] = newVec;
        allIndexSize += POST_BYTES + AVG_WORD_BYTES;
    }
}

string InvertedList::getIndexFilePath() {
    indexFileNum += 1;

    if (FILEMODE_BIN){  // FILEMODE == BIN
        return string(INDEX_FILE_FOLDER_PATH) + "BIN_" + to_string(indexFileNum - 1) + ".bin";
    }
    else {  // FILEMODE == ASCII
        return string(INDEX_FILE_FOLDER_PATH) + "ASCII_" + to_string(indexFileNum - 1) + ".txt";
    }
}

string InvertedList::getIndexFilePath(uint32_t fileNum) {
    if (FILEMODE_BIN){  // FILEMODE == FILEMODE_BIN
        return string(INDEX_FILE_FOLDER_PATH) + "BIN_" + to_string(fileNum) + ".bin";
    }
    else {  // FILEMODE == ASCII
        return string(INDEX_FILE_FOLDER_PATH) + "ASCII_" + to_string(fileNum) + ".txt";
    }

}

void InvertedList::Write()
{
    string path = getIndexFilePath();
    ofstream outfile;
    if (FILEMODE_BIN) {  // FILEMODE == BIN
        outfile.open(path, ofstream::binary);
    }
    else {  // FILEMODE == ASCII
        outfile.open(path);
    }
//    for (map<string, vector<pair<uint32_t, uint32_t>>>::iterator it = HashWord.begin(); it != HashWord.end(); ++it) {
//        outfile << it->first << ":";
//        for (vector<pair<uint32_t, uint32_t>>::iterator iter = it->second.begin(); iter != it->second.end(); ++iter) {
//            if (iter != it->second.begin()) {
//                outfile << ",";
//            }
//            outfile << iter->first << " " << iter->second;
//        }
//        outfile << endl;
//    }
    for (const auto& [word, postings] : HashWord) {
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

void InvertedList::Clear()
{
    allIndexSize = 0;
    HashWord.clear();
}