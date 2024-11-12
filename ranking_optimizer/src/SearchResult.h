//
// Created by Dong Li on 10/20/24.
//

#ifndef SEARCHSYSTEM_SEARCHRESULT_H
#define SEARCHSYSTEM_SEARCHRESULT_H

#include "config.h"
using namespace std;

class SearchResult {
public:
    uint32_t docId;
    double score;
    string content;
    SearchResult();
    SearchResult(uint32_t, double, string);
    ~SearchResult();
};

class SearchResultList {
public:
    vector<SearchResult> resultList;
    void insert(uint32_t, double, string);
    void clear();
    void printToConsole();
    void printToServer(ostream &out = cout);  // overload, for front-end
};

#endif //SEARCHSYSTEM_SEARCHRESULT_H
