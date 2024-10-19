#include "IndexBuilder.h"
#include <iomanip>
using namespace std;

int main() {
    IndexBuilder index_builder;
    if (INDEX_FLAG) {
        cout << "Building postings and intermediate inverted index. Timing started... " << endl;
        clock_t index_start = clock();
        index_builder.read_data(DATA_SOURCE_PATH);
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
        cout << "Writing lexicon structure. Timing started..." << endl;
        clock_t lexicon_start = clock();
        index_builder.WriteLexicon();
        clock_t lexicon_end = clock();
        double lexicon_time = double(lexicon_end - lexicon_start) / 1000000;
        cout << "Writing lexicon structure DONE." << endl;
        cout << "Time elapsed: " << fixed << setprecision(2) << lexicon_time << " Seconds" << endl;
    }
}