
#include "simple_socket/UnixDomainSocket.hpp"

#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

using namespace simple_socket;

namespace {

#ifdef _WIN32
    const std::string domain{"afunix_socket"};
#else
    const std::string domain{"/tmp/unix_socket"};
#endif

    std::string generateMessage() {

        return "Per";
    }

    std::string generateResponse(const std::string& msg) {

        return "Hello " + msg + "!";
    }

    void socketHandler(std::unique_ptr<SocketConnection> conn) {

        std::vector<unsigned char> buffer(1024);
        const auto bytesRead = conn->read(buffer);

        std::string expectedMessage = generateMessage();
        REQUIRE(bytesRead == expectedMessage.size());

        std::string msg(buffer.begin(), buffer.begin() + bytesRead);
        REQUIRE(msg == expectedMessage);

        REQUIRE(conn->write(generateResponse(msg)));
    }

}// namespace

TEST_CASE("UNIX Domain Socket read/write") {

    UnixDomainServer server(domain);
    UnixDomainClient client;

    std::thread serverThread([&server] {
        std::unique_ptr<SocketConnection> conn;
        REQUIRE_NOTHROW(conn = server.accept());
        socketHandler(std::move(conn));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread clientThread([&client] {
        REQUIRE(client.connect(domain));

        std::string message = generateMessage();
        std::string expectedResponse = generateResponse(message);

        client.write(message);

        std::vector<unsigned char> buffer(1024);
        const auto bytesRead = client.read(buffer);
        REQUIRE(bytesRead == expectedResponse.size());
        std::string response(buffer.begin(), buffer.begin() + bytesRead);

        CHECK(response == expectedResponse);
    });

    clientThread.join();
    client.close();

    REQUIRE(!server.write(""));

    server.close();
    serverThread.join();
}


TEST_CASE("UNIX Domain Socket readexact/write") {

    UnixDomainServer server(domain);
    UnixDomainClient client;

    std::thread serverThread([&server] {
        std::unique_ptr<SocketConnection> conn;
        REQUIRE_NOTHROW(conn = server.accept());
        socketHandler(std::move(conn));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread clientThread([&client] {
        REQUIRE(client.connect(domain));

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
