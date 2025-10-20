
#include "simple_socket/WebSocket.hpp"
#include "simple_socket/util/port_query.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <catch2/catch_test_macros.hpp>

using namespace simple_socket;

TEST_CASE("test Websocket") {

    const auto port = getAvailablePort(8000, 9000);
    REQUIRE(port);

    std::atomic_bool serverOpen{false};
    std::atomic_bool serverClose{false};
    std::string servermsg;

    std::atomic_bool clientOpen{false};
    std::atomic_bool clientClose{false};
    std::string clientmsg;

    std::mutex m;
    std::condition_variable cv;

    WebSocket ws(*port);
    ws.start();

    ws.onOpen = [&](auto c) {
        serverOpen = true;
        c->send("Hello from server!");
        cv.notify_one();
    };

    ws.onMessage = [&](auto, const auto& msg) {
        servermsg = msg;
        cv.notify_one();
    };

    ws.onClose = [&](auto) {
        serverClose = true;
        cv.notify_one();
    };


    WebSocketClient client;
    client.onOpen = [&](auto c) {
        clientOpen = true;
        c->send("Hello from client!");
        cv.notify_one();
    };

    client.onMessage = [&](auto, const auto& msg) {
        clientmsg = msg;
        cv.notify_one();
    };

    client.onClose = [&](auto) {
        clientClose = true;
        cv.notify_one();
    };

    client.connect("ws://127.0.0.1:" + std::to_string(*port));

    std::unique_lock lock(m);
    cv.wait(lock, [&]() {
        return clientOpen.load() && serverOpen.load();
    });


    cv.wait(lock, [&]() {
        return !servermsg.empty() && !clientmsg.empty();
    });

    client.close();


    cv.wait(lock, [&]() {
        return clientClose.load() && serverClose.load();
    });
    ws.stop();

    CHECK(serverOpen);
    CHECK(serverClose);
    CHECK(clientOpen);
    CHECK(clientClose);

    CHECK(servermsg == "Hello from client!");
    CHECK(clientmsg == "Hello from server!");
}
