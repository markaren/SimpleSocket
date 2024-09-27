
#include "simple_socket/UDPSocket.hpp"
#include "simple_socket/util/port_query.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace simple_socket;

TEST_CASE("Test UDP") {

    std::string address{"127.0.0.1"};
    const auto serverPort = getAvailablePort(8000, 9000);
    const auto clientPort = getAvailablePort(8000, 9000, {*serverPort});

    REQUIRE(serverPort);
    REQUIRE(clientPort);

    UDPSocket socket1(*serverPort);
    UDPSocket socket2(*clientPort);

    REQUIRE(socket1.sendTo(address, *clientPort, "Hello"));

    std::vector<unsigned char> buffer(1024);
    const auto read = socket2.recvFrom(address, *serverPort, buffer);
    REQUIRE(read != -1);

    std::string result(buffer.begin(), buffer.begin() + read);
    REQUIRE(result == "Hello");

    REQUIRE(socket2.sendTo(address, *serverPort, "World"));
    REQUIRE(socket1.recvFrom(address, *clientPort, buffer) == 5);

    REQUIRE(socket2.sendTo(address, *serverPort, std::vector<unsigned char>{'H', 'e', 'l', 'l', 'o'}));
    REQUIRE(socket1.recvFrom(address, *clientPort, buffer) == 5);

    REQUIRE(socket1.sendTo(address, *clientPort, "Hello World!"));
    REQUIRE(socket2.recvFrom(address, *serverPort) == "Hello World!");

    std::vector<unsigned char> largeBuffer(MAX_UDP_PACKET_SIZE);
    REQUIRE(socket1.sendTo(address, *clientPort, largeBuffer));
    REQUIRE(socket2.recvFrom(address, *serverPort).size() == MAX_UDP_PACKET_SIZE);

    std::vector<unsigned char> toLargeBuffer(MAX_UDP_PACKET_SIZE+1);
    REQUIRE(!socket1.sendTo(address, *clientPort, toLargeBuffer));
}

TEST_CASE("Test UDP SimpleConnection") {

    std::string address{"127.0.0.1"};
    const auto serverPort = getAvailablePort(8000, 9000);
    const auto clientPort = getAvailablePort(8000, 9000, {*serverPort});

    REQUIRE(serverPort);
    REQUIRE(clientPort);

    UDPSocket socket1(*serverPort);
    UDPSocket socket2(*clientPort);

    auto conn1 = socket1.makeConnection(address, *clientPort);
    auto conn2 = socket2.makeConnection(address, *serverPort);

    REQUIRE(conn1->write("Hello"));

    std::vector<unsigned char> buffer(1024);
    const auto read = conn2->read(buffer);
    REQUIRE(read != -1);

    std::string result(buffer.begin(), buffer.begin() + read);
    REQUIRE(result == "Hello");

    REQUIRE(conn2->write( "World"));
    REQUIRE(conn1->read(buffer) == 5);

    REQUIRE(conn2->write(std::vector<unsigned char>{'H', 'e', 'l', 'l', 'o'}));
    REQUIRE(conn1->read(buffer) == 5);

    REQUIRE(socket1.sendTo(address, *clientPort, "Hello World!"));
    REQUIRE(socket2.recvFrom(address, *serverPort) == "Hello World!");

    std::vector<unsigned char> largeBuffer(MAX_UDP_PACKET_SIZE);
    REQUIRE(conn1->write(largeBuffer));
    REQUIRE(conn2->read(largeBuffer) == MAX_UDP_PACKET_SIZE);

    std::vector<unsigned char> toLargeBuffer(MAX_UDP_PACKET_SIZE+1);
    REQUIRE(!conn1->write(toLargeBuffer));
}
