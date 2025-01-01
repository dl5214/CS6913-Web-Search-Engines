ranking_optimizer/
│
├── cmake-build-debug/
│
├── data/
│
├── src/
│   ├── ranking-optimizer/
│       ├── build_hnsw_index.py
│       ├── query_hnsw.py
│       ├── query_bm25.py
│       ├── query_rerank.py
│       └── trec_eval.py
│
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
│
└── CMakeLists.txt



Remarks:
System1: query_bm25.py
Run C++ boost server first, then use python Flask backend to read the queries and send query to the BM25 ranking system.
System2: build_hnsw_index.py and query_hnsw.py
System3: query_rerank.py
Trec_Eval: trec_eval.py




Python dependencies:
requirements.txt provided. For this homework, we need h5py for parsing h5 files, faiss for construction and querying HNSW indices, and pytrec_eval for calculating benchmarks efficiently.




C++ Limitations and How to Run:
I used CLion IDE so the $PWD is /cmake-build-debug. Therefore, when referring to any data such as index file and dataset, the directory starts with ../ in my code.
Use CLion IDE and run with no arguments. The entry point is main.cpp. Before running, you can change the flags in config.h to execute specific part or run in a desired mode.
For web service, after running main.cpp, go to /src/web-interface and run app.py, and then you can access the web interface at 127.0.0.1:5001.