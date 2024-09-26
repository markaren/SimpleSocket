
#include <catch2/catch_test_macros.hpp>

#include "simple_socket/byte_conversion.hpp"

using namespace simple_socket;

TEST_CASE("int_to_bytes converts integers to byte arrays correctly", "[int_to_bytes]") {
    constexpr int num = 305419896; // 0x12345678 in hexadecimal
    constexpr std::array<unsigned char, 4> expected_little_endian = {0x78, 0x56, 0x34, 0x12};
    constexpr std::array<unsigned char, 4> expected_big_endian = {0x12, 0x34, 0x56, 0x78};

    SECTION("Little-endian conversion") {
        const auto result = encode_uint32(num, ByteOrder::LITTLE);
        REQUIRE(result == expected_little_endian);
    }

    SECTION("Big-endian conversion") {
        const auto result = encode_uint32(num, ByteOrder::BIG);
        REQUIRE(result == expected_big_endian);
    }
}

TEST_CASE("bytes_to_int converts byte arrays to integers correctly", "[bytes_to_int]") {
    constexpr std::array<unsigned char, 4> bytes_little_endian = {0x78, 0x56, 0x34, 0x12};
    constexpr std::array<unsigned char, 4> bytes_big_endian = {0x12, 0x34, 0x56, 0x78};
    constexpr int expected_num = 305419896; // 0x12345678 in hexadecimal

    SECTION("Little-endian conversion") {
        constexpr int result = decode_uint32(bytes_little_endian, ByteOrder::LITTLE);
        REQUIRE(result == expected_num);
    }

    SECTION("Big-endian conversion") {
        constexpr int result = decode_uint32(bytes_big_endian, ByteOrder::BIG);
        REQUIRE(result == expected_num);
    }
}
