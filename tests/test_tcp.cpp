
#include "TCPSocket.hpp"

#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

namespace {

    std::string generateMessage() {

        return "Per";
    }

    std::string generateResponse(const std::string& msg) {

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

TEST_CASE("TCP read/write") {

    TCPServer server;
    TCPClient client;

    int port = 8080;

    std::thread serverThread([&server, port] {
        server.bind(port);
        server.listen();

        std::unique_ptr<Connection> conn;
        REQUIRE_NOTHROW(conn = server.accept());
        socketHandler(std::move(conn));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread clientThread([&client, port] {
        REQUIRE(client.connect("127.0.0.1", port));

        std::string message = generateMessage();
        std::string expectedResponse = generateResponse(message);

        client.write(message);

        size_t bytesRead = 0;
        std::vector<unsigned char> buffer(1024);
        REQUIRE(client.read(buffer, &bytesRead));

        REQUIRE(bytesRead == expectedResponse.size());
        std::string response(buffer.begin(), buffer.begin() + static_cast<int>(bytesRead));

        CHECK(response == expectedResponse);
    });

    clientThread.join();
    server.close();
    serverThread.join();
}

TEST_CASE("TCP readexact/write") {

    TCPServer server;
    TCPClient client;

    int port = 8080;

    std::thread serverThread([&server, port] {
        server.bind(port);
        server.listen();

        std::unique_ptr<Connection> conn;
        REQUIRE_NOTHROW(conn = server.accept());
        socketHandler(std::move(conn));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread clientThread([&client, port] {
        REQUIRE(client.connect("127.0.0.1", port));

        std::string message = generateMessage();
        std::string expectedResponse = generateResponse(message);
        client.write(message);

        std::vector<unsigned char> buffer(expectedResponse.size());
        REQUIRE(client.readExact(buffer));

        std::string response(buffer.begin(), buffer.end());

        CHECK(response == expectedResponse);
    });

    clientThread.join();
    server.close();
    serverThread.join();
}
