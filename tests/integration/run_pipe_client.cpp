
#include "simple_socket/Pipe.hpp"

#include <iostream>

int main() {

    auto conn = NamedPipe::connect("PingPongPipe", 500);

    if (!conn) return 1;

    // Ping-Pong logic
    std::string input;
    while (true) {
        std::cout << "Enter message: ";
        std::getline(std::cin, input);

        conn->send(input);
        if (input == "exit")
            break;

        std::vector<unsigned char> buffer(512);
        const auto bytesRecevied = conn->receive(buffer);
        std::string received{buffer.begin(), buffer.begin() + bytesRecevied};
        std::cout << "Server: " << received << std::endl;
    }

    std::cout << "Client shutting down..." << std::endl;
}
