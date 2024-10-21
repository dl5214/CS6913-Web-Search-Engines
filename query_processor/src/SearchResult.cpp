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

void SearchResultList::print() {
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

//string SearchResultList::extractSnippets(string org, string bstr, string estr, vector<string> word_list, vector<uint32_t> word_docNum_list)
//{
//    size_t begin_tag_len = bstr.length();
//
//    size_t start_pos = org.find(bstr) + begin_tag_len;
//    size_t end_pos = org.find(estr);
//    string text = org.substr(start_pos, end_pos - start_pos);
//    start_pos = text.find("\n");
//    text = text.substr(start_pos);
//
//    string snippets;
//
//    if (SNIPPETS_TYPE == LINEAR_SNIPPETS)
//    {
//        snippets = LinearMatchSnippets(text, word_list);
//    }
//    else if (SNIPPETS_TYPE == PREFIX_SNIPPETS)
//    {
//        snippets = PrefixSearchSnippets(text, word_list);
//    }
//    else if (SNIPPETS_TYPE == BM25_SNIPPETS || SNIPPETS_TYPE == VECTOR_SNIPPETS)
//    {
//        snippets = ScoreSnippets(text, word_list, word_docNum_list, SNIPPETS_TYPE);
//    }
//    else if (SNIPPETS_TYPE == KEYWORD_SNIPPETS)
//    {
//        snippets = KeywordSnippets(text, word_list, word_docNum_list);
//    }
//    else if (SNIPPETS_TYPE == WEIGHT_SNIPPETS)
//    {
//        snippets = WeightSnippets(text, word_list, word_docNum_list);
//    }
//    else if(SNIPPETS_TYPE == DUMP_SNIPPETS)
//    {
//        std::string snippetPath = SNIPPET_FOLDER;
//        snippetPath += "snippet_" + std::to_string(snippet_no++) + ".txt";
//        dumpSnippets(snippetPath,text, word_list, word_docNum_list);
//    }
//
//    return snippets;
//}

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

//// final project functions
//void SearchResultList::readFake(string fakefile_path, vector<string> &word_list, vector<uint32_t> &word_docNum_list)
//{
//    ifstream infile;
//    infile.open(fakefile_path);
//    Clear();
//
//    string word_line;
//    getline(infile, word_line);
//    word_list = splitLine(word_line);
//
//    for (int i = 0; i < word_list.size(); i++)
//    {
//        uint32_t num;
//        infile >> num;
//        word_docNum_list.push_back(num);
//    }
//
//    if (DEBUG_MODE)
//    {
//        cout << "word list: ";
//        for (int i = 0; i < word_list.size(); i++)
//        {
//            cout << word_list[i] << " " << word_docNum_list[i] << endl;
//        }
//        cout << endl;
//    }
//
//    for (int i = 0; i < RESULT_NUM; i++)
//    {
//        SearchResult res;
//        string tmp;
//        infile >> res.URL >> res.score >> res.docID;
//        _resultList.insert(_resultList.begin(), res);
//    }
//    infile.close();
//
//    return;
//}

//vector<string> SearchResultList::_splitLine(string line) {
//    vector<string> res;
//    string word;
//    for (int i = 0; i < line.length(); i++)
//    {
//        if (line[i] == ' ')
//        {
//            res.push_back(word);
//            word.clear();
//        }
//        else
//        {
//            word += line[i];
//        }
//    }
//    if (word.length())
//    {
//        res.push_back(word);
//    }
//    return res;
//}