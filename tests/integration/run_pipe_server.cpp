
#include "simple_socket/Pipe.hpp"

#include <iostream>

int main() {

    std::cout << "Waiting for client connection..." << std::endl;
    const auto conn = NamedPipe::listen("PingPongPipe");

    if (!conn) return 1;

    std::vector<unsigned char> buffer(512);
    // Ping-Pong logic
    while (true) {
        const auto len = conn->receive(buffer);
        std::string received{buffer.begin(), buffer.begin() + len};
        std::cout << "Client: " << received << std::endl;

        if (received == "exit")
            break;

        std::string response = "Pong: " + received;
        conn->send(response);
    }

    std::cout << "Server shutting down..." << std::endl;
}
