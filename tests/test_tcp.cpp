
#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/port_query.hpp"

#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

using namespace simple_socket;

namespace {

    std::string generateMessage() {

        return "Per";
    }

    std::string generateResponse(const std::string& msg) {

        return "Hello " + msg + "!";
    }

    void socketHandler(std::unique_ptr<TCPConnection> conn) {

        std::vector<unsigned char> buffer(1024);
        const auto bytesRead = conn->read(buffer);

        std::string expectedMessage = generateMessage();
        REQUIRE(bytesRead == expectedMessage.size());

        std::string msg(buffer.begin(), buffer.begin() + bytesRead);
        REQUIRE(msg == expectedMessage);

        REQUIRE(conn->write(generateResponse(msg)));
    }

}// namespace

TEST_CASE("TCP read/write") {

    int port = getAvailablePort(8000, 9000);
    REQUIRE(port != -1);

    TCPServer server(port);
    TCPClient client;

    std::thread serverThread([&server] {
        std::unique_ptr<TCPConnection> conn;
        REQUIRE_NOTHROW(conn = server.accept());
        socketHandler(std::move(conn));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread clientThread([&client, port] {
        REQUIRE(client.connect("127.0.0.1", port));

        std::string message = generateMessage();
        std::string expectedResponse = generateResponse(message);

        client.write(message);

        std::vector<unsigned char> buffer(1024);
        const auto bytesRead = client.read(buffer);
        REQUIRE(bytesRead == expectedResponse.size());
        std::string response(buffer.begin(), buffer.begin() + static_cast<int>(bytesRead));

        CHECK(response == expectedResponse);
    });

    clientThread.join();
    client.close();

    REQUIRE(!server.write(""));

    server.close();
    serverThread.join();
}

TEST_CASE("TCP readexact/write") {

    int port = getAvailablePort(8000, 9000);
    REQUIRE(port != -1);

    TCPServer server(port);
    TCPClient client;

    std::thread serverThread([&server] {
        std::unique_ptr<TCPConnection> conn;
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
    client.close();

    REQUIRE(!server.write(""));

    server.close();
    serverThread.join();
}
