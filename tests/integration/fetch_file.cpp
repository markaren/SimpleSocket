
#include "simple_socket/http/SimpleHttpFetcher.hpp"

#include <fstream>
#include <iostream>

using namespace simple_socket;

int main() {
    SimpleHttpFetcher fetcher;
    const auto response = fetcher.fetch("https://upload.wikimedia.org/wikipedia/commons/3/3f/Placeholder_view_vector.svg");

    if (response) {
        std::cout << response.value() << std::endl;
        std::ofstream output("Placeholder_view_vector.svg", std::ios::binary);
        output << *response;
    } else {
        std::cerr << "Failed to fetch the file." << std::endl;
    }
}
