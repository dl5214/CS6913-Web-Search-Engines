//
// Created by Dong Li on 10/15/24.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <utility>
using namespace std;

//#define DATA_SOURCE_PATH "../data/collection_debug.tsv"
#define DATA_SOURCE_PATH "../data/collection.tsv"
#define INTERMEDIATE_INDEX_PATH "../data/intermediate_index/"
#define MERGED_INDEX_PATH "../data/index_no_compress.txt"
#define FINAL_INDEX_PATH "../data/index.idx"
#define LEXICON_PATH "../data/lexicon.lex"
#define PAGE_TABLE_PATH "../data/PageTable.pt"

//#define INDEX_CHUNK (400 * 1024) //400KB
#define INDEX_BUFFER_SIZE (10 * 1024 * 1024) //10MB
#define POST_BYTES 10 // 2*uint_32(4) + 2*seperator
#define AVG_WORD_BYTES 12 //estimated
#define CHAR_END_TAG '\0'

#define INDEX_CHUNK_SIZE (20 * 1024 * 1024) //20MB

#define POSTINGS_PER_BLOCK 64
#define BLOCK_SIZE (64 * 1024)  // 64 KB
#define MAX_META_SIZE 8192//8KB
#define MAX_DOC_ID -1


//#define FILEMODE_ASCII 0
#define FILE_MODE_BIN 1 // 0: ASCII, 1: BIN

//#define FILEMODE 1 // 0: ASCII, 1: BIN

#define CONJUNCTIVE 0
#define DISJUNCTIVE 1
//#define QUERY_MODE_CONJ 1  // 1: CONJUNCTIVE, 0: DISJUNCTIVE
#define NUM_TOP_RESULT 20

#define DEBUG_MODE 1
#define PARSE_INDEX_FLAG 0 //whether to build intermediate index
#define PAGE_TABLE_FLAG 0 //whether write Page Table
#define MERGE_FLAG 0 //whether to merge index
#define LEXICON_FLAG 1 //whether write Lexicon Structure
#define DELETE_INTERMEDIATE 0

#define LOAD_FLAG 0 //whether to load lexicon and pagetable into main memory
#define QUERY_FLAG 0

#endif