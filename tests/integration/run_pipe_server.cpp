
#include "simple_socket/Pipe.hpp"

#include <iostream>
#include <array>

using namespace simple_socket;

int main() {

    std::cout << "Waiting for client connection..." << std::endl;
    const auto conn = NamedPipe::listen("PingPongPipe");

    if (!conn) return 1;

    std::array<unsigned char, 512> buffer{};
    // Ping-Pong logic
    while (true) {
        const auto len = conn->write(buffer);
        std::string received{buffer.begin(), buffer.begin() + len};
        std::cout << "Client: " << received << std::endl;

        if (received == "exit")
            break;

        std::string response = "Pong: " + received;
        conn->write(response);
    }

    std::cout << "Server shutting down..." << std::endl;
}
