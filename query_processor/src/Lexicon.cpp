//
// Created by Dong Li on 10/16/24.
//
#include "Lexicon.h"
#include <sys/stat.h>
using namespace std;

// Function to encode a uint32 value using Varbyte encoding
vector<uint8_t> varbyte_encode(uint32_t value) {
    vector<uint8_t> encoded;
    while (value > 0) {
        // Extract the 7 least significant bits
        uint8_t byte = value & 0x7F;
        // Set the high bit to indicate more bytes if needed
        if (value > 0x7F)
        {
            byte |= 0x80;
        }
        encoded.push_back(byte);
        // Shift the value to the right by 7 bits
        value >>= 7;
    }
    return encoded;
}

// Function to decode a Varbyte encoded sequence into a uint32 value
uint32_t varbyte_decode(const vector<uint8_t> &encoded) {
    uint32_t value = 0;
    for (size_t i = 0; i < encoded.size(); ++i) {
        // Extract the 7 least significant bits from the byte
        uint32_t byte = encoded[i] & 0x7F;
        // Add the extracted bits to the result
        value |= (byte << (7 * i));
        // If the high bit is not set, it's the last byte
        if (!(encoded[i] & 0x80)) {
            break;
        }
    }
    return value;
}

void LexiconItem::update(uint32_t bgp, uint32_t edp, uint32_t dn) {
    beginp = bgp;
    endp = edp;
    docNum = dn;
}

Lexicon::Lexicon() {
    // Adjust the paths to include the "_BIN" or "_ASCII" suffix before the file name, not within the directory structure
    if (FILEMODE_BIN) {  // FILEMODE == BIN
        LexiconPath = string(LEXICON_PATH).substr(0, string(LEXICON_PATH).find_last_of('/')) + "/BIN_" + string(LEXICON_PATH).substr(string(LEXICON_PATH).find_last_of('/') + 1);
        IndexPath = string(FINAL_INDEX_PATH).substr(0, string(FINAL_INDEX_PATH).find_last_of('/')) + "/BIN_" + string(FINAL_INDEX_PATH).substr(string(FINAL_INDEX_PATH).find_last_of('/') + 1);
    } else {  // FILEMODE == ASCII
        LexiconPath = string(LEXICON_PATH).substr(0, string(LEXICON_PATH).find_last_of('/')) + "/ASCII_" + string(LEXICON_PATH).substr(string(LEXICON_PATH).find_last_of('/') + 1);
        IndexPath = string(FINAL_INDEX_PATH).substr(0, string(FINAL_INDEX_PATH).find_last_of('/')) + "/ASCII_" + string(FINAL_INDEX_PATH).substr(string(FINAL_INDEX_PATH).find_last_of('/') + 1);
    }
}

Lexicon::~Lexicon() {
}

uint32_t Lexicon::calcDocNum(string idxWordList) {
    uint32_t spaceNum = 0;
    for (size_t i = 0; i < idxWordList.length(); i++) {
        if (idxWordList[i] == ' ') {
            spaceNum += 1;
        }
    }
    return (spaceNum + 1) / 2;  // Each word will have one space between docID and freq
}

bool Lexicon::Insert(string word, uint32_t beginp, uint32_t endp, uint32_t docNum) {
    if (word.empty()) {
        return false;
    }
    LexiconItem lexItem;
    lexItem.update(beginp, endp, docNum);
    _lexiconList[word] = lexItem;
    return true;
}

uint32_t Lexicon::WriteBitArray(string arr, ofstream &outfile) {
    size_t beginpos = 0;
    uint32_t freq, docID;
    bool isOk = true;
    uint32_t docNum = 0;
    // uint32_t prev_docID = 0;
    while (isOk) {
        size_t space, comma;
        try {
            space = arr.find(" ", beginpos);
            comma = arr.find(",", space);
            docID = stoi(arr.substr(beginpos, space - beginpos));

            if (comma != string::npos) {
                freq = stoi(arr.substr(space + 1, comma - space - 1));
            }
            else {
                freq = stoi(arr.substr(space + 1));
                isOk = false;
            }
        }
        catch (...) {
            cout << arr.substr(beginpos, space - beginpos) << " " << arr.substr(space + 1, comma - space - 1);
            cout << arr.substr(beginpos, space - beginpos).length() << endl;
            cout << arr.substr(space + 1, comma - space - 1).length() << endl;
            cout << docNum << endl;
            continue;
        }

        vector<uint8_t> endocID = varbyte_encode(docID);
        vector<uint8_t> enFreq = varbyte_encode(freq);
//        for (int i = 0; i < endocID.size(); i++) {
//            outfile.write(reinterpret_cast<const char *>(&endocID[i]), sizeof(uint8_t));
//        }
//        for (int i = 0; i < enFreq.size(); i++) {
//            outfile.write(reinterpret_cast<const char *>(&enFreq[i]), sizeof(uint8_t));
//        }
        for (const auto& byte : endocID) {
            outfile.write(reinterpret_cast<const char*>(&byte), sizeof(uint8_t));
        }

        for (const auto& byte : enFreq) {
            outfile.write(reinterpret_cast<const char*>(&byte), sizeof(uint8_t));
        }

        // prev_docID = docID;
        beginpos = comma + 1;
        docNum += 1;

    }
    return docNum;
}

//void Lexicon::Build(string path1, string path2) {
//    ifstream infile1;
//    ifstream infile2;
//    ofstream outfile;
//
//    if (FILEMODE_BIN) {  // FILEMODE == BIN
//        infile1.open(path1, ifstream::binary);
//        infile2.open(path2, ifstream::binary);
//        outfile.open(IndexPath, ofstream::binary);
//        uint32_t beginp, endp;
//
//        string line1, line2;
//        string word1, word2;
//
//        while (infile1 && infile2) {
//            if (word1.empty() && word2.empty()) {
//                beginp = outfile.tellp();
//                getline(infile1, line1);
//                getline(infile2, line2);
//                word1 = line1.substr(0, line1.find(":"));
//                word2 = line2.substr(0, line2.find(":"));
//            }
//            if (word1.compare(word2) == 0) {
//                // merge word1 and word2
//                string arr1 = line1.substr(line1.find(":") + 1);
//                string arr2 = line2.substr(line2.find(":") + 1);
//                string docID1 = arr1.substr(0, arr1.find(" "));
//                string docID2 = arr2.substr(0, arr2.find(" "));
//                if (arr1.empty() || arr2.empty()) {
//                    break;
//                }
//                uint32_t docNum1, docNum2;
//
//                if (docID1 < docID2) {
//                    docNum1 = WriteBitArray(arr1, outfile);
//                    docNum2 = WriteBitArray(arr2, outfile);
//                }
//                else {
//                    docNum2 = WriteBitArray(arr2, outfile);
//                    docNum1 = WriteBitArray(arr1, outfile);
//                }
//
//                // update Lexicon
//                endp = outfile.tellp();
//                Insert(word1, beginp, endp, docNum1 + docNum2);
//                beginp = endp;
//
//                // update line1 and line2
//                getline(infile1, line1);
//                getline(infile2, line2);
//                word1 = line1.substr(0, line1.find(":"));
//                word2 = line2.substr(0, line2.find(":"));
//            }
//            else if (word1.compare(word2) < 0) {
//                string arr1 = line1.substr(line1.find(":") + 1);
//                if (arr1.empty()) {
//                    break;
//                }
//
//                // write line1 only
//                uint32_t docNum = WriteBitArray(arr1, outfile);
//
//                // update Lexicon
//                endp = outfile.tellp();
//                Insert(word1, beginp, endp, docNum);
//                beginp = endp;
//
//                // update line1
//                getline(infile1, line1);
//                word1 = line1.substr(0, line1.find(":"));
//            }
//            else {
//                string arr2 = line2.substr(line2.find(":") + 1);
//                if (arr2.empty())
//                    break;
//
//                // write line2 only
//                uint32_t docNum = WriteBitArray(arr2, outfile);
//
//                // update Lexicon
//                endp = outfile.tellp();
//                Insert(word2, beginp, endp, docNum);
//                beginp = endp;
//
//                // update line2
//                getline(infile2, line2);
//                word2 = line2.substr(0, line2.find(":"));
//            }
//        }
//
//        if (infile1) {
//            string arr1 = line1.substr(line1.find(":") + 1);
//            if (!arr1.empty()) {
//                uint32_t docNum = WriteBitArray(arr1, outfile);
//
//                // update Lexicon
//
//                endp = outfile.tellp();
//                Insert(word1, beginp, endp, docNum);
//                beginp = endp;
//            }
//        }
//        if (infile2) {
//            string arr2 = line2.substr(line2.find(":") + 1);
//            if (!arr2.empty()) {
//                uint32_t docNum = WriteBitArray(arr2, outfile);
//
//                // update Lexicon
//                if (docNum != -1) {
//                    endp = outfile.tellp();
//                    Insert(word2, beginp, endp, docNum);
//                    beginp = endp;
//                }
//            }
//        }
//
//        while (infile1) {
//            string arr1 = line1.substr(line1.find(":") + 1);
//            if (arr1.empty()) {
//                break;
//            }
//            getline(infile1, line1);
//
//            uint32_t docNum = WriteBitArray(arr1, outfile);
//
//            // update Lexicon
//            if (docNum != -1) {
//                break;
//            }
//            endp = outfile.tellp();
//            Insert(word1, beginp, endp, docNum);
//            beginp = endp;
//        }
//        while (infile2) {
//            string arr2 = line2.substr(line2.find(":") + 1);
//            if (arr2.empty()) {
//                break;
//            }
//
//            getline(infile2, line2);
//
//            uint32_t docNum = WriteBitArray(arr2, outfile);
//
//            // update Lexicon
//            endp = outfile.tellp();
//            Insert(word2, beginp, endp, docNum);
//            beginp = endp;
//        }
//    }
//
//    else {  // FILEMODE == ASCII
//        infile1.open(path1);
//        infile2.open(path2);
//        outfile.open(IndexPath);
//        string line1, line2;
//        string word1, word2;
//        uint32_t beginp, endp;
//
//        while (infile1 && infile2) {
//            if (word1.empty() && word2.empty()) {
//                beginp = outfile.tellp();
//                getline(infile1, line1);
//                getline(infile2, line2);
//                word1 = line1.substr(0, line1.find(":"));
//                word2 = line2.substr(0, line2.find(":"));
//            }
//            if (word1.compare(word2) == 0) {
//                // merge word1 and word2
//                string arr1 = line1.substr(line1.find(":") + 1);
//                string arr2 = line2.substr(line2.find(":") + 1);
//                string docID1 = arr1.substr(0, arr1.find(" "));
//                string docID2 = arr2.substr(0, arr2.find(" "));
//                if (docID1 < docID2) {
//                    outfile << word1 << ":" << arr1 << "," << arr2 << endl;
//                }
//                else {
//                    outfile << word1 << ":" << arr2 << "," << arr1 << endl;
//                }
//
//                // update Lexicon
//                endp = outfile.tellp();
//                Insert(word1, beginp, endp, calcDocNum(arr1) + calcDocNum(arr2));
//                beginp = endp;
//
//                // update line1 and line2
//                getline(infile1, line1);
//                getline(infile2, line2);
//                word1 = line1.substr(0, line1.find(":"));
//                word2 = line2.substr(0, line2.find(":"));
//            }
//            else if (word1.compare(word2) < 0) {
//                // write line1 only
//                outfile << line1 << endl;
//
//                // update Lexicon
//                endp = outfile.tellp();
//                Insert(word1, beginp, endp, calcDocNum(line1));
//                beginp = endp;
//
//                // update line1
//                getline(infile1, line1);
//                word1 = line1.substr(0, line1.find(":"));
//            }
//            else {
//                // write line2 only
//                outfile << line2 << endl;
//
//                // update Lexicon
//                endp = outfile.tellp();
//                Insert(word2, beginp, endp, calcDocNum(line2));
//                beginp = endp;
//
//                // update line2
//                getline(infile2, line2);
//                word2 = line2.substr(0, line2.find(":"));
//            }
//        }
//
//        if (infile1) {
//            outfile << line1 << endl;
//            // update Lexicon
//            endp = outfile.tellp();
//            Insert(word1, beginp, endp, calcDocNum(line1));
//            beginp = endp;
//        }
//        if (infile2) {
//            outfile << line2 << endl;
//            // update Lexicon
//            endp = outfile.tellp();
//            Insert(word2, beginp, endp, calcDocNum(line2));
//            beginp = endp;
//        }
//
//        while (infile1) {
//            getline(infile1, line1);
//            outfile << line1 << endl;
//
//            // update Lexicon
//            endp = outfile.tellp();
//            Insert(word1, beginp, endp, calcDocNum(line1));
//            beginp = endp;
//        }
//        while (infile2) {
//            getline(infile2, line2);
//            outfile << line2 << endl;
//
//            // update Lexicon
//            endp = outfile.tellp();
//            Insert(word2, beginp, endp, calcDocNum(line2));
//            beginp = endp;
//        }
//    }
//
//
//    infile1.close();
//    infile2.close();
//    outfile.close();
//}
// New Build function that processes a single merged file
void Lexicon::Build(const string& mergedIndexPath) {
    ifstream infile;
    ofstream outfile;

    // Ensure output directory exists before writing the output file
    string outputDirectory = "../data/";  // Extract directory from IndexPath
    struct stat info;

    if (stat(outputDirectory.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
        cerr << "Error: Output directory does not exist or cannot be accessed: " << outputDirectory << endl;
        return;
    }

    const size_t BUFFER_SIZE = 50 * 1024;  // Define a buffer size (e.g., 50KB)

    if (FILEMODE_BIN) {
        infile.open(mergedIndexPath, ifstream::binary);
        outfile.open(IndexPath, ofstream::binary);
    } else {
        infile.open(mergedIndexPath);
        outfile.open(IndexPath);
    }

//    // Check if the files opened successfully
//    if (!infile.is_open() || !outfile.is_open()) {
//        cerr << "Error opening file(s)!" << endl;
//        return;
//    }
    // Check if the input file opened successfully
    if (!infile.is_open()) {
        cerr << "Error opening input file: " << mergedIndexPath << endl;
        cerr << "Errno: " << errno << ", Error: " << strerror(errno) << endl;
        return;
    }

    // Check if the output file opened successfully
    if (!outfile.is_open()) {
        cerr << "Error opening output file: " << IndexPath << endl;
        cerr << "Errno: " << errno << ", Error: " << strerror(errno) << endl;
        return;
    }

    // Set buffer for efficient reading and writing
    char* inBuffer = new char[BUFFER_SIZE];
    infile.rdbuf()->pubsetbuf(inBuffer, BUFFER_SIZE);

    char* outBuffer = new char[BUFFER_SIZE];
    outfile.rdbuf()->pubsetbuf(outBuffer, BUFFER_SIZE);

    string line;
    uint32_t beginp = 0, endp = 0;

    while (getline(infile, line)) {
        string word = line.substr(0, line.find(":"));
        string postingsStr = line.substr(line.find(":") + 1);

        // Get the current position in the output file (begin position for this word)
        beginp = outfile.tellp();

        // Encode the postings and write them in the output file
        uint32_t docNum = WriteBitArray(postingsStr, outfile);

        // Get the current position after writing (end position for this word)
        endp = outfile.tellp();

        // Insert the word and its positions into the Lexicon
        Insert(word, beginp, endp, docNum);
    }

    delete[] inBuffer;
    delete[] outBuffer;

    infile.close();
    outfile.close();
}


void Lexicon::Write() {
    ofstream outfile;

    if (FILEMODE_BIN) {  // FILEMODE == BIN
        outfile.open(LexiconPath, ofstream::binary);
    }
    else {  // FILEMODE == ASCII
        outfile.open(LexiconPath);
    }

//    for (map<string, LexiconItem>::iterator iter = _lexiconList.begin(); iter != _lexiconList.end(); ++iter) {
//        outfile << iter->first << " " << iter->second.beginp << " " << iter->second.endp << " "
//                << iter->second.docNum << endl;
//    }

    for (const auto& [word, lexItem] : _lexiconList) {
        outfile << word << " " << lexItem.beginp << " " << lexItem.endp << " "
                << lexItem.docNum << endl;
    }

    outfile.close();
}