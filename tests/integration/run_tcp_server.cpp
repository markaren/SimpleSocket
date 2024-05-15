#include "TCPSocket.hpp"

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

void socketHandler(std::unique_ptr<Connection> conn) {
    size_t n;
    std::vector<unsigned char> buffer(1024);
    conn->read(buffer, &n);

    std::string msg(buffer.begin(), buffer.begin() + static_cast<int>(n));
    std::cout << "Received from client: " << msg << std::endl;
    conn->write("Hello " + msg + "!");
}

int main() {

    TCPServer server;
    server.bind(8080);
    server.listen();

    std::atomic_bool stop = false;
    std::thread t([&] {
        while (!stop) {
            std::unique_ptr<Connection> conn;
            try {
                conn = server.accept();
            } catch (const std::exception& e) {
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