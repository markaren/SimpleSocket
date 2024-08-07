
#include "UDPSocket.hpp"
#include "AvailablePortQuery.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Test UDP") {

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

    REQUIRE(socket2.sendTo(address, serverPort, "World"));
    REQUIRE(socket1.recvFrom(address, clientPort, buffer) == 5);

    REQUIRE(socket2.sendTo(address, serverPort, std::vector<unsigned char>{'H', 'e', 'l', 'l', 'o'}));
    REQUIRE(socket1.recvFrom(address, clientPort, buffer) == 5);

    REQUIRE(socket1.sendTo(address, clientPort, "Hello World!"));
    REQUIRE(socket2.recvFrom(address, serverPort) == "Hello World!");

    std::vector<unsigned char> largeBuffer(MAX_UDP_PACKET_SIZE);
    REQUIRE(socket1.sendTo(address, clientPort, largeBuffer));
    REQUIRE(socket2.recvFrom(address, serverPort).size() == MAX_UDP_PACKET_SIZE);

    std::vector<unsigned char> toLargeBuffer(MAX_UDP_PACKET_SIZE+1);
    REQUIRE(!socket1.sendTo(address, clientPort, toLargeBuffer));
}
