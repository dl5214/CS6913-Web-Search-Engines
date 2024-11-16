//
// Created by Dong Li on 10/20/24.
//
#include "SearchResult.h"
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
    // Set the precision for score output to 4 decimal places
    cout << fixed << setprecision(4);
    // Iterate through the results in order of insertion
    for (int i = 0; i < resultList.size(); i++) {
        cout << setw(2) << (i + 1) << ": "  // Output the rank
             << resultList[i].score << " " << resultList[i].docId << endl;
        cout << resultList[i].content << endl;  // Output the content
        cout << endl;
    }
}


// For front-end server output
void SearchResultList::printToServer(ostream &out) {
    // Iterate through the results in order of insertion
    for (auto &result : resultList) {
        // Replace commas in content with spaces to avoid format issues
        replace(result.content.begin(), result.content.end(), ',', ' ');
        // Output the result to the server stream
        out << "DocId: " << result.docId << ", Score: " << result.score
            << ", Content: " << result.content << endl;
    }
}