
#include "simple_socket/SharedMemoryConnection.hpp"

#include <iostream>

#include <ostream>
#include <vector>

using namespace simple_socket;

int main() {

    SharedMemoryConnection server("test_shared_mem", 1024, true);

    std::vector<uint8_t> buffer(1024);

    std::cout << "Waiting for client..." << std::endl;
    const auto read = server.read(buffer);

    std::string msg{buffer.begin(), buffer.begin() + read};
    std::cout << "Got: " << msg << std::endl;


    server.write("Hello " + msg + "!");
}
