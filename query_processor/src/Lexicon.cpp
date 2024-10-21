//
// Created by Dong Li on 10/16/24.
//
#include "Lexicon.h"
using namespace std;


// Function to encode a uint32 value using Varbyte encoding
vector<uint8_t> varbyteEncode(uint32_t value) {
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
uint32_t varbyteDecode(const vector<uint8_t> &encoded) {
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


// Default constructor and destructor for LexiconItem
LexiconItem::LexiconItem() = default;
LexiconItem::~LexiconItem() = default;


// Update the LexiconItem fields
void LexiconItem::update(uint32_t beginPos, uint32_t endPos, uint32_t docNum, uint32_t blockNum) {
    this->beginPos = beginPos;
    this->endPos = endPos;
    this->docNum = docNum;
    this->blockNum = blockNum;
}


// Constructor for the Lexicon class, setting paths based on file mode (binary or ASCII)
Lexicon::Lexicon() {
    if (FILE_MODE_BIN) {  // FILEMODE == BIN
        _lexiconPath = string(LEXICON_PATH).substr(0, string(LEXICON_PATH).find_last_of('/')) + "/BIN_" + string(LEXICON_PATH).substr(string(LEXICON_PATH).find_last_of('/') + 1);
        indexPath = string(FINAL_INDEX_PATH).substr(0, string(FINAL_INDEX_PATH).find_last_of('/')) + "/BIN_" + string(FINAL_INDEX_PATH).substr(string(FINAL_INDEX_PATH).find_last_of('/') + 1);
    }
    else {  // FILEMODE == ASCII
        _lexiconPath = string(LEXICON_PATH).substr(0, string(LEXICON_PATH).find_last_of('/')) + "/ASCII_" + string(LEXICON_PATH).substr(string(LEXICON_PATH).find_last_of('/') + 1);
        indexPath = string(FINAL_INDEX_PATH).substr(0, string(FINAL_INDEX_PATH).find_last_of('/')) + "/ASCII_" + string(FINAL_INDEX_PATH).substr(string(FINAL_INDEX_PATH).find_last_of('/') + 1);
    }
}


Lexicon::~Lexicon() {  // Destructor - currently no dynamic memory to free
}


// Calculate the number of documents based on the index word list (docID and frequency pairs)
uint32_t Lexicon::_getPostingDocNum(string idxWordList) {
    uint32_t spaceNum = 0;
    for (size_t i = 0; i < idxWordList.length(); i++) {
        if (idxWordList[i] == ' ') {
            spaceNum += 1;
        }
    }
    return (spaceNum + 1) / 2;  // Each word will have one space between docID and freq
}


// Insert a new entry into the lexicon map
bool Lexicon::insert(string word, uint32_t beginPos, uint32_t endPos, uint32_t docNum, uint32_t blockNum) {
    if (word.empty()) {
        return false;  // Avoid empty words
    }
    LexiconItem lexItem;
    lexItem.update(beginPos, endPos, docNum, blockNum);  // Update lexicon item details
    lexiconList[word] = lexItem;  // Insert into the lexicon map
    return true;
}


// Write encoded blocks of postings to the index file and return the number of blocks
uint32_t Lexicon::_writeBlocks(string term, uint32_t &docNum, string arr, ofstream &outfile) {
    // Initialize position and lists for storing encoded docIDs and frequencies
    size_t beginPos = 0;
    vector<vector<uint8_t>> docIdList;  // Encoded docID list
    vector<vector<uint8_t>> freqList;  // Encoded frequency list
    vector<uint32_t> lastDocIdMetadata;  // Last docID of each block
    vector<uint32_t> docIdBlockSizeMetadata;  // Sizes of docID blocks
    vector<uint32_t> freqBlockSizeMetadata;  // Sizes of frequency blocks

    uint32_t freq, docID;
    bool isOk = true;
    uint32_t prevdocID = 0;

    // Loop through the input postings string to extract and encode docIDs and frequencies
    while (isOk) {
        size_t space, comma;
        try {
            // Extract docID and frequency from the postings list
            space = arr.find(" ", beginPos);
            comma = arr.find(",", space);
            docID = stoi(arr.substr(beginPos, space - beginPos));  // Extract docID

            if (comma != string::npos) {
                freq = stoi(arr.substr(space + 1, comma - space - 1));  // Extract frequency
            }
            else {
                freq = stoi(arr.substr(space + 1));  // Last frequency in the string
                isOk = false;  // No more postings to process
            }

            // Encode docID as a difference (delta) from the previous docID using VarByte encoding
            if (docID < prevdocID) {
                cout << "nooo" << endl;
            }

            vector<uint8_t> endocID = varbyteEncode(docID - prevdocID);
            vector<uint8_t> enFreq = varbyteEncode(freq);  // Encode frequency
            docIdList.push_back(endocID);  // Store encoded docID
            freqList.push_back(enFreq);  // Store encoded frequency
            prevdocID = docID;  // Update previous docID

            // Store metadata for each block if it's full or we are at the end of the postings
            if (docIdList.size() % POSTINGS_PER_BLOCK == 0 || !isOk) {
                lastDocIdMetadata.push_back(docID);  // Store last docID of this block
                prevdocID = 0;  // Reset for the next block
            }
        }
        catch (...) {
            cout << arr.substr(beginPos, space - beginPos) << " " << arr.substr(space + 1, comma - space - 1);
            cout << arr.substr(beginPos, space - beginPos).length() << endl;
            cout << arr.substr(space + 1, comma - space - 1).length() << endl;
            continue;
        }
        beginPos = comma + 1;  // Move to the next docID in the postings list
    }

    docNum = docIdList.size();  // Total number of postings (docID-frequency pairs)

    // Calculate sizes for the metadata (docID block size and frequency block size)
    uint32_t docIDsize = 0, freqSize = 0;
    for (int i = 0; i < docIdList.size(); i++) {
        docIDsize += docIdList[i].size();  // Sum size of encoded docIDs
        freqSize += freqList[i].size();  // Sum size of encoded frequencies

        // If block is full, store the block sizes
        if ((i + 1) % POSTINGS_PER_BLOCK == 0 || i == docIdList.size() - 1) {
            docIdBlockSizeMetadata.push_back(docIDsize);  // Store docID block size
            freqBlockSizeMetadata.push_back(freqSize);  // Store frequency block size
            docIDsize = 0;
            freqSize = 0;  // Reset for the next block
        }
    }

    // Write the blocks of postings and their metadata to the output file
    int blocks_num = (int)lastDocIdMetadata.size();  // Total number of blocks
    int pblocks = 0;  // Pointer to track blocks written so far

    uint32_t blockNum = 0;  // Total number of blocks

    // Pack as many blocks as possible into the current buffer size (BLOCK_SIZE)
    while (pblocks < blocks_num) {
        uint32_t nowbyte = 4;  // Start with 4 bytes for block length
        int pbeginblock = pblocks;

        // Pack as many blocks as possible into the current buffer size (BLOCK_SIZE)
        while (nowbyte <= BLOCK_SIZE && pblocks < blocks_num) {
            uint32_t newsize = 4 * 3 + docIdBlockSizeMetadata[pblocks] + freqBlockSizeMetadata[pblocks];
            if (nowbyte + newsize > BLOCK_SIZE) {
                break;  // Stop if the new block doesn't fit
            }
            pblocks += 1;
            nowbyte += newsize;  // Update current byte count
        }

        // Write metadata for the current block (docIDs, block sizes)
        blockNum += 1;
        uint32_t block_len = pblocks - pbeginblock;
        outfile.write(reinterpret_cast<const char *>(&block_len), sizeof(uint32_t));  // Write block length

        // Write last docID, docID block sizes, and frequency block sizes for each block
        for (int i = pbeginblock; i < pblocks; i++) {
            outfile.write(reinterpret_cast<const char *>(&lastDocIdMetadata[i]), sizeof(uint32_t));
            // if(term=="0") cout << metadata_last_docID[i] << " ";
        }
        // if(term=="0") cout << endl;
        for (int i = pbeginblock; i < pblocks; i++) {
            outfile.write(reinterpret_cast<const char *>(&docIdBlockSizeMetadata[i]), sizeof(uint32_t));
            // if(term=="0") cout<<metadata_docID_block_sizes[i]<<" ";
        }
        // if(term=="0") cout << endl;
        for (int i = pbeginblock; i < pblocks; i++) {
            outfile.write(reinterpret_cast<const char *>(&freqBlockSizeMetadata[i]), sizeof(uint32_t));
            // if(term=="0") cout << metadata_freq_block_sizes[i] << " ";
        }

        // Write the actual postings (docID and frequency pairs)
        for (int i = pbeginblock; i < pblocks; i++) {
            // Write encoded docIDs for each block
            for (int j = i * POSTINGS_PER_BLOCK; j < (i + 1) * POSTINGS_PER_BLOCK && j < docIdList.size(); j++) {
                vector<uint8_t> endocID = docIdList[j];
                for (int k = 0; k < endocID.size(); k++) {
                    // Write each byte
                    outfile.write(reinterpret_cast<const char *>(&endocID[k]), sizeof(uint8_t));
                }
            }
            // Write encoded frequencies for each block
            for (int j = i * POSTINGS_PER_BLOCK; j < (i + 1) * POSTINGS_PER_BLOCK && j < docIdList.size(); j++) {
                vector<uint8_t> enFreq = freqList[j];
                for (int k = 0; k < enFreq.size(); k++) {
                    // Write each byte
                    outfile.write(reinterpret_cast<const char *>(&enFreq[k]), sizeof(uint8_t));
                }
            }
        }
        // file left blocks in 0
        // uint8_t zero = 0;
        // for (int i = 0; i < BLOCK_SIZE - nowbyte; i++)
        // {
        //     outfile.write(reinterpret_cast<const char *>(&zero), sizeof(uint8_t));
        // }
    }

    return blockNum;  // Return the total number of blocks written
}


// Build function that processes a single merged file
void Lexicon::build(const string& mergedIndexPath) {
    ifstream infile;
    ofstream outfile;
    // Open the merged index file for reading and the final index file for writing
    infile.open(mergedIndexPath, FILE_MODE_BIN ? ifstream::binary : ifstream::in);
    outfile.open(indexPath, FILE_MODE_BIN ? ifstream::binary : ifstream::in);
    uint32_t beginPos = 0;  // Variables to track the positions of the postings in the index file
    uint32_t endPos;  // Variables to track the positions of the postings in the index file
    string line;

//    if (DEBUG_MODE) {
//        cout << "indexPath: " << _indexPath << endl;
//        // Check if the input file opened correctly
//        if (!infile.is_open()) {
//            cerr << "Error: Failed to open merged index file: " << mergedIndexPath << endl;
//            return;
//        } else {
//            cout << "Merged index file opened successfully." << endl;
//        }
//
//        // Check if the output file opened correctly
//        if (!outfile.is_open()) {
//            cerr << "Error: Failed to open final index file: " << indexPath << endl;
//            return;
//        } else {
//            cout << "Final index file opened successfully." << endl;
//        }
//    }

    // Read the merged index file line by line
    while (!infile.eof()) {
        getline(infile, line);  // Read each line (term and postings)
        string word = line.substr(0, line.find(":"));   // Extract the term (before the colon)
        if (!word.length()) {
            break;  // If no word is found, exit the loop
        }
        string arr = line.substr(line.find(":") + 1);   // Extract the postings list (after the colon)

        uint32_t docNum;
        // Write the blocks of postings for this word and get the number of blocks
        uint32_t blockNum = _writeBlocks(word, docNum, arr, outfile);

        // Update the lexicon with the term's metadata (begin/end positions, docNum, blockNum)
        endPos = outfile.tellp();   // Get the current position (end of the postings for this word)
        if (DEBUG_MODE and blockNum > 1) {
            cout << word << " " << beginPos << " " << endPos << " " << docNum << " " << blockNum << endl;
        }
        insert(word, beginPos, endPos, docNum, blockNum);   // Insert the term and its metadata into the lexicon
        beginPos = endPos;  // Update the starting position for the next term
    }

    // Close the input and output files    infile.close();
    outfile.close();
}


// Function to write the lexicon to disk
void Lexicon::write() {
    ofstream outfile;

    // Open the lexicon file for writing based on the file mode (binary or ASCII)
    if (FILE_MODE_BIN) {  // FILEMODE == BIN
        outfile.open(_lexiconPath, ofstream::binary);
    }
    else {  // FILEMODE == ASCII
        outfile.open(_lexiconPath);
    }

    // Write each term and its metadata (begin/end positions, docNum) from the lexicon map
    for (const auto& [word, lexItem] : lexiconList) {
        outfile << word << " " << lexItem.beginPos << " " << lexItem.endPos << " "
                << lexItem.docNum << " " << lexItem.blockNum << endl;
    }

    outfile.close();    // Close the lexicon file
}


void Lexicon::load() {
    ifstream infile;
    if (DEBUG_MODE) {
        cout << "lexicon file path: " << _lexiconPath << endl;
    }
    if (FILE_MODE_BIN) {
        infile.open(_lexiconPath, ifstream::binary);
    }
    else {
        infile.open(_lexiconPath);
    }
    if (!infile) {
        cout << "can not read " << _lexiconPath << endl;
        exit(0);
    }
    lexiconList.clear();
//    while (!infile.eof()) {
//        string term;
//        LexiconItem lexItem;
//        infile >> term;
//        infile >> lexItem.beginPos >> lexItem.endPos >> lexItem.docNum >> lexItem.blockNum;
//        lexiconList[term] = lexItem;
//    }
    string term;
    while (infile >> term) {
        LexiconItem lexItem;
        if (DEBUG_MODE) {
            cout << "read lexiconItem: " << lexItem.beginPos << " " << lexItem.endPos << " "
            << lexItem.docNum << " " << lexItem.blockNum << endl;
        }
        infile >> lexItem.beginPos >> lexItem.endPos >> lexItem.docNum >> lexItem.blockNum;
        lexiconList[term] = lexItem;
    }
    cout << "There are " << lexiconList.size() << " words in Lexicon Structure" << endl;
}