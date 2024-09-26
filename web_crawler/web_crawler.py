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
import socket


debug_mode = False


# Signal handler for timeout
def handler(signum, frame):
    raise TimeoutException()

# Set the global timeout handler (this can be outside of WebCrawler class)
signal.signal(signal.SIGALRM, handler)
# Set a global socket timeout, in seconds
socket.setdefaulttimeout(3)  # 3-second timeout, including DNS resolution time


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
        # object TLD
        self.domain = '.nz'
        # For logger to record seed pages
        self.seed_urls = set()
        # Initialize a set to track URLs that have been enqueued but not yet visited
        # self.url_enqueued = set()  # sync by url_enqueued_lock
        self.visited_and_redirected_urls_minimized = set()  # sync by url_visited_lock
        # self.url_visited_raw = set()  # not normalized, for debug purpose of 'page already visited'
        self.url_enqueued_minimized = set()  # if (equivalently) minimized url has enqueued, then skip

        # Dictionary structure for visited pages
        # key: normalized URL, value: page size (in bytes), access time, HTTP return code, BFS depth
        self.url_visited = {}  # sync by url_visited_lock
        # Dictionary to track domain enqueue times (key: domain, value: {enqueue_count: int, in_degree: int})
        self.domain_enqueued = {}  # sync by domain_enqueued_lock
        # Dictionary to track second-level domains (key: second-last domain, value: visited count)
        self.second_last_domain_visited = {}  # sync by second_last_domain_visited_lock
        # Initialize priority queue
        # self.to_visit = []
        self.to_visit = PriorityQueue()  # thread-safe itself

        # The number of URLs visited
        self.url_counter = 0  # sync by url_counter_lock
        # Counter for ordering within the priority queue
        self.queue_index = 1  # lock not needed
        # Store robot parsers for each domain
        self.robots_parsers = {}  # sync by robots_parsers_lock
        self.redirect_count = 0

        self.url_visited_lock = threading.Lock()
        self.domain_enqueued_lock = threading.Lock()
        self.second_last_domain_visited_lock = threading.Lock()
        # self.visited_and_redirected_urls_lock = threading.Lock()
        self.url_enqueued_lock = threading.Lock()
        self.url_counter_lock = threading.Lock()
        # self.queue_index_lock = threading.Lock()
        self.robots_parsers_lock = threading.Lock()
        # self.domain_and_url_lock = threading.Lock()

        # self.pq_lock = threading.Lock()
        # self.priority_lock = threading.Lock()

        # Do not visit the extensions in the blacklist
        self.blacklist_extensions = {
            '.jpg', '.jpeg', '.png', '.gif', '.bmp', '.svg',  # Image files
            '.pdf', '.doc', '.docx', '.xls', '.xlsx', '.ppt', '.txt',  # Document files
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
        # Common top domain (following .nz)
        self.second_last_domain_whitelist = {
            'govt', 'org', 'ac', 'co', 'cri', 'health',
            'com', 'net', 'edu', 'mil', 'info', 'biz', 'int',
            'ai', 'io', 'tech', 'xyz'
        }
        # Insert seed URLs into the priority queue
        for url in seeds:
            # self.to_visit.put((url, 0))
            normalized_url = self.normalize_url(url)
            minimized_url = self.minimize_url(url)
            domain = self.get_domain(normalized_url)
            # heapq.heappush(self.to_visit, (priority, self.queue_index, (url, 0)))  # Use priority, queue_index, (url, depth)
            # self.to_visit.put((priority, self.queue_index, (url, 0)))  # Use priority, queue_index, (url, depth)
            # Calculate priority for seed URLs
            final_priority, domain_priority, in_degree_priority, second_last_priority = self.get_priority(domain)
            self.to_visit.put((final_priority, self.queue_index,
                               (normalized_url, 0, domain_priority, in_degree_priority, second_last_priority)))
            self.queue_index += 1
            self.update_domain_enqueued(domain, None)  # Update domain enqueue count for seeds
            self.seed_urls.add(url)  # For logger to record seed pages
            # self.url_enqueued.add(normalized_url)  # Mark the seed URL as enqueued
            self.url_enqueued_minimized.add(minimized_url)

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

    def update_domain_enqueued(self, domain, parent_domain):
        """
        Update the count of how many times a domain has been enqueued.
        """
        with self.domain_enqueued_lock:
        # with self.domain_and_url_lock:
            if domain not in self.domain_enqueued:
                self.domain_enqueued[domain] = {'enqueue_count': 0, 'in_degree': 0, 'last_accessed': None}
            else:
                self.domain_enqueued[domain]['enqueue_count'] += 1

            # If it's a seed (origin_domain is None), we don't count in-degree
            if parent_domain and domain != parent_domain:
                self.domain_enqueued[domain]['in_degree'] += 1

    def get_second_last_domain(self, domain):
        """
        Extracts the second-last part of a domain.
        For example, if the domain is 'example.co.nz', this will return 'co'.
        """
        parts = domain.split('.')
        if len(parts) > 1:
            second_last = parts[-2]
            return second_last
        return None

    def update_second_last_domain_visited(self, domain):
        """
        Update the count of visits for second-last domains (e.g., 'co', 'com').
        :param domain: The domain being visited.
        """
        second_last = self.get_second_last_domain(domain)
        # if debug_mode:
        #     print(f"Extracted second last domain: {second_last}")
        if second_last:
            with self.second_last_domain_visited_lock:  # Ensure thread safety for second_last_domain_visited
            # with self.domain_and_url_lock:
                if second_last not in self.second_last_domain_visited:
                    self.second_last_domain_visited[second_last] = 0
                self.second_last_domain_visited[second_last] += 1

    def get_domain_priority(self, domain):
        """
        Calculate priority based on how many times the domain has been enqueued.
        Fewer enqueues result in a higher priority (lower number).
        """
        with self.domain_enqueued_lock:
        # with self.domain_and_url_lock:
            domain_info = self.domain_enqueued.get(domain, {'enqueue_count': 0})
            enqueue_count = domain_info['enqueue_count']

        # Lower enqueue count gets higher priority
        if enqueue_count == 0:
            return 1
        elif enqueue_count == 1:
            return 2
        elif enqueue_count == 2:
            return 3
        elif enqueue_count <= 5:
            return 4
        elif enqueue_count <= 10:
            return 5
        elif enqueue_count <= 100:
            return 6
        elif enqueue_count <= 1000:
            return 7
        elif enqueue_count <= 10000:
            return 8
        elif enqueue_count <= 1000000:
            return 9
        else:
            return 10  # Lowest priority for frequent enqueues

    def get_in_degree_priority(self, domain):
        """
        Calculate priority based on in-degree (number of other domains linking to this domain).
        Higher in-degree results in higher priority (lower number).
        """
        with self.domain_enqueued_lock:
        # with self.domain_and_url_lock:
            domain_info = self.domain_enqueued.get(domain, {'in_degree': 0})
            in_degree = domain_info['in_degree']

        # More in-degree results in higher priority
        if in_degree == 0:
            return 10  # Low priority for no incoming links
        elif in_degree == 1:
            return 9
        elif in_degree == 2:
            return 8
        elif in_degree <= 5:
            return 7
        elif in_degree <= 10:
            return 6
        elif in_degree <= 20:
            return 5
        elif in_degree <= 50:
            return 4
        elif in_degree <= 500:
            return 3
        elif in_degree <= 8000:
            return 2
        else:
            return 1  # Highest priority for very popular domains

    def get_second_last_domain_priority(self, domain):
        """
        Calculate priority based on how frequently the second-last domain has been visited.
        Less frequent second-last domains should have higher priority (lower number).
        """
        second_last = self.get_second_last_domain(domain)
        if not second_last:
            return 10  # Default low priority if second-last domain is missing

        with self.second_last_domain_visited_lock:
        # with self.domain_and_url_lock:
            second_last_count = self.second_last_domain_visited.get(second_last, 0)

        # Fewer occurrences of second-last domain result in higher priority
        if second_last in self.second_last_domain_whitelist:
            if second_last_count == 0:
                return 1  # High priority for common second-last domains
            elif second_last_count <= 5:
                return 2
            elif second_last_count <= 20:
                return 3
            elif second_last_count <= 50:
                return 4
            elif second_last_count <= 200:
                return 5
            elif second_last_count <= 500:
                return 6
            elif second_last_count <= 1000:
                return 7
            elif second_last_count <= 10000:
                return 8
            elif second_last_count <= 100000:
                return 9
            else:
                return 10  # Low priority for common second-last domains
        else:
            if second_last_count == 0:
                return 2
            elif second_last_count <= 3:
                return 3
            elif second_last_count <= 6:
                return 4
            elif second_last_count <= 15:
                return 6
            elif second_last_count <= 50:
                return 6
            elif second_last_count <= 600:
                return 8
            elif second_last_count <= 10000:
                return 9
            else:
                return 10  # Low priority for common second-last domains

    def get_priority(self, domain):
        """
        Combine all priority factors to calculate the final priority for the domain.
        """
        domain_priority = self.get_domain_priority(domain)
        in_degree_priority = self.get_in_degree_priority(domain)
        second_last_priority = self.get_second_last_domain_priority(domain)

        # Combine priorities (weighted sum, can adjust weights as needed)
        final_priority = (domain_priority * 47) + (in_degree_priority * 13) + (second_last_priority * 29)

        return final_priority, domain_priority, in_degree_priority, second_last_priority

    def get_robots_parser(self, base_url, thread_id=0, total_timeout=6):
        """
        Retrieve and parse 'robots.txt' to respect Robot Exclusion Protocol
        :param base_url: str: The base URL of the website
        :return: RobotFileParser: An instanciated object to manage crawling rules according to 'robot.txt'
        """
        # # Set the total timeout for this operation
        # signal.alarm(total_timeout)

        parsed_url = urlparse(base_url)
        base_domain = f"{parsed_url.scheme}://{parsed_url.netloc}"
        with self.robots_parsers_lock:
            # Check if we already have a cached result (even for failure)
            if base_domain in self.robots_parsers:
                return self.robots_parsers[base_domain]

        robots_url = urljoin(base_domain, "/robots.txt")
        rp = RobotFileParser()
        try:
            # Simulate a browser by setting a User-Agent
            headers = {
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36'
            }
            response = requests.get(robots_url, headers=headers, timeout=(3, 5))
            # Check if the robots.txt is accessible
            if response.status_code == 200:
                rp.parse(response.text.splitlines())
                # if debug_mode:
                #     print(f"Robots.txt content for {base_domain}: {response.text}")
                with self.robots_parsers_lock:
                    self.robots_parsers[base_domain] = rp  # Store parser for future use
                return rp
            else:
                print(f"Thread-{thread_id}: Failed to fetch robots.txt for {base_domain}: Status {response.status_code}")
        except TimeoutException:
            print(f"Thread-{thread_id}: Timeout while fetching robots.txt for {base_domain}. Assuming allowed.")
        except requests.exceptions.Timeout:
            print(f"Thread-{thread_id}: Timeout while fetching robots.txt for {base_domain}. Assuming allowed.")
        except requests.exceptions.ConnectionError as ce:
            print(f"Thread-{thread_id}: Connection error while fetching robots.txt for {base_domain}: {ce}")
        except Exception as e:
            print(f"Thread-{thread_id}: Failed to retrieve or parse robots.txt for {base_domain}: {e}")
        # finally:
        #     # Disable the alarm after the operation completes
        #     signal.alarm(0)

    def is_allowed_by_robots(self, url, thread_id=0, timeout_duration=5):
        """
        Check if the given URL is allowed to be crawled based on robots.txt rules.
        If fetching the robots.txt takes longer than timeout_duration, assume allowed.
        """
        parsed_url = urlparse(url)
        base_url = f"{parsed_url.scheme}://{parsed_url.netloc}"

        # # Set the signal alarm for timeout
        # signal.alarm(timeout_duration)

        try:

            # Try to get the robots.txt parser
            rp = self.get_robots_parser(base_url, thread_id)

            if rp:
                can_fetch = rp.can_fetch("*", url)
                return can_fetch

            # If no robots.txt was found, assume allowed
            return True

        except TimeoutException:
            print(f"Thread-{thread_id}: Timeout fetching robots.txt for {base_url}. Assuming allowed.")
            # If a timeout occurs, assume allowed
            return True

        # finally:
        #     # Make sure to disable the alarm in case of other exceptions
        #     signal.alarm(0)

    def check_and_wait_for_interval(self, domain, min_interval=2, thread_id=0, max_total_wait=20):
        """
        Check the last access time of a domain and wait if needed to ensure the minimum interval.
        If waiting exceeds max_total_wait, the function will raise a TimeoutException.
        :param domain: The domain to check.
        :param min_interval: The minimum time interval (in seconds) between two accesses to the same domain.
        :param max_total_wait: The maximum time a thread will wait before raising a timeout (in seconds).
        """
        total_wait_time = 0  # Track the total amount of time this thread has been waiting

        while total_wait_time < max_total_wait:
            with self.domain_enqueued_lock:
            # with self.domain_and_url_lock:
                # Retrieve the last accessed time
                last_accessed = self.domain_enqueued.get(domain, {}).get('last_accessed')
                elapsed_time = time.time() - last_accessed if last_accessed else None

                # If no recent access or enough time has passed, proceed
                if last_accessed is None or elapsed_time >= min_interval:
                    # Update last accessed time and proceed
                    self.domain_enqueued[domain]['last_accessed'] = time.time()
                    break  # Exit the retry loop since this thread can now access the domain

            # If we need to wait, release the lock and sleep for a bit before retrying
            if last_accessed:
                wait_time = min(min_interval - elapsed_time, max_total_wait - total_wait_time)
                sleep_time = random.uniform(0.5, wait_time)  # Sleep for a random portion of the wait time
                total_wait_time += sleep_time  # Increment the total wait time
                # print(f"Thread-{thread_id}: Waiting for {sleep_time:.2f} seconds "
                #       f"before retrying access to domain: {domain}")
                time.sleep(sleep_time)

        if total_wait_time >= max_total_wait:
            raise TimeoutException(f"Thread-{thread_id}: Timeout exceeded while waiting for domain: {domain}")

    def download_page(self, url, thread_id=0, total_timeout = 10):
        """
        If url is of type 'text/html', then download the HTML content.
        :param url: (str)
        :return: (tuple): (content, size, response_code, final_url)
        """
        domain = self.get_domain(url)
        # signal.alarm(20)  # Set the total timeout (in seconds)
        try:
            # Ensure the minimum interval between requests
            self.check_and_wait_for_interval(domain, min_interval=2, thread_id=thread_id)

            # # Start the timeout countdown
            # signal.alarm(total_timeout)  # Set the total timeout (in seconds)

            if not self.is_allowed_by_robots(url, thread_id):
                print(f"Thread-{thread_id}: Blocked by robots.txt: {url}")
                return None, 0, 'Robots', None

            # time.sleep(1)  # Add delay between requests to avoid overloading the server
            response = requests.get(url, timeout=(3, 8))

            # Normalize the final URL after any redirection
            final_url = self.normalize_url(response.url)

            # Check if the Content-Type is text/html
            content_type = response.headers.get('Content-Type', '').split(';')[0].strip()

            if content_type == 'text/html':
                print(f"Thread-{thread_id}: Successfully downloaded: {url}, Status code: {response.status_code}")
                return response.text, len(response.content), response.status_code, final_url
            else:
                # If the MIME type is not text/html, skip processing this file
                print(f"Thread-{thread_id}: Skipped (not text/html): {url} [MIME type: {content_type}]")
                return None, 0, 'MIME Type Not HTML', final_url
        except TimeoutException as te:
            print(f"Thread-{thread_id}: Operation timed out for {url}")
            return None, 0, 'Timeout', None
        except requests.exceptions.Timeout:
            print(f"Thread-{thread_id}: Timeout while trying to access {url}")
            return None, 0, 'Timeout', None
        except requests.exceptions.ConnectionError as ce:
            print(f"Thread-{thread_id}: Connection error when accessing {url}: {ce}")
            return None, 0, 'Connection Failure', None
        except Exception as e:
            print(f"Thread-{thread_id}: An error occurred when downloading {url}: {e}")
            return None, 0, 'Unexpected Failure', None
        # finally:
            # signal.alarm(0)  # Disable the alarm after the operation completes

    def extract_links(self, page_content, base_url, parse_timeout=8):
        """
        Extracts all 'href' links from the page content using BeautifulSoup.
        :param page_content: (str) The HTML content of the page.
        :param base_url: (str) The base URL of the page (for handling relative links).
        :return: (list) A list of extracted absolute URLs.
        """

        # # Set a timeout seconds for parsing the page
        # signal.alarm(8)

        try:
            # soup = BeautifulSoup(page_content, 'lxml')
            soup = BeautifulSoup(page_content, 'html.parser')
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
            return links
        except TimeoutException:
            print(f"Parsing timed out for page: {base_url}")
            return []
        except Exception as e:
            print(f"An unexpected error occurred while parsing {base_url}: {e}")
            return []
        # finally:
        #     # Disable the alarm
        #     signal.alarm(0)

    # for data storage purpose
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

    # for comparison purpose
    def minimize_url(self, url):
        """
        Minimize the URL by removing the protocol (http/https) and 'www.' from the domain,
        but keeping the path and other elements intact for comparison purposes.
        :param url: The original URL
        :return: Minimized URL for comparison
        """
        try:
            # Parse the URL
            parts = urlsplit(url)

            # Remove 'www.' from the netloc (domain) for normalization and minimize comparison
            netloc = parts.netloc.lower().replace('www.', '')  # Ignore 'www.' for comparison

            # Normalize the path: remove trailing '/' but keep the rest of the path
            path = parts.path.rstrip('/')
            if path == '':
                path = ''  # Ensure root path '/' is treated as empty

            # Rebuild minimized URL without scheme (protocol) and without query/fragment
            minimized_url = urlunsplit(('', netloc, path, '', ''))

            return minimized_url
        except Exception as e:
            print(f"Error minimizing URL {url}: {e}")
            return url  # Return the original URL if minimization fails

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
            while retries < retry_limit:
                with self.url_counter_lock:
                    curr_url_index = self.url_counter
                if curr_url_index >= max_pages:
                    break   # Exit the loop if the max page limit is reached

                try:
                    # Try to get a URL from the queue with a timeout of 3 seconds
                    # with self.pq_lock:
                    final_priority, queue_index, (url, depth,
                                                  domain_priority, in_degree_priority,
                                                  second_last_priority) = self.to_visit.get(timeout=3)
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
                minimized_url = self.minimize_url(normalized_url)
                parent_domain = self.get_domain(normalized_url)  # The current domain is the parent of urls in this page
                # Ensure update_second_last_domain_visited
                self.update_second_last_domain_visited(parent_domain)  # thread-safe

                with self.url_visited_lock:
                # with self.url_enqueued_lock:
                # with self.domain_and_url_lock:
                # with self.visited_and_redirected_urls_lock:
                    if minimized_url in self.visited_and_redirected_urls_minimized:
                        # if normalized_url in self.url_visited_raw:
                        #     print(f"Thread-{thread_id}: (Unexpected) Page already visited. Skipping {normalized_url}...")
                        # else:
                        #     print(f"Thread-{thread_id}: Page already visited due to redirecting mechanism. "
                        #           f"Skipping {normalized_url}...")
                        # continue  # Skip already visited URLs
                        print(f"Thread-{thread_id}: Page already visited due to redirecting mechanism. "
                              f"Skipping {normalized_url}...")
                        continue  # Skip already visited URLs
                    else:
                        # Mark this URL as visited
                        self.visited_and_redirected_urls_minimized.add(minimized_url)
                with self.url_counter_lock:
                    self.url_counter += 1

                thread_stats[f"Thread-{thread_id}"] += 1

                print(
                    f"Thread-{thread_id}: Now visiting #: {curr_url_index}, "
                    f"Final Priority: {final_priority}, Depth: {depth}, "
                    f"Domain Priority: {domain_priority}, In-degree Priority: {in_degree_priority}, "
                    f"Second-last Priority: {second_last_priority},\n "
                    f"Queue Index: {queue_index}, Queue Size: {self.to_visit.qsize()}, URL: {normalized_url}")

                # Download and process the page
                page_content, size, status_code, final_url = self.download_page(normalized_url, thread_id)
                redirect_flag = False
                if final_url and self.minimize_url(final_url) != minimized_url:
                    print(f"Thread-{thread_id}: Redirect from {normalized_url} to {final_url} detected ")
                    redirect_flag = True
                    self.redirect_count += 1
                    # redirect_domain = self.get_domain(final_url)

                # Update size and status in visited URLs
                with self.url_visited_lock:
                # with self.domain_and_url_lock:
                    self.url_visited[normalized_url] = {
                        'size': size,
                        'time': datetime.now(),
                        'status': status_code,
                        'depth': depth,  # BFS depth
                        'order': curr_url_index,  # Visiting order
                        'redirect_url': None
                    }
                    # self.url_visited_raw.add(normalized_url)

                    if redirect_flag:
                        self.url_visited[normalized_url]['redirect_url'] = final_url
                        minimized_final = self.minimize_url(final_url)
                        if minimized_final not in self.visited_and_redirected_urls_minimized:
                            self.visited_and_redirected_urls_minimized.add(minimized_final)
                            with self.url_enqueued_lock:
                                self.url_enqueued_minimized.add(minimized_final)
                            if not self.is_valid_url(final_url):
                                print(
                                    f"Thread-{thread_id}: Redirected URL {final_url} is invalid "
                                    f"(out-of-domain or in blacklist). Skipping...")
                                continue  # Skip the redirected URL if it's not valid
                        else:
                            print(f"Thread-{thread_id}: Redirected page already visited. "
                                  f"Skipping {final_url} redirected by {normalized_url}...")
                            continue  # Skip already visited URLs

                if not page_content:
                    if debug_mode:
                        print(f"Thread-{thread_id}: Skipping {normalized_url} due to status: {status_code} ")
                    continue  # Skip the page

                # Extract links and enqueue new URLs
                extracted_links = self.extract_links(page_content, normalized_url)
                if debug_mode:
                    print(f"Thread-{thread_id}: Extracted {len(extracted_links)} links from {normalized_url}")

                # unique_links = set(extracted_links)
                # Construct absolute URLs and normalize before adding to the set
                unique_links = set(self.normalize_url(urljoin(url, link)) for link in extracted_links)
                # visited_count = 0
                previously_enqueued_count = 0
                not_visited_count = 0
                invalid_url_count = 0
                existing_domain_count = 0
                new_domain_count = 0

                for full_url in unique_links:

                    link_domain = self.get_domain(full_url)
                    minimized_full = self.minimize_url(full_url)

                    # Check if the URL is valid and not in the blacklist
                    if not self.is_valid_url(full_url):
                        invalid_url_count += 1
                        continue  # Skip this URL if it's invalid

                    # Before enqueueing, check if the URL has already enqueued (either visited or not)
                    visited_flag = False
                    with self.url_visited_lock:
                        if minimized_full in self.visited_and_redirected_urls_minimized:
                            visited_flag = True
                    with self.url_enqueued_lock:
                        # if (minimized_full in self.url_enqueued_minimized) or \
                        #         (minimized_full in self.visited_and_redirected_urls_minimized):
                        if visited_flag or (minimized_full in self.url_enqueued_minimized):
                            previously_enqueued_count += 1
                            continue  # Skip already enqueued URLs
                        else:
                            # self.url_enqueued.add(full_url)
                            self.url_enqueued_minimized.add(minimized_full)

                    # Get priority for the new URL based on its domain
                    # with self.priority_lock:
                    final_priority, domain_priority, in_degree_priority, second_last_priority = self.get_priority(link_domain)
                    # Immediately update domain_enqueued to avoid racing conditions
                    self.update_domain_enqueued(link_domain, parent_domain)
                    # with self.pq_lock:
                    self.to_visit.put((final_priority, self.queue_index,
                                       (full_url, depth + 1,
                                        domain_priority, in_degree_priority, second_last_priority)))
                    self.queue_index += 1
                    not_visited_count += 1
                    # with self.domain_enqueued_lock:
                    if link_domain in self.domain_enqueued:
                        existing_domain_count += 1
                    else:
                        new_domain_count += 1
                    # self.update_domain_enqueued(link_domain, parent_domain)

                # with self.domain_enqueued_lock:
                with self.domain_enqueued_lock:
                    curr_domain_enqueued_size = len(self.domain_enqueued)
                print(
                    f"Thread-{thread_id}: Unique links extracted: {len(unique_links)}, "
                    # f"Already visited: {visited_count}, "
                    f"Previously enqueued (either visited or not): {previously_enqueued_count}, "
                    f"Never enqueued (added to queue): {not_visited_count}, \n"
                    f"Thread-{thread_id}: In blacklist or invalid: {invalid_url_count}, "
                    f"URLs enqueued with existing domain: {existing_domain_count}, "
                    f"URLs enqueued with new domain: {new_domain_count}, "
                    # f"Thread-{thread_id}: Total URLs visited: {self.url_counter}, "
                    f"Total unique domains enqueued: {curr_domain_enqueued_size} \n"
                    f"==================================")

                if curr_url_index >= max_pages:
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
        print("Crawling completed. Start logging...")

        total_pages = len(self.url_visited)
        successful_pages = len({url: info for url, info in self.url_visited.items() if info['status'] == 200})
        total_size_bytes = sum(info['size'] for info in self.url_visited.values())
        total_size_mb = total_size_bytes / (1024 * 1024)  # Convert bytes to megabytes
        average_size = total_size_bytes / successful_pages / 1024 if successful_pages > 0 else 0 # in kilobytes
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
                    f"Depth: {info['depth']}, Status: {info['status']}, "
                    f"{'(seed)' if url in self.seed_urls else ''}URL: {url}, "
                )
                if info.get('redirect_url'):
                    log_file.write(f"Redirected to: {info['redirect_url']}, ")
                log_file.write(
                    f"Time: {info['time']}, Size: {info['size']} bytes \n"
                )
                # Count HTTP status codes
                status_code = info['status']
                if status_code not in status_codes_count:
                    status_codes_count[status_code] = 1
                else:
                    status_codes_count[status_code] += 1

            log_file.write("======================================\n")
            log_file.write("Statistics:\n")
            log_file.write(f"Total pages crawled (trials): {total_pages}\n")
            log_file.write(f"Total size: {total_size_mb:.2f} MB\n")  # Total size in megabytes
            log_file.write(f"Average non-empty page size: {average_size:.2f} KB\n")
            log_file.write(f"Total redirects detected: {self.redirect_count} times\n")
            log_file.write(f"Total time taken: {total_time_taken:.2f} seconds\n")

            # Log all HTTP status codes
            log_file.write("\nHTTP Status Codes Summary:\n")
            for code in sorted(status_codes_count, key=str):
                count = status_codes_count[code]
                log_file.write(f"HTTP {code}: {count} times\n")
            if 'Unexpected Failure' in status_codes_count:
                log_file.write("*Unexpected Failure refers to parsing error, redirecting exceeds limit, etc.\n")

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


if __name__ == "__main__":
    socket.setdefaulttimeout(3)
    start_time = time.time()

    debug_mode = True
    main(num_seeds=20, max_pages=20000, num_threads=30)

    end_time = time.time()
    duration = end_time - start_time
    print(f"Crawling completed in {duration:.2f} seconds.")

