## Files
.
├── web_crawler.py (Python program source)
├── requirements.txt (python lib need to install)
├── data
│   ├── explain.txt
│   ├── readme.txt
│   ├── crawler_log_1.txt
│   ├── crawler_log_2.txt
│   ├── seeds_1.txt
│   └── seeds_2.txt

## Requirements
Beautiful soup (bs4) for parsing. Also available:
$ pip install -r requirements.txt

## How To Run
Use an IDE with requirements (bs4) installed, or use command-line - No parameters accepted through the shell. However, in __name__ == '__main__', the number of selected seeds (num_seeds), the maximum trials of downloading and crawling (max_pages), and the number of threads (num_threads) can be adjusted. Hopefully using 20 seeds and 20-30 threads, crawling 1000 pages takes 3 mins and crawling 12000 pages takes 30 mins.