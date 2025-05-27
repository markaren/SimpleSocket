
#include "simple_socket/SharedMemoryConnection.hpp"

#include <iostream>

#include <ostream>
#include <vector>

using namespace simple_socket;

int main() {

    SharedMemoryConnection client("test_shared_mem", 1024, false);

    client.write("Per");

    std::vector<uint8_t> buffer(1024);
    const int read = client.read(buffer);
    std::string msg{buffer.begin(), buffer.begin() + read};

    std::cout << "Got: " << msg << std::endl;
}