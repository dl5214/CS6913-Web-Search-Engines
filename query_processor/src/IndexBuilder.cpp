//
// Created by Dong Li on 10/16/24.
//
#include "IndexBuilder.h"

#include <queue>
#include <vector>
#include <tuple>
#include <sstream>


// Struct for comparing entries in the priority queue (min-heap)
struct ComparePosting {
    bool operator()(const tuple<string, ifstream*, vector<pair<uint32_t, uint32_t>>>& a,
                    const tuple<string, ifstream*, vector<pair<uint32_t, uint32_t>>>& b) const {
        return get<0>(a) > get<0>(b);  // Compare by word, ascending order
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
                catch (const invalid_argument &e) {
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

// N-way merge logic with support for binary and ASCII modes
// Helper function to parse postings from a string
vector<pair<uint32_t, uint32_t>> IndexBuilder::parsePostings(const string& postingsStr) {
    vector<pair<uint32_t, uint32_t>> postings;
    istringstream iss(postingsStr);
    uint32_t docID, freq;
    char separator;
    while (iss >> docID >> freq) {
        postings.emplace_back(docID, freq);
        iss >> separator;  // To consume the separator (',') between postings
    }
    return postings;
}

// Helper function to write merged postings to output, ensuring correct handling for single postings
void IndexBuilder::writeMergedPostings(ofstream& outfile, const string& word, const vector<pair<uint32_t, uint32_t>>& postings) {
    if (!postings.empty()) {  // Ensure that we only write non-empty posting lists
        outfile << word << ":";
        for (size_t i = 0; i < postings.size(); ++i) {
            if (i > 0) {
                outfile << ",";
            }
            outfile << postings[i].first << " " << postings[i].second;
        }
        outfile << endl;  // Write the newline to properly terminate the entry
    }
}

// Merges postings with the same docID, ensuring no postings are lost
void IndexBuilder::mergePostingLists(vector<pair<uint32_t, uint32_t>>& base, const vector<pair<uint32_t, uint32_t>>& newPostings, bool ordered) {
    if (ordered && !base.empty() && !newPostings.empty() && base.back().first < newPostings.front().first) {
        // Append the newPostings directly if they are all larger than the ones in base
        base.insert(base.end(), newPostings.begin(), newPostings.end());
        return;
    }

    // Regular merging logic if the lists are not strictly ordered
    size_t i = 0, j = 0;
    vector<pair<uint32_t, uint32_t>> merged;

    while (i < base.size() && j < newPostings.size()) {
        if (base[i].first == newPostings[j].first) {
            merged.emplace_back(base[i].first, base[i].second + newPostings[j].second);
            i++;
            j++;
        } else if (base[i].first < newPostings[j].first) {
            merged.push_back(base[i]);
            i++;
        } else {
            merged.push_back(newPostings[j]);
            j++;
        }
    }

    // Add remaining postings from both lists
    while (i < base.size()) {
        merged.push_back(base[i]);
        i++;
    }
    while (j < newPostings.size()) {
        merged.push_back(newPostings[j]);
        j++;
    }

    base.swap(merged);
}

// Multi-way merge function with improved comparison and handling of missing postings
void IndexBuilder::mergeIndex() {
    uint32_t leftIndexNum = _InvertedList.indexFileNum;  // Number of intermediate index files
    cout << "Number of intermediate index files: " << leftIndexNum << endl;

    // Priority queue for multi-way merge
    priority_queue<tuple<string, ifstream*, vector<pair<uint32_t, uint32_t>>>,
            vector<tuple<string, ifstream*, vector<pair<uint32_t, uint32_t>>>>,
            ComparePosting> pq;

    vector<ifstream> inputStreams(leftIndexNum);

    // Open all intermediate files and add the first word from each file into the priority queue
    for (uint32_t i = 0; i < leftIndexNum; ++i) {
        string path = _InvertedList.getIndexFilePath(i);
        inputStreams[i].open(path, FILEMODE_BIN ? ifstream::binary : ifstream::in);
        if (!inputStreams[i].is_open()) {
            cerr << "Error opening file: " << path << endl;
            return;
        }

        string line;
        if (getline(inputStreams[i], line)) {
            size_t colonPos = line.find(":");
            string word = line.substr(0, colonPos);
            string postingsStr = line.substr(colonPos + 1);
            vector<pair<uint32_t, uint32_t>> postings = parsePostings(postingsStr);
            pq.emplace(word, &inputStreams[i], postings);
        }
    }

    // Open output file for final merged index
    ofstream outfile;
//    string dst_mergePath = _InvertedList.getIndexFilePath();  // Output final index file path
    string dst_mergePath = MERGED_INDEX_PATH;
    if (FILEMODE_BIN) {
        outfile.open(dst_mergePath, ofstream::binary);
    } else {
        outfile.open(dst_mergePath);
    }

    // Process the priority queue (min-heap)
    while (!pq.empty()) {
        auto [word, infile, postings] = pq.top();
        pq.pop();

        // Merge postings from other files with the same word
        while (!pq.empty() && get<0>(pq.top()) == word) {
            auto [nextWord, nextInfile, nextPostings] = pq.top();
            pq.pop();
            mergePostingLists(postings, nextPostings, true);  // Merge postings for the same word

            // Read next entry from that file and push it into the queue
            string line;
            if (getline(*nextInfile, line)) {
                size_t colonPos = line.find(":");
                string newWord = line.substr(0, colonPos);
                string postingsStr = line.substr(colonPos + 1);
                vector<pair<uint32_t, uint32_t>> newPostings = parsePostings(postingsStr);
                pq.emplace(newWord, nextInfile, newPostings);
            }
        }

        // Write merged postings to output file
        writeMergedPostings(outfile, word, postings);

        // Read next entry from the current file and push it into the queue
        string line;
        if (getline(*infile, line)) {
            size_t colonPos = line.find(":");
            string newWord = line.substr(0, colonPos);
            string postingsStr = line.substr(colonPos + 1);
            vector<pair<uint32_t, uint32_t>> newPostings = parsePostings(postingsStr);
            pq.emplace(newWord, infile, newPostings);
        }
    }

    // Close all input streams
    for (auto& inputStream : inputStreams) {
        if (inputStream.is_open()) {
            inputStream.close();
        }
    }

    outfile.close();  // Close the output file
}

// Writes the page table to disk
void IndexBuilder::WritePageTable() {
    _PageTable.Write();
}

// Writes the lexicon to disk
void IndexBuilder::WriteLexicon() {
    _Lexicon.Write();
}