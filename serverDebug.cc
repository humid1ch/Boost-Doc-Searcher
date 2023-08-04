#include <iostream>
#include "util.hpp"
#include "index.hpp"
#include "searcher.hpp"

const std::string& rawPath = "./data/output/raw";

int main() {
	ns_searcher::searcher searcher;
	searcher.initSearcher(rawPath);

	std::string query;
	std::string json_string;

	char buffer[1024];
	while (true) {
		std::cout << "Please Enter You Search Query# ";
		fgets(buffer, sizeof(buffer) - 1, stdin);
		buffer[strlen(buffer) - 1] = 0;
		query = buffer;
		searcher.search(query, &json_string);
		std::cout << json_string << std::endl;
	}

	return 0;
}
