
#include "TCPSocket.hpp"

#include <iostream>
#include <vector>

int main() {

    TCPClient client;
    if (client.connect("127.0.0.1", 8080)) {

        std::string message = "Per";
        client.write(message);

        size_t bytesRead = 0;
        std::vector<unsigned char> buffer(1024);
        client.read(buffer, &bytesRead);

        std::cout << "Response from server: " << std::string(buffer.begin(), buffer.begin() + static_cast<int>(bytesRead)) << std::endl;
    } else {
        std::cerr << "Failed to connect to server" << std::endl;
    }
}
