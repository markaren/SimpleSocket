
#ifndef SIMPLE_SOCKET_MODBUS_HELPER_HPP
#define SIMPLE_SOCKET_MODBUS_HELPER_HPP

#include <array>
#include <cstring>
#include <sstream>
#include <vector>

namespace simple_socket {

    // Decode 32-bit unsigned integer (4 bytes)
    inline uint32_t decode_uint32(const std::vector<uint16_t>& data, size_t offset = 0) {
        return (static_cast<uint32_t>(data[offset + 0]) << 16) |
               static_cast<uint32_t>(data[offset + 1]);
    }

    // Decode 64-bit unsigned integer (8 bytes)
    inline uint32_t decode_uint64(const std::vector<uint16_t>& data, size_t offset = 0) {
        return (static_cast<uint64_t>(data[offset]) << 48) |
               (static_cast<uint64_t>(data[offset + 1]) << 32) |
               (static_cast<uint64_t>(data[offset + 2]) << 16) |
               (static_cast<uint64_t>(data[offset + 3]));
    }

    // Decode IEEE 754 32-bit float (4 bytes)
    inline float decode_float(const std::vector<uint16_t>& data, size_t offset = 0) {
        if (offset + 2 > data.size()) {
            throw std::out_of_range("Not enough data to decode a double.");
        }
        uint32_t raw_value = decode_uint32(data, offset);
        float decoded_value;
        std::memcpy(&decoded_value, &raw_value, sizeof(decoded_value));// Reinterpret the bits as float
        return decoded_value;
    }

    // Decode IEEE 754 64-bit double (8 bytes)
    inline double decode_double(const std::vector<uint16_t>& data, size_t offset = 0) {
        if (offset + 4 > data.size()) {
            throw std::out_of_range("Not enough data to decode a double.");
        }
        uint64_t raw_value = decode_uint64(data, offset);
        double decoded_value;
        std::memcpy(&decoded_value, &raw_value, sizeof(decoded_value));
        return decoded_value;
    }

    // Decode 32-bit unsigned integer (4 bytes)
    inline std::string decode_text(const std::vector<uint16_t>& data, size_t num, size_t offset = 0) {
        std::stringstream ss;
        for (unsigned index = offset; index < offset + num; ++index) {
            const uint8_t high_byte = (data[index] >> 8) & 0xFF;// Get the upper 8 bits
            const uint8_t low_byte = data[index] & 0xFF;
            ss << high_byte << low_byte;
        }
        return ss.str();
    }

    template <typename ArrayLike>
    void encode_uint32(uint32_t value, ArrayLike& buffer, size_t offset = 0) {
        static_assert(std::is_same<typename ArrayLike::value_type, uint16_t>::value,
                  "Buffer must hold uint16_t elements");
        buffer[0+offset] = (static_cast<uint16_t>((value >> 16) & 0xFFFF));
        buffer[1+offset] = (static_cast<uint16_t>(value & 0xFFFF));
    }

    inline std::array<uint16_t, 2> encode_uint32(uint32_t value) {
        std::array<uint16_t, 2> data{};
        encode_uint32(value, data);
        return data;
    }

    template <typename ArrayLike>
    void encode_uint64(uint64_t value, ArrayLike& buffer, size_t offset = 0) {
        static_assert(std::is_same<typename ArrayLike::value_type, uint16_t>::value,
                  "Buffer must hold uint16_t elements");
        buffer[0+offset] = (static_cast<uint16_t>((value >> 48) & 0xFFFF));
        buffer[1+offset] = (static_cast<uint16_t>(value >> 32 & 0xFFFF));
        buffer[2+offset] = (static_cast<uint16_t>((value >> 16) & 0xFFFF));
        buffer[3+offset] = (static_cast<uint16_t>(value & 0xFFFF));
    }

    inline std::array<uint16_t, 4> encode_uint64(uint64_t value) {
        std::array<uint16_t, 4> data{};
        encode_uint64(value, data);
        return data;
    }

    template <typename ArrayLike>
    void encode_float(float value, ArrayLike& buffer, size_t offset = 0) {
        static_assert(std::is_same<typename ArrayLike::value_type, uint16_t>::value,
                  "Buffer must hold uint16_t elements");
        uint32_t raw_value;
        std::memcpy(&raw_value, &value, sizeof(raw_value));
        encode_uint32(raw_value, buffer, offset);
    }

    inline std::array<uint16_t, 2> encode_float(float value) {
        uint32_t raw_value;
        std::memcpy(&raw_value, &value, sizeof(raw_value));
        return encode_uint32(raw_value);
    }

    template <typename ArrayLike>
    void encode_double(double value, ArrayLike& buffer, size_t offset = 0) {
        static_assert(std::is_same<typename ArrayLike::value_type, uint16_t>::value,
                  "Buffer must hold uint16_t elements");
        uint64_t raw_value;
        std::memcpy(&raw_value, &value, sizeof(raw_value));
        encode_uint64(raw_value, buffer, offset);
    }

    inline std::array<uint16_t, 4> encode_double(double value) {
        uint64_t raw_value;
        std::memcpy(&raw_value, &value, sizeof(raw_value));
        return encode_uint64(raw_value);
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
