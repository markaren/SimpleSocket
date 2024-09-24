
#include "simple_socket/WebSocket.hpp"

#include <iostream>

using namespace simple_socket;

int main() {

    WebSocket ws(8081);
    ws.onOpen = [](auto) {
        std::cout << "onOpen" << std::endl;
    };
    ws.onClose = [](auto) {
        std::cout << "onClose" << std::endl;
    };
    ws.onMessage = [](auto c, const auto& msg) {
        std::cout << "onMessage: " << msg << std::endl;
        c->send("Hello from server");
    };

    system("start ws_client.html");

    //wait for key press
    std::cout << "Press any key to stop server.." << std::endl;
    std::cin.get();
}
