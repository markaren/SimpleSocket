
#ifndef SIMPLE_SOCKET_MODBUS_HELPER_HPP
#define SIMPLE_SOCKET_MODBUS_HELPER_HPP

#include <sstream>
#include <vector>

namespace simple_socket {

    // Decode 32-bit unsigned integer (4 bytes)
    inline uint32_t decode_uint32(const std::vector<uint16_t>& data) {
        return (static_cast<uint32_t>(data[0]) << 16) |
               static_cast<uint32_t>(data[1]);
    }

    // Decode 32-bit unsigned integer (4 bytes)
    inline std::string decode_text(const std::vector<uint16_t>& data) {
        std::stringstream ss;
        for (const auto& byte : data) {
            const uint8_t high_byte = (byte >> 8) & 0xFF;// Get the upper 8 bits
            const uint8_t low_byte = byte & 0xFF;
            ss << high_byte << low_byte;
        }
        return ss.str();
    }

    // Decode IEEE 754 32-bit float (4 bytes)
    inline float decode_float(const std::vector<uint16_t>& data) {
        uint32_t raw_value = decode_uint32(data);
        float decoded_value;
        std::memcpy(&decoded_value, &raw_value, sizeof(decoded_value));// Reinterpret the bits as float
        return decoded_value;
    }

    inline std::vector<uint16_t> encode_uint32(uint32_t value) {
        std::vector<uint16_t> data(2);

        data[0] = (static_cast<uint16_t>((value >> 16) & 0xFFFF));// High 16 bits
        data[1] = (static_cast<uint16_t>(value & 0xFFFF));        // Low 16 bit

        return data;
    }

    inline std::vector<uint16_t> encode_float(float value) {
        uint32_t raw_value;
        std::memcpy(&raw_value, &value, sizeof(raw_value));// Convert float to uint32_t
        return encode_uint32(raw_value);
    }

    inline std::vector<uint16_t> encode_text(const std::string& data) {

        std::vector<uint16_t> encoded_data;
        for (unsigned i = 0; i < data.size(); i += 2) {
            uint16_t value;
            if (i + 1 < data.size()) {

                value = (static_cast<uint16_t>(data[i]) << 8) | static_cast<uint16_t>(data[i + 1]);

            } else {
                value = static_cast<uint16_t>(data[i]) << 8;
            }

            encoded_data.emplace_back(value);
        }

        return encoded_data;
    }

}// namespace simple_socket

#endif//SIMPLE_SOCKET_MODBUS_HELPER_HPP
