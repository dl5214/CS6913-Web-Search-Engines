//
// Created by Dong Li on 10/16/24.
//
#include "IndexBuilder.h"

#include <queue>
#include <vector>
#include <tuple>

// Structure to compare two words in the priority queue
struct Compare {
    bool operator()(const tuple<string, uint32_t, ifstream*>& a, const tuple<string, uint32_t, ifstream*>& b) {
        return get<0>(a) > get<0>(b);  // Compare the word strings (lexicographically)
    }
};

// Prints the sorted posting list with word counts
void SortedPosting::print() {
//    for (map<string,uint32_t>::iterator it=_sortedList.begin(); it!=_sortedList.end(); ++it){
//        cout << "(" << it->first << "," << it->second << ")\n";
//    }
    // Using C++11 structured bindings and range-based loop to print more cleanly
    for (const auto& [word, count] : _sortedList) {
        cout << "(" << word << "," << count << ")\n";
    }
}

// Constructor for IndexBuilder class
IndexBuilder::IndexBuilder() {
}

// Destructor for IndexBuilder class
IndexBuilder::~IndexBuilder() {
}

// Extracts a substring from 'org' between 'bstr' and 'estr'
string IndexBuilder::extractContent(string org, string bstr, string estr) {
    size_t begin_tag_len = bstr.length();

    // Find the positions of the start and end tags
    size_t start_pos = org.find(bstr);
    size_t end_pos = org.find(estr);

    // Return the substring between the start and end tags
    return org.substr(start_pos + begin_tag_len, end_pos - start_pos - begin_tag_len);
}

// Gets the first line from the string 'str' (up to the newline character)
string IndexBuilder::getFirstLine(string str) {
    size_t endpos = str.find("\n");
    return str.substr(0, endpos);
}

// Calculates the frequency of each word in the 'text' and stores it in the inverted list for the given 'docID'
uint32_t IndexBuilder::calcWordFreq(string text, uint32_t docID) {
    // Define separator characters used to split words
    string sep = " :;,.\t\v\r\n\f[]{}()<>+-=*&^%$#@!~`´\'\"|\\/?·\"：“”"
                 "∂æâãäåàªÃÅÂÄÃÊËÉïîìÏÌóûüÙÛÚñÑÐ¸¶Øø§≠°º®©¤¯½¼¾«»±£¢¹²³¬¦¨¿_";

    string word;
    SortedPosting sortedPosting;

    // Loop through the document content character by character
    for (size_t i = 0; i < text.length(); i++) {
        // If the current character is not a separator, add it to the word
        if (sep.find(text[i]) == string::npos) {
            word += tolower(text[i]);  // Normalize case to lowercase
        }
            // If the current character is a separator, process the word
        else {
            if (!word.empty()) {
                // Keep only words that start with an English letter or a digit
                if (isalnum(word[0])) {
                    // Check if the word is already in the sorted list
                    if (!sortedPosting._sortedList.count(word)) {
                        // If not, add the word with a count of 1
                        sortedPosting._sortedList[word] = 1;
                    }
                    else {
                        // If it exists, increment the word count
                        sortedPosting._sortedList[word] += 1;
                    }
                }
            }
            // Clear the word to start fresh for the next one
            word.clear();
        }
    }

    // Ensure the last word is also processed (in case the text doesn't end with a separator)
    if (!word.empty() && isalnum(word[0])) {
        if (!sortedPosting._sortedList.count(word)) {
            sortedPosting._sortedList[word] = 1;
        } else {
            sortedPosting._sortedList[word] += 1;
        }
    }

    if (DEBUG_MODE & 0) {
        sortedPosting.print();
    }

    // Insert the word frequencies into the inverted list
    for (const auto& [word, count] : sortedPosting._sortedList) {
        _InvertedList.Insert(word, docID, count);
    }

    // Return the number of unique words in the document
    return sortedPosting._sortedList.size();
}

void IndexBuilder::read_data(const char *filepath) {
    ifstream infile(filepath);  // Open the uncompressed TSV file
    if (!infile.is_open()) {
        cerr << "Error opening file: " << filepath << endl;
        return;
    }

    const size_t BUFFER_SIZE = INDEX_BUFFER_SIZE;  // Define buffer size
    cout << "Index Buffer Size: " << BUFFER_SIZE / 1024 << " KB" << endl;

    char *buffer = new char[BUFFER_SIZE];  // Create a large buffer
    infile.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);  // Set custom buffer for efficient I/O

    string docContent;
    int debugReadCount = 0;  // Counter to limit the amount of debug output

    // Read the file in chunks and process the content
    while (getline(infile, docContent)) {
        // Output the first 1000 characters for debugging purposes
        if (DEBUG_MODE && debugReadCount == 0) {
            cout << "First 1000 characters of the file:\n";
            cout << docContent.substr(0, 1000) << endl;
            debugReadCount++;
        }

        // Assuming the format: docID <tab> content
        size_t tab_pos = docContent.find("\t");

        if (tab_pos != string::npos) {
            // Extract the docID part and validate it
            string docID_str = docContent.substr(0, tab_pos);
            docID_str.erase(0, docID_str.find_first_not_of(' '));  // Trim leading spaces
            docID_str.erase(docID_str.find_last_not_of(' ') + 1);  // Trim trailing spaces

            // Ensure that the docID is numeric
            if (!docID_str.empty() && all_of(docID_str.begin(), docID_str.end(), ::isdigit)) {
                try {
                    Document doc;
                    doc.docID = stoi(docID_str);  // Parse the docID as an integer
                    string fullText = docContent.substr(tab_pos + 1);  // Extract the content after the tab

                    // Continue processing if docID is valid
                    if (doc.docID >= 0) {
                        doc.dataLen = fullText.length();  // Calculate the length of the document
                        doc.wordnums = calcWordFreq(fullText, doc.docID);  // Calculate word frequency
                        _PageTable.add(doc);  // Add the document to the page table

                        if (DEBUG_MODE && doc.docID % 10000 == 0) {
                            cout << "Processing DocID: " << doc.docID << endl;
                        }
                    }
                }
                catch (const std::invalid_argument &e) {
                    cerr << "Error parsing docID: " << e.what() << " in line: " << docContent << endl;
                    continue;
                }
            } else {
                cerr << "Invalid or non-numeric docID in line: " << docContent << endl;  // Handle invalid docID
            }
        } else {
            cerr << "Invalid format (no tab separator) in line: " << docContent << endl;  // Handle invalid format
        }
    }

    // Write the inverted list to disk if it contains any entries
    if (!_InvertedList.HashWord.empty()) {
        _InvertedList.Write();
        _InvertedList.Clear();
    }

    // Optionally print the page table if debugging
    if (DEBUG_MODE & 0) {
        _PageTable.print();  // Print the page table
    }

    // Write the page table to the disk if necessary
    if (PAGE_TABLE_FLAG & INDEX_FLAG) {
        clock_t write_page_begin = clock();
        WritePageTable();
        clock_t write_page_end = clock();
        clock_t write_page_time = write_page_end - write_page_begin;
        cout << "Writing Page Table Takes " << double(write_page_time) / 1000000 << "Seconds" << endl;
    }

    // Clean up
    delete[] buffer;
    infile.close();  // Close the file stream
}

// Merges two inverted index files (fileIndex1 and fileIndex2) into one
void IndexBuilder::mergeIndex(uint32_t fileIndex1, uint32_t fileIndex2) {
    string dst_mergePath = _InvertedList.getIndexFilePath();  // Destination file path for merged index
    ifstream infile1;
    ifstream infile2;
    ofstream outfile;
    string file1 = _InvertedList.getIndexFilePath(fileIndex1);
    string file2 = _InvertedList.getIndexFilePath(fileIndex2);

    // Open files in binary or text mode depending on the file mode
    if (FILEMODE_BIN) {  // FILEMODE == BIN
        infile1.open(file1, ifstream::binary);
        infile2.open(file2, ifstream::binary);
        outfile.open(dst_mergePath, ofstream::binary);
    }
    else {  // FILEMODE == ASCII)
        infile1.open(file1);
        infile2.open(file2);
        outfile.open(dst_mergePath);
    }

    string line1, line2;
    string word1, word2;
    while (infile1 && infile2) {
        // If both words are empty, read a line from each file
        if (word1.empty() && word2.empty()) {
            getline(infile1, line1);
            getline(infile2, line2);
            word1 = line1.substr(0, line1.find(":"));
            word2 = line2.substr(0, line2.find(":"));
        }
        // If the words are the same, merge the two inverted lists
        if (word1.compare(word2) == 0) {
            // merge word1 and word2
            string arr1 = line1.substr(line1.find(":") + 1);
            string arr2 = line2.substr(line2.find(":") + 1);
            string docID1 = arr1.substr(0, arr1.find(" "));
            string docID2 = arr2.substr(0, arr2.find(" "));
            if (docID1 < docID2) {
                outfile << word1 << ":" << arr1 << "," << arr2 << endl;
            }
            else {
                outfile << word1 << ":" << arr2 << "," << arr1 << endl;
            }

            // update line1 and line2
            getline(infile1, line1);
            getline(infile2, line2);
            word1 = line1.substr(0, line1.find(":"));
            word2 = line2.substr(0, line2.find(":"));
        }
        // If word1 is smaller, write word1's data and move to the next line
        else if (word1.compare(word2) < 0) {
            // write line1 only
            outfile << line1 << endl;
            // update line1
            getline(infile1, line1);
            word1 = line1.substr(0, line1.find(":"));
        }
        // If word2 is smaller, write word2's data and move to the next line
        else {
            // write line2 only
            outfile << line2 << endl;
            // update line2
            getline(infile2, line2);
            word2 = line2.substr(0, line2.find(":"));
        }
    }

    // Write any remaining data from either file
    if (infile1) {
        outfile << line1 << endl;
    }
    if (infile2) {
        outfile << line2 << endl;
    }

    while (infile1) {
        getline(infile1, line1);
        outfile << line1 << endl;
    }
    while (infile2) {
        getline(infile2, line2);
        outfile << line2 << endl;
    }
    infile1.close();
    infile2.close();
    outfile.close();
}

// Merges all index files into a single index
void IndexBuilder::mergeIndexToOne() {
    uint32_t mergedIndexNum = 0;
    uint32_t leftIndexNum;
    cout<< "Number of intermediate index files: " << _InvertedList.indexFileNum << endl;  // Print the number of index files
    // Continue merging index files until only two remain
    while ((leftIndexNum = _InvertedList.indexFileNum - mergedIndexNum) > 2) {
        mergeIndex(mergedIndexNum, mergedIndexNum + 1);  // Merge pairs of index files
        mergedIndexNum += 2;  // Update the number of merged files
    }
    // If there are two remaining index files, build the final lexicon
    if (leftIndexNum == 2) {
        string path1 = _InvertedList.getIndexFilePath(mergedIndexNum);
        string path2 = _InvertedList.getIndexFilePath(mergedIndexNum + 1);
        _Lexicon.Build(path1, path2);  // Build lexicon from the last two files
    }
}

// Writes the page table to disk
void IndexBuilder::WritePageTable() {
    _PageTable.Write();
}

// Writes the lexicon to disk
void IndexBuilder::WriteLexicon() {
    _Lexicon.Write();
}