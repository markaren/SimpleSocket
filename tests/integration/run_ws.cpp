
#include "WebSocket.hpp"
#include "WSASession.hpp"

#include <iostream>

namespace {

    class MyWebSocket: public WebSocket {

    public:
        explicit MyWebSocket(uint16_t port): WebSocket(port) {}

        void onOpen(WebSocketConnection* conn) override {
            std::cout << "onOpen" << std::endl;
        }

        void onMessage(WebSocketConnection* conn, const std::string& msg) override {
            std::cout << "onMessage: " << msg << std::endl;
            conn->send("Hello from server");
        }

        void onClose(WebSocketConnection* conn) override {
            std::cout << "onClose" << std::endl;
        }
    };

}// namespace

int main() {

    WSASession session;

    MyWebSocket ws(8081);

    //wait for key press
    std::cout << "Press any key to stop server.." << std::endl;
    std::cin.get();

    ws.stop();
}