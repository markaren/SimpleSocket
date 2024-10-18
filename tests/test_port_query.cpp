
#include <catch2/catch_test_macros.hpp>

#include "simple_socket/util/port_query.hpp"

#include "simple_socket/TCPSocket.hpp"

TEST_CASE("test port query") {

    {
        auto p1 = simple_socket::getAvailablePort(8000, 9000);
        REQUIRE(p1);
        simple_socket::TCPServer s1(*p1);
        auto p2 = simple_socket::getAvailablePort(8000, 9000);
        REQUIRE(p2);
        REQUIRE(*p1 != *p2);
    }

    {
        auto p1 = simple_socket::getAvailablePort(8000, 9000);
        REQUIRE(p1);
        simple_socket::TCPServer s1(*p1);
        auto p2 = simple_socket::getAvailablePort(8000, 9000);
        REQUIRE(p2);
        REQUIRE(*p1 != *p2);
    }
}