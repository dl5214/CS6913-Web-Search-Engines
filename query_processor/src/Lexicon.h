//
// Created by Dong Li on 10/16/24.
//

#ifndef SEARCHSYSTEM_LEXICON_H
#define SEARCHSYSTEM_LEXICON_H

#include "config.h"

using namespace std;

class LexiconItem {
public:
    uint32_t beginp; // begin offset
    uint32_t endp;   // end offset
    uint32_t docNum;
    void update(uint32_t, uint32_t, uint32_t);
};

class Lexicon {
private:
    string IndexPath;
    string LexiconPath;
    uint32_t calcDocNum(string); //calc Doc Num
    uint32_t WriteBitArray(string, ofstream &);

public:
    map<string, LexiconItem> _lexiconList;

    Lexicon();
    ~Lexicon();
    bool Insert(string, uint32_t, uint32_t, uint32_t);
    void Build(string, string);
    void Write();
};

#endif //SEARCHSYSTEM_LEXICON_H
