
#include <catch2/catch_test_macros.hpp>

#include "../include/simple_socket/ws/WebSocket.hpp"

#include <algorithm>
#include <execution>
#include <iostream>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>


using namespace simple_socket;

TEST_CASE("Echo WebSocketClient wss") {

    std::mutex mutex;

    std::vector<WebSocketClient> clients(5);
    for (auto& client : clients) {

        std::binary_semaphore latch{0};
        std::string ping = "Hello, WebSocket!";

        client.onOpen = [&](WebSocketConnection* ws) {
            std::cout << "Connection opened." << std::endl;
            ws->send(ping);
        };

        client.onMessage = [&](WebSocketConnection*, const std::string& msg) {
            {
                std::lock_guard lock(mutex);
                std::cout << std::this_thread::get_id() << " Received message: " << msg << std::endl;
            }
            if (msg == ping) {
                latch.release();
            }
        };

        client.onClose = [&](WebSocketConnection* ws) {
            std::lock_guard lock(mutex);
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