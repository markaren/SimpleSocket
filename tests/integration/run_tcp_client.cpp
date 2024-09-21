
#include "simple_socket/TCPSocket.hpp"

#include <iostream>
#include <vector>

using namespace simple_socket;

int main() {

    TCPClientContext client;
    if (const auto conn = client.connect("127.0.0.1", 8080)) {

        std::string message = "Per";
        conn->write(message);

        std::vector<unsigned char> buffer(1024);
        const auto bytesRead = conn->read(buffer);

        std::cout << "Response from server: " << std::string(buffer.begin(), buffer.begin() + static_cast<int>(bytesRead)) << std::endl;
    } else {
        std::cerr << "Failed to connect to server" << std::endl;
    }
}
