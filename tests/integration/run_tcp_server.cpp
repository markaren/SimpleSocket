#include "simple_socket/TCPSocket.hpp"

#include <atomic>
#include <iostream>
#include <thread>
#include <array>

using namespace simple_socket;

void socketHandler(std::unique_ptr<SimpleConnection> conn) {

    std::array<unsigned char, 1024> buffer{};
    const auto n = conn->read(buffer);

    std::string msg{buffer.begin(), buffer.begin() + n};
    std::cout << "Received from client: " << msg << std::endl;
    conn->write("Hello " + msg + "!");
}

int main() {

    TCPServer server(8080);

    std::atomic_bool stop = false;
    std::thread t([&] {
        while (!stop) {
            std::unique_ptr<SimpleConnection> conn;
            try {
                conn = server.accept();
            } catch (const std::exception&) {
                continue;
            }

            socketHandler(std::move(conn));
        }
    });

    std::cout << "Press and key to stop server.." << std::endl;
    std::cin.get();
    stop = true;
    server.close();

    t.join();
}
