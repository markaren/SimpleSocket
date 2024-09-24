
#include "simple_socket/TCPSocket.hpp"

#include <iostream>
#include <array>

using namespace simple_socket;

int main() {

    TCPClientContext client;
    if (const auto conn = client.connect("127.0.0.1", 8080)) {

        conn->write("Per");

        std::array<unsigned char, 1024> buffer{};
        const auto bytesRead = conn->read(buffer);

        std::cout << "Response from server: " << std::string(buffer.begin(), buffer.begin() + bytesRead) << std::endl;
    } else {
        std::cerr << "Failed to connect to server" << std::endl;
    }
}
