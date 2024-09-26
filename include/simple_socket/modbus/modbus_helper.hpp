
#ifndef MODBUS_HELPER_HPP
#define MODBUS_HELPER_HPP

#include "simple_socket/ByteOrder.hpp"

#include <vector>

namespace simple_socket {

    // Decode 32-bit unsigned integer (4 bytes)
    inline uint32_t decode_uint32(const std::vector<uint16_t>& data, ByteOrder order = DEFAULT_BYTE_ORDER) {
        return (static_cast<uint32_t>(data[order == ByteOrder::BIG ? 0 : 1]) << 16) |
               static_cast<uint32_t>(data[order == ByteOrder::BIG ? 1 : 0]);
    }

    // Decode IEEE 754 32-bit float (4 bytes)
    inline float decode_float(const std::vector<uint16_t>& data, ByteOrder order = DEFAULT_BYTE_ORDER) {
        uint32_t raw_value = decode_uint32(data, order);
        float decoded_value;
        std::memcpy(&decoded_value, &raw_value, sizeof(decoded_value));// Reinterpret the bits as float
        return decoded_value;
    }

    inline std::vector<uint16_t> encode_uint32(uint32_t value, ByteOrder order = DEFAULT_BYTE_ORDER) {
        std::vector<uint16_t> data(2);

        data[order == ByteOrder::BIG ? 0 : 1] = (static_cast<uint16_t>((value >> 16) & 0xFFFF));// High 16 bits
        data[order == ByteOrder::BIG ? 1 : 0] = (static_cast<uint16_t>(value & 0xFFFF));        // Low 16 bit

        return data;
    }

    inline std::vector<uint16_t> encode_float(float value, ByteOrder order = DEFAULT_BYTE_ORDER) {
        uint32_t raw_value;
        std::memcpy(&raw_value, &value, sizeof(raw_value)); // Convert float to uint32_t
        return encode_uint32(raw_value, order);
    }

}// namespace simple_socket

#endif//MODBUS_HELPER_HPP
