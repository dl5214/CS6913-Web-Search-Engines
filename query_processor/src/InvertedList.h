//
// Created by Dong Li on 10/16/24.
//

#ifndef SEARCHSYSTEM_INVERTEDLIST_H
#define SEARCHSYSTEM_INVERTEDLIST_H

#include "config.h"
#include <filesystem>
using namespace std;

class InvertedList
{
public:
    uint32_t allIndexSize;
    map<string, vector<pair<uint32_t,uint32_t>>> HashWord;
    uint32_t indexFileNum; //record write file num

    InvertedList();
    ~InvertedList();
    string getIndexFilePath();
    string getIndexFilePath(uint32_t);
//    string getFinalIndexFilePath();
    void Insert(string,uint32_t,uint32_t);
    void Clear();
    void Write();

private:
    bool creatIndexFolder();
    bool clearIndexFolder(bool);
    void countIndexFileNum();

};

#endif //SEARCHSYSTEM_INVERTEDLIST_H
