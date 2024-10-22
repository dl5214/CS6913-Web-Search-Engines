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
//    string URL;
    double score;
//    string snippets;
    SearchResult();
//    SearchResult(uint32_t, string, double, string);
    SearchResult(uint32_t, double);
    ~SearchResult();
};

class SearchResultList {
private:
    bool _isInList(string, vector<string> &);
    int snippet_no = 400;


public:
    vector<SearchResult> resultList;
//    void Insert(uint32_t, string, double, string);
    void insert(uint32_t, double);
    void clear();
    void printToConsole();
    void printToServer(ostream &out = cout);  // overload, for front-end

};

#endif //SEARCHSYSTEM_SEARCHRESULT_H
