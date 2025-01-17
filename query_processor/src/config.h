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
#define MERGED_INDEX_PATH "../data/index_no_compress.idx"
#define FINAL_INDEX_PATH "../data/index.idx"
#define LEXICON_PATH "../data/lexicon.lex"
#define PAGE_TABLE_PATH "../data/page_table.pt"

//#define INDEX_CHUNK (400 * 1024) //400KB
#define INDEX_BUFFER_SIZE (10 * 1024 * 1024)  // 10 MB
#define POST_BYTES 10  // 2 * uint_32(4) + 2 * seperator
#define AVG_WORD_BYTES 12  // estimated

#define INDEX_CHUNK_SIZE (20 * 1024 * 1024)  // 20 MB

#define POSTINGS_PER_CHUNK 64
#define BLOCK_SIZE (64 * 1024)  // 64 KB
#define MAX_META_SIZE  8192  // 8 KB
#define MAX_DOC_ID -1

#define FILE_MODE_BIN 1 // 0: ASCII, 1: BIN

#define CONJUNCTIVE 0
#define DISJUNCTIVE 1
#define DAAT_FLAG 1 // 0: TAAT, 1: DAAT

#define NUM_TOP_RESULT 15

#define DEBUG_MODE 1
#define PARSE_INDEX_FLAG 0  // whether to build intermediate index
#define PAGE_TABLE_FLAG 0  // whether write Page Table
#define MERGE_FLAG 0  // whether to merge index into one
#define LEXICON_FLAG 0  // whether to write Lexicon Structure
#define DELETE_INTERMEDIATE 0

#define LOAD_FLAG 1  // whether to load lexicon and pagetable into main memory
#define QUERY_FLAG 1
#define FRONTEND_FLAG 1  // 0: use console, 1: use Flask web interface

#endif