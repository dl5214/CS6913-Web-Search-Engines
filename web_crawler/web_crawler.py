# from queue import Queue
# import heapq
from queue import PriorityQueue
from urllib.parse import urlparse, urljoin
from urllib.parse import urlsplit, urlunsplit
from urllib.robotparser import RobotFileParser
import requests
from datetime import datetime
from html.parser import HTMLParser
import random
import time
from bs4 import BeautifulSoup
import signal
import threading


debug_mode = False


# Signal handler for timeout
def handler(signum, frame):
    raise TimeoutException()


# Set the global timeout handler (this can be outside of WebCrawler class)
signal.signal(signal.SIGALRM, handler)


class TimeoutException(Exception):
    pass


# Deprecated: Use Beautiful Soup instead
class LinkExtractor(HTMLParser):
    """
    Extract 'href' links from <a> tags, inheriting HTMLParser
    """

    def __init__(self):
        super().__init__()
        self.extracted_links = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        """
        Override: Only extract 'href' from <a> tags
        Override: Extract 'href' and 'src' attributes from multiple tags.
        """
        # Check for Override
        assert hasattr(super(), 'handle_starttag'), "The method handle_starttag was not found in the parent class."
        if tag == 'a':
            for (key, value) in attrs:
                if key == 'href':
                    print(f"Found link: {value}")
                    self.extracted_links.append(value)


class WebCrawler:
    """
    A simple web crawler that performs a breadth-first search (BFS) to crawl pages within the '.nz' domain.
    """

    def __init__(self, seeds):
        """
        Initializes the WebCrawler with the necessary attributes.
        :param seeds: (list) A list of seed URLs to start crawling from.
        """
        # Dictionary structure for visited pages
        # key: normalized URL, value: page size (in bytes), access time, HTTP return code, BFS depth
        self.url_visited = {}
        # Dictionary to track domain enqueue times (key: domain, value: enqueue count)
        self.domain_enqueued = {}
        # Initialize priority queue
        # self.to_visit = []
        self.to_visit = PriorityQueue()
        # Do not visit the extensions in the blacklist
        self.blacklist_extensions = {
            '.jpg', '.jpeg', '.png', '.gif', '.bmp', '.svg',  # Image files
            '.pdf', '.doc', '.docx', '.xls', '.xlsx', '.ppt',  # Document files
            '.zip', '.rar', '.tar', '.gz', '.7z', '.bz2',  # Compressed files
            '.mp3', '.wav', '.ogg', '.aac', '.flac',  # Audio files
            '.mp4', '.avi', '.mov', '.mkv', '.webm',  # Video files
            '.exe', '.bin', '.dll', '.msi', '.sh', '.iso',  # Executable files
            '.css', '.js', '.json', '.xml', '.rss',  # Web resources
            '.ico', '.ttf', '.woff', '.woff2', '.eot',  # Font and icon files
            '.swf', '.flv', '.fla',  # Flash files
            '.php', '.aspx', '.cgi', '.py', '.pl', '.rb', '.jsp',  # Server-side scripts
            '.dat', '.log', '.bak'  # Miscellaneous
        }
        # object TLD
        self.domain = '.nz'
        self.url_counter = 0  # The number of URLs visited
        self.queue_index = 1  # Counter for ordering within the priority queue
        self.robots_parsers = {}  # Store robot parsers for each domain
        self.seed_urls = set()  # For logger to record seed pages

        self.visited_lock = threading.Lock()
        self.enqueue_lock = threading.Lock()
        self.counter_lock = threading.Lock()

        # Insert seed URLs into the priority queue
        for url in seeds:
            # self.to_visit.put((url, 0))
            domain = self.get_domain(url)
            priority = self.get_priority(domain)
            # heapq.heappush(self.to_visit, (priority, self.queue_index, (url, 0)))  # Use priority, queue_index, (url, depth)
            self.to_visit.put((priority, self.queue_index, (url, 0)))  # Use priority, queue_index, (url, depth)
            self.queue_index += 1
            self.update_domain_enqueued(domain)  # Update domain enqueue count for seeds
            self.seed_urls.add(url)  # For logger to record seed pages

    def is_valid_url(self, url):
        """
        Checks if the given URL is valid and should be crawled.
        :param url: (str) The URL to be checked.
        :return: (bool) True if the URL is valid and within the target domain, False otherwise.
        """
        # Parse url and get its components
        parsed_url = urlparse(url)
        # Check if URL is within the '.nz' domain
        is_within_domain = parsed_url.netloc.endswith(self.domain)
        # Check if the URL has an extension that is blacklisted
        is_in_blacklist = any(url.lower().endswith(ext) for ext in self.blacklist_extensions)
        return is_within_domain and not is_in_blacklist

    def get_domain(self, url):
        """
        Extracts and returns the domain from the given URL.
        """
        parsed_url = urlparse(url)
        # if debug_mode:
        #     print(f"Domain: {parsed_url.netloc}")
        return parsed_url.netloc

    def update_domain_enqueued(self, domain):
        """
        Update the count of how many times a domain has been enqueued.
        """
        with self.enqueue_lock:
            if domain not in self.domain_enqueued:
                self.domain_enqueued[domain] = 1
            else:
                self.domain_enqueued[domain] += 1

    def get_priority(self, domain):
        """
        Calculate priority based on how many times the domain has been enqueued.
        """
        # count = self.domain_enqueued.get(domain, 0)
        with self.enqueue_lock:
            count = self.domain_enqueued.get(domain, 0)
        if count == 0:
            return 1  # Highest priority
        elif count == 1:
            return 2
        elif count == 2:
            return 3
        elif count <= 5:
            return 4
        elif count <= 10:
            return 5
        elif count <= 100:
            return 6
        elif count <= 1000:
            return 7
        elif count <= 10000:
            return 8
        elif count <= 1000000:
            return 9
        else:
            return 10  # Lowest priority

    def get_robots_parser(self, base_url):
        """
        Retrieve and parse 'robots.txt' to respect Robot Exclusion Protocol
        :param base_url: str: The base URL of the website
        :return: RobotFileParser: An instanciated object to manage crawling rules according to 'robot.txt'
        """
        parsed_url = urlparse(base_url)
        base_domain = f"{parsed_url.scheme}://{parsed_url.netloc}"
        if base_domain in self.robots_parsers:
            return self.robots_parsers[base_domain]

        robots_url = urljoin(base_domain, "/robots.txt")
        rp = RobotFileParser()
        try:
            # Simulate a browser by setting a User-Agent
            headers = {
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36'
            }
            response = requests.get(robots_url, headers=headers, timeout=5)
            # Check if the robots.txt is accessible
            if response.status_code == 200:
                rp.parse(response.text.splitlines())
                # if debug_mode:
                #     print(f"Robots.txt content for {base_domain}: {response.text}")
                self.robots_parsers[base_domain] = rp  # Store parser for future use
                return rp
            else:
                print(f"Failed to fetch robots.txt for {base_domain}: Status {response.status_code}")
                return None

        except requests.exceptions.Timeout:
            print(f"Timeout while trying to fetch robots.txt for {base_domain}. Assuming allowed.")
            return None
        except Exception as e:
            print(f"Failed to retrieve or parse robots.txt for {base_domain}: {e}")
            return None

    def is_allowed_by_robots(self, url, thread_id=0):
        """
        Check if the given URL is allowed to be crawled based on robots.txt rules.
        """
        parsed_url = urlparse(url)
        base_url = f"{parsed_url.scheme}://{parsed_url.netloc}"
        rp = self.get_robots_parser(base_url)
        if rp:
            can_fetch = rp.can_fetch("*", url)
            # if debug_mode:
            #     print(f"Thread-{thread_id}: Robots.txt rule for {url}: {'Allowed' if can_fetch else 'Disallowed'}")
            return can_fetch

        # if debug_mode:
        #     print(f"Thread-{thread_id}: No robots.txt found or failed to parse for {base_url}. Assuming allowed.")
        return True  # If no robots.txt, assume it's allowed

    def download_page(self, url, thread_id=0):
        """
        If url is of type 'text/html', then download the HTML content.
        :param url: (str)
        :return: (tuple): (content, size, response_code)
        """
        if not self.is_allowed_by_robots(url, thread_id):
            print(f"Thread-{thread_id}: Blocked by robots.txt: {url}")
            return None, 0, None

        try:
            time.sleep(1)  # Add delay between requests to avoid overloading the server
            response = requests.get(url, timeout=8)
            # Check if the Content-Type is text/html
            content_type = response.headers.get('Content-Type', '').split(';')[0].strip()

            if content_type == 'text/html':
                print(f"Thread-{thread_id}: Successfully downloaded: {url}")
                return response.text, len(response.content), response.status_code
            else:
                # If the MIME type is not text/html, skip processing this file
                print(f"Thread-{thread_id}: Skipped (not text/html): {url} [MIME type: {content_type}]")
                return None, 0, None
        except requests.exceptions.Timeout:
            print(f"Thread-{thread_id}: Timeout while trying to access {url}")
        except Exception as e:
            print(f"Thread-{thread_id}: An error occurred when downloading {url}: {e}")
        return None, 0, None

    def extract_links(self, page_content, base_url):
        """
        Extracts all 'href' links from the page content using BeautifulSoup.
        :param page_content: (str) The HTML content of the page.
        :param base_url: (str) The base URL of the page (for handling relative links).
        :return: (list) A list of extracted absolute URLs.
        """
        try:
            # Set a timeout seconds for parsing the page
            signal.alarm(8)

            soup = BeautifulSoup(page_content, 'lxml')
            links = []
            for a_tag in soup.find_all('a', href=True):
                href = a_tag['href']
                try:
                    # Convert relative URLs to absolute URLs
                    full_url = urljoin(base_url, href)
                    links.append(full_url)
                except ValueError as ve:
                    print(f"Error joining URL: {href}, base: {base_url} - {ve}")
                    continue  # Skip the invalid URL and move to the next one

            # Disable the alarm after parsing completes
            signal.alarm(0)
            return links
        except TimeoutException:
            print(f"Parsing timed out for page: {base_url}")
            return []
        except Exception as e:
            print(f"An unexpected error occurred while parsing {base_url}: {e}")
            return []

    def normalize_url(self, url):
        """
        Normalize the URL by removing fragments and query parameters.
        :param url: The original URL
        :return: Normalized URL
        """
        try:
            # Parse the URL
            parts = urlsplit(url)
            # Remove query and fragment (i.e., part after ? or #)
            normalized_url = urlunsplit((parts.scheme, parts.netloc, parts.path, '', ''))
            return normalized_url
        except Exception as e:
            print(f"Error normalizing URL {url}: {e}")
            return url  # Return the original URL if normalization fails

    def crawl_pages(self, max_pages=1000, num_threads=10, retry_limit=5, retry_delay=2):
        """
        Multithreaded BFS crawling with priority
        """
        # Initialize start time
        self.start_time = datetime.now()

        # # Number of threads
        # num_threads

        # Keep track of how many URLs each thread processes
        thread_stats = {f"Thread-{i}": 0 for i in range(1, num_threads + 1)}

        def thread_worker(thread_id):
            nonlocal thread_stats
            retries = 0
            while self.url_counter < max_pages and retries < retry_limit:
                try:
                    # Try to get a URL from the queue with a timeout of 3 seconds
                    priority, queue_index, (url, depth) = self.to_visit.get(timeout=3)
                    retries = 0  # Reset retry count when a URL is found
                except:
                    retries += 1
                    print(f"Thread-{thread_id}: Queue empty, retrying {retries}/{retry_limit}...")
                    if retries >= retry_limit:
                        print(f"Thread-{thread_id}: Exceeded retry limit. Exiting...")
                        break  # Exit the thread after reaching retry limit
                    time.sleep(retry_delay)  # Wait before retrying
                    continue  # Retry fetching from the queue

                normalized_url = self.normalize_url(url)  # Normalize the URL first

                with self.visited_lock:
                    if normalized_url in self.url_visited:
                        print(f"Thread-{thread_id}: Page already visited. Skipping {normalized_url}...")
                        continue  # Skip already visited URLs

                print(
                    f"Thread-{thread_id}: Now visiting: Priority: {priority}, Depth: {depth}, "
                    f"Queue Index: {queue_index}, Queue Size: {self.to_visit.qsize()}, URL: {normalized_url}")

                # Download and process the page
                page_content, size, status_code = self.download_page(normalized_url, thread_id)
                if not page_content:
                    if debug_mode:
                        print(f"Thread-{thread_id}: Failed to download page {normalized_url}. Skipping...")
                    continue  # Skip the page

                # Update visited URLs
                with self.visited_lock:
                    self.url_visited[normalized_url] = {
                        'size': size,  # in bytes
                        'time': datetime.now(),
                        'status': status_code,
                        'depth': depth,  # BFS depth
                        'order': self.url_counter  # Visiting order
                    }
                    self.url_counter += 1
                    thread_stats[f"Thread-{thread_id}"] += 1

                # Extract links and enqueue new URLs
                extracted_links = self.extract_links(page_content, normalized_url)
                if debug_mode:
                    print(f"Thread-{thread_id}: Extracted {len(extracted_links)} links from {normalized_url}")

                # unique_links = set(extracted_links)
                # Normalize the extracted links before adding to the set
                unique_links = set(self.normalize_url(link) for link in extracted_links)
                visited_count = 0
                not_visited_count = 0
                invalid_url_count = 0
                existing_domain_count = 0
                new_domain_count = 0

                for link in unique_links:
                    # Construct absolute URLs and normalize
                    full_url = self.normalize_url(urljoin(url, link))
                    link_domain = self.get_domain(full_url)

                    # Check if the URL is valid and not in the blacklist
                    if not self.is_valid_url(full_url):
                        invalid_url_count += 1
                        continue  # Skip this URL if it's invalid

                    if full_url not in self.url_visited:
                        # Get priority for the new URL based on its domain
                        priority = self.get_priority(link_domain)
                        with self.enqueue_lock:
                            self.to_visit.put((priority, self.queue_index, (full_url, depth + 1)))
                            self.queue_index += 1
                        not_visited_count += 1
                        if link_domain in self.domain_enqueued:
                            existing_domain_count += 1
                        else:
                            new_domain_count += 1
                        self.update_domain_enqueued(link_domain)
                    else:
                        visited_count += 1

                print(
                    f"Thread-{thread_id}: Unique links extracted: {len(unique_links)}, Already visited: {visited_count}, "
                    f"Not visited (added to queue): {not_visited_count}, In blacklist or invalid: {invalid_url_count}, \n"
                    f"Thread-{thread_id}: URLs enqueued with existing domain: {existing_domain_count}, "
                    f"URLs enqueued with new domain: {new_domain_count}, \n"
                    f"Thread-{thread_id}: Total URLs visited: {self.url_counter}, Total unique domains enqueued: {len(self.domain_enqueued)} \n"
                    f"==================================")

                if self.url_counter >= max_pages:
                    print(f"Thread-{thread_id}: Crawling Finished: {max_pages} pages fetched. Stopping...")

        # Start threads
        threads = []
        for i in range(1, num_threads + 1):
            t = threading.Thread(target=thread_worker, args=(i,))
            t.start()
            threads.append(t)

        # Wait for all threads to complete
        for t in threads:
            t.join()

        # Log thread statistics
        self.log(thread_stats)

    def log(self, thread_stats):
        """
        Log the results of the crawl, including per-thread statistics.
        """
        total_pages = len(self.url_visited)
        total_size_bytes = sum(info['size'] for info in self.url_visited.values())
        total_size_mb = total_size_bytes / (1024 * 1024)  # Convert bytes to megabytes
        total_time_taken = (datetime.now() - self.start_time).total_seconds()

        status_codes_count = {}  # For counting different HTTP status codes

        # Generate the timestamp for log file naming
        timestamp = time.strftime("%Y-%m-%d-%H-%M-%S")
        log_filename = f'./data/crawler_log_{timestamp}.txt'

        with open(log_filename, 'w') as log_file:
            log_file.write("Crawled Pages Log:\n")
            log_file.write("======================================\n")

            # Log the crawled URLs
            for url, info in self.url_visited.items():
                log_file.write(
                    f"URL: {url}, Time: {info['time']}, Size: {info['size']} bytes, Depth: {info['depth']}, "
                    f"Status: {info['status']} {'(seed)' if url in self.seed_urls else ''}\n"
                )
                # Count HTTP status codes
                status_code = info['status']
                if status_code not in status_codes_count:
                    status_codes_count[status_code] = 1
                else:
                    status_codes_count[status_code] += 1

            log_file.write("======================================\n")
            log_file.write("Statistics:\n")
            log_file.write(f"Total pages crawled: {total_pages}\n")
            log_file.write(f"Total size: {total_size_mb:.2f} MB\n")  # Total size in megabytes
            log_file.write(f"Total time taken: {total_time_taken:.2f} seconds\n")

            # Log all HTTP status codes
            log_file.write("\nHTTP Status Codes Summary:\n")
            for code, count in status_codes_count.items():
                log_file.write(f"HTTP {code}: {count} times\n")

            # Log per-thread statistics
            log_file.write("\nThread Statistics:\n")
            for thread, count in thread_stats.items():
                log_file.write(f"{thread} processed {count} URLs\n")

            # Log the final queue size
            queue_size = self.to_visit.qsize()  # Use thread-safe qsize()
            log_file.write(f"\nSize of the PriorityQueue at program exit: {queue_size}\n")

            # Log the final domain enqueued size
            domain_enqueued_size = len(self.domain_enqueued)
            log_file.write(f"Size of domain_enqueued at program exit: {domain_enqueued_size}\n")

        print(f"Log written to {log_filename}")


def load_seeds(file_path, num_seeds=10):
    """
    Randomly select 10 seed pages from ./data/nz_domain_seeds_list.txt
    """
    with open(file_path, 'r') as file:
        seeds = file.readlines()
        seeds = [seed.strip() for seed in seeds if seed.strip()]  # clean whitespace or newlines
        if len(seeds) < num_seeds:
            num_seeds = len(seeds)
        # Randomly select seeds from the list
        return random.sample(seeds, num_seeds)


def main(num_seeds=20, max_pages=1000, num_threads=10):
    seeds_file_path = './data/nz_domain_seeds_list.txt'
    # seeds = [
    #     'https://www.mega.nz',
    #     'https://www.nzherald.co.nz',
    #     'https://www.stuff.co.nz',
    #     'https://www.google.co.nz',
    #     'https://www.auckland.ac.nz',
    #     'https://www.rnz.co.nz'
    # ]
    # seeds = [
    #     'https://www.gisborneherald.co.nz/'
    # ]
    seeds = load_seeds(seeds_file_path, num_seeds=num_seeds)

    print("Seed Pages:")
    for seed in seeds:
        print(seed)

    crawler = WebCrawler(seeds)
    print("Web crawler initialized succesfully! Crawling...")
    crawler.crawl_pages(max_pages=max_pages, num_threads=num_threads)


def robot_parser_test():
    """
    For debug purpose only
    """
    wc = WebCrawler(['https://www.gisborneherald.co.nz/'])
    rp = wc.get_robots_parser('https://www.gisborneherald.co.nz/')
    print(rp.can_fetch("*", 'https://www.gisborneherald.co.nz/'))


if __name__ == "__main__":
    start_time = time.time()

    debug_mode = True
    main(num_seeds=20, max_pages=1000, num_threads=30)

    # robot_parser_test()

    end_time = time.time()
    duration = end_time - start_time
    print(f"Crawling completed in {duration:.2f} seconds.")

