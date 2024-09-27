
#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/util/port_query.hpp"

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

    void socketHandler(std::unique_ptr<SimpleConnection> conn) {

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

    const auto port = getAvailablePort(8000, 9000);
    REQUIRE(port != -1);

    TCPServer server(port);

    std::thread serverThread([&server] {
        std::unique_ptr<SimpleConnection> conn;
        REQUIRE_NOTHROW(conn = server.accept());
        socketHandler(std::move(conn));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread clientThread([port] {
        TCPClientContext client;
        const auto conn = client.connect("127.0.0.1", port);
        REQUIRE(conn);

        std::string message = generateMessage();
        std::string expectedResponse = generateResponse(message);

        conn->write(message);

        std::vector<unsigned char> buffer(1024);
        const auto bytesRead = conn->read(buffer);
        REQUIRE(bytesRead == expectedResponse.size());
        std::string response(buffer.begin(), buffer.begin() + bytesRead);

        CHECK(response == expectedResponse);
    });

    clientThread.join();

    server.close();
    serverThread.join();
}

TEST_CASE("TCP readexact/write") {

    const auto port = getAvailablePort(8000, 9000);
    REQUIRE(port != -1);

    TCPServer server(port);

    std::thread serverThread([&server] {
        std::unique_ptr<SimpleConnection> conn;
        REQUIRE_NOTHROW(conn = server.accept());
        socketHandler(std::move(conn));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread clientThread([port] {
        TCPClientContext client;
        const auto conn = client.connect("127.0.0.1", port);
        REQUIRE(conn);

        std::string message = generateMessage();
        std::string expectedResponse = generateResponse(message);
        conn->write(message);

        std::vector<unsigned char> buffer(expectedResponse.size());
        REQUIRE(conn->readExact(buffer));

        std::string response(buffer.begin(), buffer.end());
        CHECK(response == expectedResponse);
    });

    clientThread.join();

    server.close();
    serverThread.join();
}
