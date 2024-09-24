
#include "simple_socket/Pipe.hpp"

#include <iostream>

using namespace simple_socket;

int main() {

    const auto conn = NamedPipe::connect("PingPongPipe", 500);

    if (!conn) return 1;

    // Ping-Pong logic
    std::string input;
    while (true) {
        std::cout << "Enter message: ";
        std::getline(std::cin, input);

        conn->write(input);
        if (input == "exit")
            break;

        std::vector<unsigned char> buffer(512);
        const auto bytesRecevied = conn->read(buffer);
        std::string received{buffer.begin(), buffer.begin() + bytesRecevied};
        std::cout << "Server: " << received << std::endl;
    }

    std::cout << "Client shutting down..." << std::endl;
}
