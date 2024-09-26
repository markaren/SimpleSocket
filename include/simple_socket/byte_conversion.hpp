
#ifndef SIMPLE_SOCKET_BYTE_CONVERSION_HPP
#define SIMPLE_SOCKET_BYTE_CONVERSION_HPP

#include <array>
#include <cstring>
#include <stdexcept>

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

    // Decode 32-bit unsigned integer (4 bytes)
    inline uint32_t decode_uint32(const std::vector<uint16_t>& data, size_t index = 0) {
        if (index + 1 >= data.size()) {
            throw std::out_of_range("Invalid index for uint32_t decoding.");
        }
        return (static_cast<uint32_t>(data[index]) << 16) |
               static_cast<uint32_t>(data[index + 1]);
    }

    // Decode IEEE 754 32-bit float (4 bytes)
    inline float decode_float(const std::vector<uint16_t>& data, size_t index = 0) {
        if (index + 1 >= data.size()) {
            throw std::out_of_range("Invalid index for float decoding.");
        }
        uint32_t raw_value = decode_uint32(data, index);
        float decoded_value;
        std::memcpy(&decoded_value, &raw_value, sizeof(decoded_value));// Reinterpret the bits as float
        return decoded_value;
    }

}// namespace simple_socket

#endif//SIMPLE_SOCKET_BYTE_CONVERSION_HPP
