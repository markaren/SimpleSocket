
#ifndef SIMPLE_SOCKET_BYTE_CONVERSION_HPP
#define SIMPLE_SOCKET_BYTE_CONVERSION_HPP

#include "simple_socket/ByteOrder.hpp"

#include <array>

namespace simple_socket {

    constexpr std::array<uint8_t, 4> encode_uint32(uint32_t n, ByteOrder order = DEFAULT_BYTE_ORDER) {
        std::array<uint8_t, 4> bytes{};

        bytes[order == ByteOrder::LITTLE ? 0 : 3] = n & 0xFF;
        bytes[order == ByteOrder::LITTLE ? 1 : 2] = n >> 8 & 0xFF;
        bytes[order == ByteOrder::LITTLE ? 2 : 1] = n >> 16 & 0xFF;
        bytes[order == ByteOrder::LITTLE ? 3 : 0] = n >> 24 & 0xFF;

        return bytes;
    }

    template<typename ArrayLike>
    constexpr uint32_t decode_uint32(ArrayLike buffer, ByteOrder order = DEFAULT_BYTE_ORDER) {

        return static_cast<uint32_t>(buffer[order == ByteOrder::LITTLE ? 0 : 3] |
                                     buffer[order == ByteOrder::LITTLE ? 1 : 2] << 8 |
                                     buffer[order == ByteOrder::LITTLE ? 2 : 1] << 16 |
                                     buffer[order == ByteOrder::LITTLE ? 3 : 0] << 24);
    }

}// namespace simple_socket

#endif//SIMPLE_SOCKET_BYTE_CONVERSION_HPP
