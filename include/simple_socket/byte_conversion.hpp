
#ifndef SIMPLE_SOCKET_BYTE_CONVERSION_HPP
#define SIMPLE_SOCKET_BYTE_CONVERSION_HPP

#include <array>
#include <bit>

namespace simple_socket {

    constexpr std::array<uint8_t, 4> encode_uint32(uint32_t n, std::endian order = std::endian::native) {
        std::array<uint8_t, 4> bytes{};

        bytes[order == std::endian::little ? 0 : 3] = n & 0xFF;
        bytes[order == std::endian::little ? 1 : 2] = n >> 8 & 0xFF;
        bytes[order == std::endian::little ? 2 : 1] = n >> 16 & 0xFF;
        bytes[order == std::endian::little ? 3 : 0] = n >> 24 & 0xFF;

        return bytes;
    }

    template<typename ArrayLike>
    constexpr uint32_t decode_uint32(ArrayLike buffer, std::endian order = std::endian::native) {

        return static_cast<uint32_t>(buffer[order == std::endian::little ? 0 : 3] |
                                     buffer[order == std::endian::little ? 1 : 2] << 8 |
                                     buffer[order == std::endian::little ? 2 : 1] << 16 |
                                     buffer[order == std::endian::little ? 3 : 0] << 24);
    }

}// namespace simple_socket

#endif//SIMPLE_SOCKET_BYTE_CONVERSION_HPP
