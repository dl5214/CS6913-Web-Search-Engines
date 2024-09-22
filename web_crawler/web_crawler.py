# from collections import deque
from queue import Queue
from urllib.parse import urlparse, urljoin
from urllib.robotparser import RobotFileParser
import requests
from datetime import datetime
from html.parser import HTMLParser
import random
import time


debug_mode = False

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
                    # print(f"Found link: {value}")
                    self.extracted_links.append(value)

        # # Check for 'href' or 'src' attributes in <a>, <link>, <script>, etc.
        # for key, value in attrs:
        #     if key in ('href', 'src') and value is not None:
        #         self.extracted_links.append(value)


class WebCrawler:
    """
    A simple web crawler that performs a breadth-first search (BFS) to crawl pages within the '.nz' domain.
    """

    def __init__(self, seeds, max_links_per_page=10):
        """
        Initializes the WebCrawler with the necessary attributes.
        :param seeds: (list) A list of seed URLs to start crawling from.
        """
        # Dictionary structure for visited pages
        # key: normalized URL, value: page size (in bytes), access time, HTTP return code, BFS depth
        self.visited = {}
        # BFS queue
        # self.to_visit = deque(seeds)
        # self.to_visit = deque([(url, 0) for url in seeds])
        self.to_visit = Queue()
        for url in seeds:
            self.to_visit.put((url, 0))
        # Do not visit the extensions in the blacklist
        self.blacklist_extensions = {'.jpg', '.jpeg', '.png', '.gif', '.pdf', '.doc', '.docx', '.xls', '.xlsx'}
        # object TLD
        self.domain = '.nz'
        self.counter = 1  # Visiting order
        self.robots_parsers = {}  # Store robot parsers for each domain
        self.max_links_per_page = max_links_per_page

    def is_valid_url(self, url):
        """
        Checks if the given URL is valid and should be crawled.
        :param url: (str) The URL to be checked.
        :return: (bool) True if the URL is valid and within the target domain, False otherwise.
        """
        # Parse url and get its components
        parsed_url = urlparse(url)
        # within .nz domain AND extension not in blacklist
        is_within_domain = parsed_url.netloc.endswith(self.domain)
        is_in_blacklist = False
        for extension in self.blacklist_extensions:
            if url.endswith(extension):
                is_in_blacklist = True
                break
        return is_within_domain and not is_in_blacklist

    # def get_robots_parser(self, base_url):
    #     """
    #     Retrieve and parse 'robots.txt' to respect Robot Exclusion Protocol
    #     :param base_url: str: The base URL of the website
    #     :return: RobotFileParser: An instanciated object to manage crawling rules according to 'robot.txt'
    #     """
    #     # Construct the full url for robot.txt
    #     robots_url = urljoin(base_url, "/robots.txt")
    #     rp = RobotFileParser()
    #     try:
    #         response = requests.get(robots_url, timeout=10)
    #         if response.status_code == 200:
    #             rp.parse(response.text.splitlines())
    #             print(f"Successfully parsed robots.txt for {base_url}")
    #         else:
    #             print(f"Status code {response.status_code} when fetching robots.txt for {base_url}")
    #         # rp.set_url(robots_url)
    #         # # Read and parse the robot.txt file
    #         # rp.read()
    #         # print(f"Successfully parsed robots.txt for {base_url}")
    #         # return rp
    #     except requests.exceptions.Timeout:
    #         print(f"Timeout while trying to access robots.txt for {base_url}")
    #     except Exception as e:
    #         print(f"Failed to retrieve or parse robots.txt for {base_url}: {e}")
    #
    #     return rp

    def get_robots_parser(self, base_url):
        """
        Retrieve and parse 'robots.txt' to respect Robot Exclusion Protocol
        :param base_url: str: The base URL of the website
        :return: RobotFileParser: An instanciated object to manage crawling rules according to 'robot.txt'
        """
        # Construct the full url for robot.txt
        # robots_url = urljoin(base_url, "/robots.txt")
        # rp = RobotFileParser()
        # try:
        #     rp.set_url(robots_url)
        #     # Read and parse the robot.txt file
        #     rp.read()
        #     print(f"Successfully parsed robots.txt for {base_url}")
        #     return rp
        # except Exception as e:
        #     print(f"Failed to retrieve or parse robots.txt for {base_url}: {e}")
        #     return None
        parsed_url = urlparse(base_url)
        base_domain = f"{parsed_url.scheme}://{parsed_url.netloc}"
        if base_domain in self.robots_parsers:
            return self.robots_parsers[base_domain]

        robots_url = urljoin(base_domain, "/robots.txt")
        rp = RobotFileParser()
        try:
            rp.set_url(robots_url)
            rp.read()
            self.robots_parsers[base_domain] = rp  # Store parser for future use
            return rp
        except Exception as e:
            print(f"Failed to retrieve or parse robots.txt for {base_domain}: {e}")
            return None

    def is_allowed_by_robots(self, url):
        """
        Check if the given URL is allowed to be crawled based on robots.txt rules.
        """
        parsed_url = urlparse(url)
        base_url = f"{parsed_url.scheme}://{parsed_url.netloc}"
        rp = self.get_robots_parser(base_url)
        if rp:
            return rp.can_fetch("*", url)
        return True  # If no robots.txt, assume it's allowed

    def download_page(self, url):
        """
        If url is of type 'text/html', then download the HTML content.
        :param url: (str)
        :return: (tuple): (content, size, response_code)
        """
        if not self.is_allowed_by_robots(url):
            print(f"Blocked by robots.txt: {url}")
            return None, 0, None

        print(f"Attempting to download: {url}")
        try:
            response = requests.get(url, timeout=5)
            if 'text/html' in response.headers['Content-Type']:
                print(f"Successfully downloaded: {url}")
                return response.text, len(response.content), response.status_code
            else:
                print(f"Content type mismatch for {url}: {response.headers['Content-Type']}")
        except requests.exceptions.Timeout:
            print(f"Timeout while trying to access {url}")
        except Exception as e:
            print(f"An error occurred when downloading {url}: {e}")
        return None, 0, None

    def crawl_pages(self, max_pages=1000):
        """
        BFS crawling
        :return: None
        """
        while not self.to_visit.empty() and self.counter <= max_pages:
            # url, depth = self.to_visit.popleft()
            print(f"Queue size: {self.to_visit.qsize()}")
            url, depth = self.to_visit.get()
            if url in self.visited:
                print("Page already visited. Skipping...")
                continue  # Skip the URL if already visited

            print(f"Visiting #: {self.counter}, Depth: {depth}, URL: {url}")
            page_content, size, status_code = self.download_page(url)
            if not page_content:
                if debug_mode:
                    print("Failed to download page. Skipping...")
                continue  # Skip the page

            # Update visited dictionary: Key: URL
            self.visited[url] = {
                'size': size,  # in bytes
                'time': datetime.now(),
                'status': status_code,
                'depth': depth,  # BFS depth
                'order': self.counter  # Visiting order
            }

            extractor = LinkExtractor()
            extractor.feed(page_content)
            # Limit the number of links to process by randomly selecting from extracted links
            if len(extractor.extracted_links) > self.max_links_per_page:
                selected_links = random.sample(extractor.extracted_links, self.max_links_per_page)
            else:
                selected_links = extractor.extracted_links
            for link in selected_links:
                # Construct absolute URLs
                full_url = urljoin(url, link)
                if self.is_valid_url(full_url) and full_url not in self.visited:
                    # self.to_visit.append((full_url, depth + 1))
                    self.to_visit.put((full_url, depth + 1))

            self.counter += 1

            if self.counter > max_pages:
                print(f"Crawling Finished: {max_pages} pages fetched. Stopping...")

        self.log()

    def log(self):
        with open('./data/crawler_log.txt', 'w') as log_file:
            for url, info in self.visited.items():
                log_file.write(
                    f"Time: {info['time']} - Depth: {info['depth']} - Order: {info['order']} - Status: {info['status']} - Size: {info['size']} bytes - URL: {url}\n"
                )

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


def main():
    seeds_file_path = './data/nz_domain_seeds_list.txt'
    if debug_mode:
        seeds = [
            # 'https://www.mega.nz',
            # 'https://www.nzherald.co.nz',
            # 'https://www.stuff.co.nz',
            # 'https://www.google.co.nz',
            # 'https://www.auckland.ac.nz',
            # 'https://www.rnz.co.nz'
            'https://www.gisborneherald.co.nz/'
        ]
    else:
        seeds = load_seeds(seeds_file_path, num_seeds=10)
    print("Seed Pages:")
    for seed in seeds:
        print(seed)

    crawler = WebCrawler(seeds, max_links_per_page=5)
    print("Web crawler initialized succesfully! Crawling...")
    crawler.crawl_pages(max_pages=1000)


if __name__=="__main__":
    start_time = time.time()

    debug_mode = True
    main()

    end_time = time.time()
    duration = end_time - start_time
    print(f"Crawling completed in {duration:.2f} seconds.")

