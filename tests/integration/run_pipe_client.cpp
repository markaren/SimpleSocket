
#include "simple_socket/Pipe.hpp"

int main() {
    PipeClient client;
    if (!client.connect(R"(\\.\pipe\PingPongPipe)", 500)) {
        return 1;
    }

    // Ping-Pong logic
    std::string input;
    while (true) {
        std::cout << "Enter message: ";
        std::getline(std::cin, input);

        client.send(input);
        if (input == "exit")
            break;

        std::vector<unsigned char> buffer(512);
        const auto bytesRecevied = client.receive(buffer);
        std::string received{buffer.begin(), buffer.begin() + bytesRecevied};
        std::cout << "Server: " << received << std::endl;
    }

    std::cout << "Client shutting down..." << std::endl;
}
