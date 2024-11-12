//
// Created by Dong Li on 10/20/24.
//
#include "SearchResult.h"
//#include "Snippets.h"
using namespace std;

SearchResult::SearchResult() {
}

//SearchResult::SearchResult(uint32_t docID, string URL, double score, string snippets) {
SearchResult::SearchResult(uint32_t docId, double score, string content) {
    this->docId = docId;
    this->score = score;
    this->content = content;
}

SearchResult::~SearchResult() {
}

//void SearchResultList::Insert(uint32_t docID, double score, string snippets)
void SearchResultList::insert(uint32_t docID, double score, string content) {
//    SearchResult res(docID, URL, score, snippets);
    SearchResult resultItem(docID, score, content);
    resultList.push_back(resultItem);
}

void SearchResultList::clear() {
    resultList.clear();
}

void SearchResultList::printToConsole() {
    cout << "Here are the Top " << NUM_TOP_RESULT << " results:" << endl;
    for (int i = resultList.size() - 1; i >= 0; i--)
    {
        cout << setw(2) << (resultList.size() - i) << ": "
        << resultList[i].score << " " << resultList[i].docId << endl;
        cout << resultList[i].content << endl;
        cout << endl;
    }
}

// For front-end
void SearchResultList::printToServer(ostream &out) {
    for (auto &result : resultList) {

        replace(result.content.begin(), result.content.end(), ',', ' ');

        out << "DocId: " << result.docId << ", Score: " << result.score
        << ", Content: " << result.content << endl;
    }
}