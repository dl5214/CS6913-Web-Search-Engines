//
// Created by Dong Li on 10/20/24.
//
#include "SearchResult.h"
//#include "Snippets.h"
using namespace std;

SearchResult::SearchResult() {
}

//SearchResult::SearchResult(uint32_t docID, string URL, double score, string snippets) {
SearchResult::SearchResult(uint32_t docId, double score) {
    this->docId = docId;
//    this->URL = URL;
    this->score = score;
//    this->snippets = snippets;
}

SearchResult::~SearchResult() {
}

//void SearchResultList::Insert(uint32_t docID, double score, string snippets)
void SearchResultList::insert(uint32_t docID, double score) {
//    SearchResult res(docID, URL, score, snippets);
    SearchResult res(docID, score);
    resultList.push_back(res);
}

void SearchResultList::clear() {
    resultList.clear();
}

void SearchResultList::printToConsole() {
    cout << "Here are the Top " << NUM_TOP_RESULT << " results:" << std::endl;
    for (int i = resultList.size() - 1; i >= 0; i--)
    {
        cout << setw(2) << (resultList.size() - i) << ": "
//                  << resultList[i].URL << " " << resultList[i].score << " " << resultList[i].docID << endl;
        << resultList[i].score << " " << resultList[i].docId << endl;
//        cout << resultList[i].snippets << endl;
        cout << endl;
    }
}


// For front-end
void SearchResultList::printToServer(ostream &out) {
    for (const auto &result : resultList) {
        out << "DocId: " << result.docId << ", Score: " << result.score << endl;
    }
}


bool SearchResultList::_isInList(string word, vector<string> &word_list)
{
    for (int i = 0; i < word_list.size(); i++)
    {
        if (word == word_list[i])
        {
            return true;
        }
    }
    return false;
}