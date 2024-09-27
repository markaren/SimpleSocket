
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "simple_socket/modbus/HoldingRegister.hpp"
#include "simple_socket/modbus/ModbusServer.hpp"
#include "simple_socket/modbus/ModbusClient.hpp"

#include "simple_socket/util/port_query.hpp"


using namespace simple_socket;

// Test fixture for HoldingRegister
TEST_CASE("HoldingRegister basic operations", "[HoldingRegister]") {
    SECTION("Initialization and size check") {
        HoldingRegister reg(10);
        REQUIRE(reg.size() == 10); // Check if size is correct after initialization
    }

    SECTION("Setting and getting uint16_t values") {
        HoldingRegister reg(10);

        // Set and get uint16_t at valid index
        reg.setUint16(0, 0x1234);
        REQUIRE(reg.getUint16(0) == 0x1234);

        // Check out-of-bounds behavior
        REQUIRE_THROWS_AS(reg.setUint16(10, 0x1234), std::out_of_range); // Index out of bounds
        REQUIRE_THROWS_AS(reg.getUint16(10), std::out_of_range);         // Index out of bounds
    }

    SECTION("Setting and getting uint32_t values") {
        HoldingRegister reg(10);

        // Set and get uint32_t at valid index
        reg.setUint32(1, 0x12345678);
        REQUIRE(reg.getUint32(1) == 0x12345678);

        // Check that high and low parts are correctly placed
        REQUIRE(reg.getUint16(1) == 0x1234); // High 16 bits
        REQUIRE(reg.getUint16(2) == 0x5678); // Low 16 bits

        // Check out-of-bounds behavior
        REQUIRE_THROWS_AS(reg.setUint32(9, 0x12345678), std::out_of_range); // Not enough space for two registers
        REQUIRE_THROWS_AS(reg.getUint32(9), std::out_of_range);             // Not enough space for two registers
    }

    SECTION("Setting and getting float values") {
        HoldingRegister reg(10);
        float value = 3.14159f;

        // Set and get float at valid index
        reg.setFloat(0, value);
        REQUIRE_THAT(reg.getFloat(0), Catch::Matchers::WithinRel(value));

        // Check out-of-bounds behavior
        REQUIRE_THROWS_AS(reg.setFloat(9, value), std::out_of_range); // Not enough space for two registers
        REQUIRE_THROWS_AS(reg.getFloat(9), std::out_of_range);        // Not enough space for two registers
    }

}

TEST_CASE("Test modbus client/server") {

    const auto port = getAvailablePort(502, 600);
    REQUIRE(port != -1);

    uint16_t v1 = 1;
    uint32_t v2 = 2;
    float v3 = 3.1415f;

    HoldingRegister reg(5);
    reg.setUint16(0, v1);
    reg.setUint32(1, v2);
    reg.setFloat(3, v3);

    ModbusServer server(reg, port);
    server.start();

    ModbusClient client("127.0.0.1", port);
    REQUIRE(client.read_uint16(0) == v1);
    REQUIRE(client.read_uint32(1) == v2);
    REQUIRE_THAT(client.read_float(3), Catch::Matchers::WithinRel(v3));

    server.stop();

}
