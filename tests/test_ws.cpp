
#include "simple_socket/WebSocket.hpp"
#include "simple_socket/util/port_query.hpp"

#include <atomic>
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

    WebSocket ws(*port);
    ws.start();


    ws.onOpen = [&](auto c) {
        serverOpen = true;
        c->send("Hello from server!");
    };

    ws.onMessage = [&](auto c, const auto& msg) {
        servermsg = msg;
    };

    ws.onClose = [&](auto c) {
        serverClose = true;
    };


    WebSocketClient client;
    client.onOpen = [&](auto c) {
        clientOpen = true;
        c->send("Hello from client!");
    };

    client.onMessage = [&](auto c, const auto& msg) {
        clientmsg = msg;
    };

    client.onClose = [&](auto c) {
        clientClose = true;
    };

    client.connect("127.0.0.1", *port);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    client.close();
    ws.stop();

    CHECK(serverOpen == true);
    CHECK(serverClose == true);
    CHECK(clientOpen == true);
    CHECK(clientClose == true);

    CHECK(servermsg == "Hello from client!");
    CHECK(clientmsg == "Hello from server!");
}
