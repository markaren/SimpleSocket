
#include "Socket.hpp"

#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

namespace {

    std::string generateMessage() {

        return "Per";
    }

    std::string generateResponse(const std::string &msg) {

        return "Hello " + msg + "!";
    }

    void socketHandler(std::unique_ptr<Connection> conn) {

        size_t bytesRead = 0;
        std::vector<unsigned char> buffer(1024);
        REQUIRE(conn->read(buffer, &bytesRead));

        std::string expectedMessage = generateMessage();
        REQUIRE(bytesRead == expectedMessage.size());

        std::string msg(buffer.begin(), buffer.begin() + static_cast<int>(bytesRead));
        REQUIRE(msg == expectedMessage);

        REQUIRE(conn->write(generateResponse(msg)));
    }
}// namespace

TEST_CASE("socketlib") {

    ServerSocket server;
    ClientSocket client;

    std::thread serverThread([&server] {
        server.bind(8080);
        server.listen();

        std::unique_ptr<Connection> conn;
        REQUIRE_NOTHROW(conn = server.accept());
        socketHandler(std::move(conn));
    });

    std::thread clientThread([&client] {
        REQUIRE(client.connect("127.0.0.1", 8080));

        std::string message = "Per";
        client.write(message);

        size_t bytesRead = 0;
        std::vector<unsigned char> buffer(1024);
        REQUIRE(client.read(buffer, &bytesRead));

        std::string expectedResponse = generateResponse(message);
        REQUIRE(bytesRead == expectedResponse.size());

        std::string response(buffer.begin(), buffer.begin() + static_cast<int>(bytesRead));

        CHECK(response == expectedResponse);
    });

    clientThread.join();
    server.close();
    serverThread.join();
}
