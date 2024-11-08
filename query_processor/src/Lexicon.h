//
// Created by Dong Li on 10/16/24.
//

#ifndef SEARCHSYSTEM_LEXICON_H
#define SEARCHSYSTEM_LEXICON_H

#include "config.h"
using namespace std;


class LexiconItem {
public:
    uint32_t beginPos{}; // begin offset
    uint32_t endPos{};   // end offset
    uint32_t docNum{};
    uint32_t blockNum{};

    LexiconItem();
    ~LexiconItem();
    void update(uint32_t, uint32_t, uint32_t, uint32_t);
};

class Lexicon {
private:
//    string _indexPath;
    string _lexiconPath;
    uint32_t _getPostingDocNum(string); //calc Doc Num
    uint32_t _writeBlocks(string, uint32_t&, string, ofstream &);

public:
    map<string, LexiconItem> lexiconList;
    string indexPath;
    Lexicon();
    ~Lexicon();
    bool insert(string, uint32_t, uint32_t, uint32_t, uint32_t);
    void build(const string& mergedIndexPath);
    void write();
    void load();
};

#endif //SEARCHSYSTEM_LEXICON_H
