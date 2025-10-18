
#include <catch2/catch_test_macros.hpp>

#include "simple_socket/WebSocket.hpp"

#include <chrono>
#include <iostream>
#include <semaphore>
#include <thread>


using namespace simple_socket;
using namespace std::chrono_literals;

TEST_CASE("Echo WebSocketClient wss") {
    for (int i = 0; i < 5; ++i) {
        std::binary_semaphore latch{0};

        WebSocketClient client;
        std::string ping = "Hello, WebSocket!";

        client.onOpen = [&](WebSocketConnection* ws) {
            std::cout << "Connection opened." << std::endl;
            ws->send(ping);
        };

        client.onMessage = [&](WebSocketConnection*, const std::string& msg) {
            std::cout << "Received message: " << msg << std::endl;
            if (msg == ping) {
                latch.release();
            }
        };

        client.onClose = [&](WebSocketConnection* ws) {
            std::cout << "Connection closed." << std::endl;
        };

        client.connect("wss://echo.websocket.org");

        while (!latch.try_acquire()) {

            std::this_thread::yield();
        }

        REQUIRE(true);

        client.close();
    }
}