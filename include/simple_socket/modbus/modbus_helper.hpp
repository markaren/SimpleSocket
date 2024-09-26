
#ifndef SIMPLE_SOCKET_MODBUS_HELPER_HPP
#define SIMPLE_SOCKET_MODBUS_HELPER_HPP

#include "simple_socket/ByteOrder.hpp"

#include <sstream>
#include <vector>

namespace simple_socket {

    // Decode 32-bit unsigned integer (4 bytes)
    inline uint32_t decode_uint32(const std::vector<uint16_t>& data, ByteOrder order = DEFAULT_BYTE_ORDER) {
        return (static_cast<uint32_t>(data[order == ByteOrder::BIG ? 0 : 1]) << 16) |
               static_cast<uint32_t>(data[order == ByteOrder::BIG ? 1 : 0]);
    }

    // Decode 32-bit unsigned integer (4 bytes)
    inline std::string decode_text(const std::vector<uint16_t>& data, ByteOrder order = DEFAULT_BYTE_ORDER) {
        std::stringstream ss;
        for (const auto& byte : data) {
            uint8_t high_byte = (byte >> 8) & 0xFF;// Get the upper 8 bits
            uint8_t low_byte = byte & 0xFF;
            ss << (order == ByteOrder::BIG ? high_byte : low_byte) << (order == ByteOrder::BIG ? low_byte : high_byte);
        }
        return ss.str();
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
        std::memcpy(&raw_value, &value, sizeof(raw_value));// Convert float to uint32_t
        return encode_uint32(raw_value, order);
    }

    inline std::vector<uint16_t> encode_text(const std::string& data, ByteOrder order = DEFAULT_BYTE_ORDER) {

        std::vector<uint16_t> encoded_data;
        for (unsigned i = 0; i < data.size(); i += 2) {
            uint16_t value;
            if (i + 1 < data.size()) {
                if (order == ByteOrder::BIG) {
                    value = (static_cast<uint16_t>(data[i]) << 8) | static_cast<uint16_t>(data[i + 1]);
                } else {
                    value = (static_cast<uint16_t>(data[i + 1]) << 8) | static_cast<uint16_t>(data[i]);
                }
            } else {
                value = static_cast<uint16_t>(data[i]) << (order == ByteOrder::BIG ? 8 : 0);
            }

            encoded_data.emplace_back(value);
        }

        return encoded_data;
    }

}// namespace simple_socket

#endif//SIMPLE_SOCKET_MODBUS_HELPER_HPP
