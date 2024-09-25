
#ifndef SIMPLE_SOCKET_BYTE_CONVERSION_HPP
#define SIMPLE_SOCKET_BYTE_CONVERSION_HPP

#include <array>

namespace simple_socket {

    enum class byte_order {
        LITTLE,
        BIG
    };

    constexpr std::array<unsigned char, 4> int_to_bytes(int n, byte_order order = byte_order::LITTLE) {
        std::array<unsigned char, 4> bytes{};

        if (order == byte_order::LITTLE) {
            bytes[0] = n & 0xFF;
            bytes[1] = (n >> 8) & 0xFF;
            bytes[2] = (n >> 16) & 0xFF;
            bytes[3] = (n >> 24) & 0xFF;
        } else {
            bytes[0] = (n >> 24) & 0xFF;
            bytes[1] = (n >> 16) & 0xFF;
            bytes[2] = (n >> 8) & 0xFF;
            bytes[3] = n & 0xFF;
        }

        return bytes;
    }

    template<typename ArrayLike>
    constexpr int bytes_to_int(ArrayLike buffer, byte_order order = byte_order::LITTLE) {
        if (order == byte_order::LITTLE) {
            return static_cast<int>(buffer[0] |
                                    buffer[1] << 8 |
                                    buffer[2] << 16 |
                                    buffer[3] << 24);
        }
        return static_cast<int>(buffer[0] << 24 |
                                buffer[1] << 16 |
                                buffer[2] << 8 |
                                buffer[3]);
    }

}

#endif//SIMPLE_SOCKET_BYTE_CONVERSION_HPP
