#include "IndexBuilder.h"
#include "QueryProcessor.h"
#include <iomanip>
using namespace std;

int main() {

//    if (DEBUG_MODE) {
//        char cwd[1024];
//        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
//            std::cout << "Current working directory: " << cwd << std::endl;
//        }
//        ifstream infile("/Users/dl5214/CS6913-Web-Search-Engines/query_processor/data/index_no_compress.txt");
//        if (!infile.is_open()) {
//            cerr << "Error opening file!" << endl;
//        } else {
//            cout << "File opened successfully!" << endl;
//        }
//    }

    IndexBuilder index_builder;
    QueryProcessor query_processor;

    if (PARSE_INDEX_FLAG) {
        cout << "Building postings and intermediate inverted index. Timing started... " << endl;
        clock_t index_start = clock();
        index_builder.readData(DATA_SOURCE_PATH);
        clock_t index_end = clock();
        double index_time = double(index_end - index_start) / 1000000;
        cout << "Building postings and intermediate inverted index DONE." << endl;
        cout << "Time elapsed: " << fixed << setprecision(2) << index_time << " Seconds" << endl;
    }

    if (MERGE_FLAG) {
        cout << "Merging inverted index. Timing started..." << endl;
        clock_t merge_start = clock();
        index_builder.mergeIndex();
        clock_t merge_end = clock();
        double merge_time = double(merge_end - merge_start) / 1000000;
        cout << "Merging inverted index DONE." << endl;
        cout << "Time elapsed: " << fixed << setprecision(2) << merge_time << " Seconds" << endl;
    }

    if (LEXICON_FLAG) {
        cout << "Building Lexicon and Final Compressed Index. Timing started..." << endl;
        clock_t lexicon_build_start = clock();
        index_builder.lexicon.build(MERGED_INDEX_PATH);  // Pass the final merged index file
        index_builder.writeLexicon();
        clock_t lexicon_build_end = clock();
        double lexicon_build_time = double(lexicon_build_end - lexicon_build_start) / 1000000;
        cout << "Building Lexicon and Final Index DONE." << endl;
        cout << "Time elapsed: " << fixed << setprecision(2) << lexicon_build_time << " Seconds" << endl;
    }

    if(LOAD_FLAG) {
        cout << "Loading PageTable and Lexicon Into Main Memory. Timing Started..." << endl;
        clock_t reload_begin = clock();
        query_processor.pageTable.load();
        query_processor.lexicon.load();
        clock_t reload_end = clock();
        clock_t reload_time = reload_end - reload_begin;
        double load_time = double(reload_time)/1000000;
        cout << "Loading PageTable and Lexicon Done." << endl;
        cout << "Time elapsed: "<< fixed << setprecision(2) << load_time <<" Seconds"<<endl;
    }

    if (QUERY_FLAG) {
        query_processor.queryLoop();
    }
}