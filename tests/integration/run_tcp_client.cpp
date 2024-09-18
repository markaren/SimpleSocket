
#include "TCPSocket.hpp"

#include <iostream>
#include <vector>

using namespace simple_socket;

int main() {

    if (TCPClient client; client.connect("127.0.0.1", 8080)) {

        std::string message = "Per";
        client.write(message);

        std::vector<unsigned char> buffer(1024);
        const auto bytesRead = client.read(buffer);

        std::cout << "Response from server: " << std::string(buffer.begin(), buffer.begin() + static_cast<int>(bytesRead)) << std::endl;
    } else {
        std::cerr << "Failed to connect to server" << std::endl;
    }
}
