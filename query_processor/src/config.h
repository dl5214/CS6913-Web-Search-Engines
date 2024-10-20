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

#define DATA_SOURCE_PATH "../data/collection_debug.tsv"
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
//#define FILEMODE_ASCII 0
#define FILEMODE_BIN 1 // 0: ASCII, 1: BIN

//#define FILEMODE 1 // 0: ASCII, 1: BIN

#define DEBUG_MODE 1
#define INDEX_FLAG 1 //whether to build intermediate index
#define PAGE_TABLE_FLAG 1 //whether write Page Table
#define MERGE_FLAG 1 //whether to merge index
#define LEXICON_FLAG 1 //whether write Lexicon Structure
#define DELETE_INTERMEDIATE 0

#endif