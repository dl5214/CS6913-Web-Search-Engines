from queue import Queue
from urllib.parse import urlparse, urljoin
from urllib.robotparser import RobotFileParser
import requests
from datetime import datetime
from html.parser import HTMLParser
import random
import time
from bs4 import BeautifulSoup
import my_pq


debug_mode = False

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

    def __init__(self, seeds, max_links_per_page=10):
        """
        Initializes the WebCrawler with the necessary attributes.
        :param seeds: (list) A list of seed URLs to start crawling from.
        """
        # Dictionary structure for visited pages
        # key: normalized URL, value: page size (in bytes), access time, HTTP return code, BFS depth
        self.url_visited = {}
        # Dictionary to track domain enqueue times (key: domain, value: enqueue count)
        self.domain_enqueued = {}
        # BFS queue
        self.to_visit = Queue()
        for url in seeds:
            self.to_visit.put((url, 0))
            domain = self.get_domain(url)
            self.update_domain_enqueued(domain)  # Update domain enqueue count for seeds
        # Do not visit the extensions in the blacklist
        self.blacklist_extensions = {'.jpg', '.jpeg', '.png', '.gif', '.pdf', '.doc', '.docx', '.xls', '.xlsx'}
        # object TLD
        self.domain = '.nz'
        self.url_counter = 0  # The number of URLs visited
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
        if domain not in self.domain_enqueued:
            self.domain_enqueued[domain] = 1
        else:
            self.domain_enqueued[domain] += 1

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
            response = requests.get(robots_url, headers=headers)
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
            can_fetch = rp.can_fetch("*", url)
            if debug_mode:
                print(f"Robots.txt rule for {url}: {'Allowed' if can_fetch else 'Disallowed'}")
            return can_fetch

        if debug_mode:
            print(f"No robots.txt found or failed to parse for {base_url}. Assuming allowed.")
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

    def extract_links(self, page_content, base_url):
        """
        Extracts all 'href' links from the page content using BeautifulSoup.
        :param page_content: (str) The HTML content of the page.
        :param base_url: (str) The base URL of the page (for handling relative links).
        :return: (list) A list of extracted absolute URLs.
        """
        soup = BeautifulSoup(page_content, 'lxml')
        links = []
        for a_tag in soup.find_all('a', href=True):
            href = a_tag['href']
            # Convert relative URLs to absolute URLs
            full_url = urljoin(base_url, href)
            links.append(full_url)
        return links

    def crawl_pages(self, max_pages=1000):
        """
        BFS crawling
        :return: None
        """
        while not self.to_visit.empty() and self.url_counter <= max_pages:
            # url, depth = self.to_visit.popleft()
            print(f"Queue size: {self.to_visit.qsize()}")
            url, depth = self.to_visit.get()

            if url in self.url_visited:
                print("Page already visited. Skipping...")
                continue  # Skip the URL if already visited

            print(f"Now visiting: Depth: {depth}, URL: {url}")
            page_content, size, status_code = self.download_page(url)
            if not page_content:
                if debug_mode:
                    print("Failed to download page. Skipping...")
                continue  # Skip the page

            # Update visited dictionary: Key: URL
            self.url_visited[url] = {
                'size': size,  # in bytes
                'time': datetime.now(),
                'status': status_code,
                'depth': depth,  # BFS depth
                'order': self.url_counter  # Visiting order
            }

            # Deprecated: extract using LinkExtractor class
            # extractor = LinkExtractor()
            # extractor.feed(page_content)
            # if debug_mode:
            #     print(f"Extracted {len(extractor.extracted_links)} links from {url}")
            # Limit the number of links to process by randomly selecting from extracted links
            # if len(extractor.extracted_links) > self.max_links_per_page:
            #     selected_links = random.sample(extractor.extracted_links, self.max_links_per_page)
            # else:
            #     selected_links = extractor.extracted_links

            # Extract links using BeautifulSoup
            extracted_links = self.extract_links(page_content, url)
            if debug_mode:
                print(f"Extracted {len(extracted_links)} links from {url}")

            # Use a set to remove duplicate links
            unique_links = set(extracted_links)

            # Initialize visited and not visited counters
            visited_count = 0
            not_visited_count = 0
            invalid_url_count = 0
            existing_domain_count = 0
            new_domain_count = 0

            for link in unique_links:
                # Construct absolute URLs
                full_url = urljoin(url, link)
                link_domain = self.get_domain(full_url)

                # Check if the URL is valid and not in the blacklist
                if not self.is_valid_url(full_url):
                    invalid_url_count += 1
                    # if debug_mode:
                    #     print(f"Invalid or blacklisted URL: {full_url}")
                    continue  # Skip this URL if it's invalid

                if full_url not in self.url_visited:
                    # Add to the queue if not visited
                    self.to_visit.put((full_url, depth + 1))
                    not_visited_count += 1
                    # Check if the domain is already in the domain_enqueued dictionary
                    if link_domain in self.domain_enqueued:
                        existing_domain_count += 1
                    else:
                        new_domain_count += 1
                    self.update_domain_enqueued(link_domain)
                else:
                    visited_count += 1

            # Print the count of visited, not visited, added to queue, and invalid URLs after processing the links
            print(f"Unique links extracted: {len(unique_links)}, "
                  f"Already visited: {visited_count}, "
                  f"Not visited (added to queue): {not_visited_count}, "
                  f"In blacklist or invalid: {invalid_url_count}")
            print(f"URLs enqueued with existing domain: {existing_domain_count}, "
                  f"URLs enqueued with new domain: {new_domain_count}")
            self.url_counter += 1

            print(f"Total URLs visited: {self.url_counter}, "
                  f"Total unique domains enqueued: {len(self.domain_enqueued)}")

            if self.url_counter > max_pages:
                print(f"Crawling Finished: {max_pages} pages fetched. Stopping...")

        self.log()

    def log(self):
        with open('./data/crawler_log.txt', 'w') as log_file:
            for url, info in self.url_visited.items():
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
            'https://www.mega.nz',
            'https://www.nzherald.co.nz',
            'https://www.stuff.co.nz',
            'https://www.google.co.nz',
            'https://www.auckland.ac.nz',
            'https://www.rnz.co.nz'
        ]
        # seeds = [
        #     'https://www.gisborneherald.co.nz/'
        # ]
    else:
        seeds = load_seeds(seeds_file_path, num_seeds=10)
    print("Seed Pages:")
    for seed in seeds:
        print(seed)

    crawler = WebCrawler(seeds, max_links_per_page=5)
    print("Web crawler initialized succesfully! Crawling...")
    crawler.crawl_pages(max_pages=1000)


def robot_parser_test():
    """
    For debug purpose only
    """
    wc = WebCrawler(['https://www.gisborneherald.co.nz/'])
    rp = wc.get_robots_parser('https://www.gisborneherald.co.nz/')
    print(rp.can_fetch("*", 'https://www.gisborneherald.co.nz/'))


if __name__=="__main__":
    start_time = time.time()

    debug_mode = True
    main()

    # robot_parser_test()

    end_time = time.time()
    duration = end_time - start_time
    print(f"Crawling completed in {duration:.2f} seconds.")

