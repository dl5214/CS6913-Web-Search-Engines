//
// Created by Dong Li on 10/16/24.
//

#ifndef SEARCHSYSTEM_INVERTEDLIST_H
#define SEARCHSYSTEM_INVERTEDLIST_H

#include "config.h"
#include <filesystem>
using namespace std;


class InvertedList {
public:
    uint32_t allIndexSize;
    map<string, vector<pair<uint32_t,uint32_t>>> hashWord;
    uint32_t indexFileCount; //record write file num

    InvertedList();
    ~InvertedList();
    string getIndexFilePath();
    string getIndexFilePath(uint32_t);
//    string getFinalIndexFilePath();
    void insertWord(string,uint32_t,uint32_t);
    void clear();
    void writeToFile();

private:
    bool _creatIndexFolder();
    bool _clearIndexFolder(bool);
    void _countIndexFiles();

};

#endif //SEARCHSYSTEM_INVERTEDLIST_H
