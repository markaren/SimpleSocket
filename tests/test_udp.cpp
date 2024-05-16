
#include "UDPSocket.hpp"
#include "WSASession.hpp"
#include "AvailablePortQuery.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Test UDP") {

    WSASession session;

    std::string address{"127.0.0.1"};
    int serverPort = getAvailablePort(8000, 9000);
    int clientPort = getAvailablePort(8000, 9000, {serverPort});

    REQUIRE(serverPort != -1);
    REQUIRE(clientPort != -1);

    UDPSocket socket1(serverPort);
    UDPSocket socket2(clientPort);

    REQUIRE(socket1.sendTo(address, clientPort, "Hello"));

    std::vector<unsigned char> buffer(1024);
    int read = socket2.recvFrom(address, serverPort, buffer);

    REQUIRE(read != -1);

    std::string result(buffer.begin(), buffer.begin() + read);

    REQUIRE(result == "Hello");
}
