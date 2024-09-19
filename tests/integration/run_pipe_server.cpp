
#include "simple_socket/Pipe.hpp"

int main() {
    PipeServer server(R"(\\.\pipe\PingPongPipe)");
    server.accept();

    std::vector<unsigned char> buffer(512);
    // Ping-Pong logic
    while (true) {
        const auto len = server.receive(buffer);
        std::string received{buffer.begin(), buffer.begin() + len};
        std::cout << "Client: " << received << std::endl;

        if (received == "exit")
            break;

        std::string response = "Pong: " + received;
        server.send(response);
    }

    std::cout << "Server shutting down..." << std::endl;
}
