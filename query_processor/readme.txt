query_processor/
│
├── cmake-build-debug/
│
├── CMakeLists.txt
│
├── data/
│   ├── intermediate_index/
│   ├── collection.tsv
│   ├── BIN_index.idx
│   ├── BIN_lexicon.lex
│   ├── BIN_page_table.pt
│   └── index_no_compress.idx
│
├── src/
│   ├── web-interface/
│       ├── static/
│           ├── css/
│               ├── result.css
│               ├── search.css
│       ├── templates/
│           ├── result.html
│           ├── search.html
│       ├── app.py
│
│   ├── config.h
│   ├── IndexBuilder.cpp
│   ├── IndexBuilder.h
│   ├── InvertedList.cpp
│   ├── InvertedList.h
│   ├── Lexicon.cpp
│   ├── Lexicon.h
│   ├── main.cpp
│   ├── PageTable.cpp
│   ├── PageTable.h
│   ├── QueryProcessor.cpp
│   ├── QueryProcessor.h
│   ├── SearchResult.cpp
│   └── SearchResult.h
└── CMakeLists.txt


Running Limitations: 
I used CLion IDE so the $PWD is /cmake-build-debug. Therefore, when referring to any data such as index file and dataset, the directory starts with ../ in my code.


How to run:
Use CLion IDE and run with no arguments. The entry point is main.cpp. Before running, you can change the flags in config.h to execute specific part or run in a desired mode.
For web service, after running main.cpp, go to /src/web-interface and run app.py, and then you can access the web interface at 127.0.0.1:5001.