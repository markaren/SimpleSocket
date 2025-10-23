
#include "../../include/simple_socket/ws/WebSocket.hpp"

#include <iostream>

using namespace simple_socket;

int main() {

    WebSocket ws(8081);
    ws.start();

    ws.onOpen = [](auto c) {
        c->send("Hello from server!");
        std::cout << "[" << c->uuid() << "] onOpen" << std::endl;
    };
    ws.onClose = [](auto c) {
        std::cout << "[" << c->uuid() << "] onClose" << std::endl;
    };
    ws.onMessage = [](auto c, const auto& msg) {
        std::cout << "[" << c->uuid() << "] onMessage: " << msg << std::endl;
        c->send(msg);
    };

#ifdef _WIN32
    system("start ws_client.html");
#endif

    //wait for key press
    std::cout << "Press any key to stop server.." << std::endl;
    std::cin.get();

    ws.stop();
}
