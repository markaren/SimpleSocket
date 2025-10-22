
#ifndef SIMPLE_SOCKET_MQTT_HPP
#define SIMPLE_SOCKET_MQTT_HPP

#include <string>
#include <vector>

namespace simple_socket {


    enum PacketType {
        CONNECT = 0x10,
        CONNACK = 0x20,
        PUBLISH = 0x30,
        SUBSCRIBE = 0x82,
        SUBACK = 0x90,
        UNSUBSCRIBE = 0xA2,
        UNSUBACK = 0xB0,
        PINGREQ = 0xC0,
        PINGRESP = 0xD0,
        DISCONNECT = 0xE0
    };

    inline std::vector<uint8_t> encodeShortString(const std::string& s) {
        std::vector<uint8_t> data;
        data.reserve(s.size() + 2);
        data.push_back(static_cast<uint8_t>(s.size() >> 8));
        data.push_back(static_cast<uint8_t>(s.size() & 0xFF));
        data.insert(data.end(), s.begin(), s.end());
        return data;
    }


    inline std::vector<uint8_t> encodeRemainingLength(size_t len) {
        std::vector<uint8_t> bytes;
        do {
            uint8_t byte = len % 128;
            len /= 128;
            if (len > 0) byte |= 128;
            bytes.push_back(byte);
        } while (len > 0);
        return bytes;
    }

    inline size_t decodeRemainingLength(SimpleConnection* conn) {
        size_t remLen = 0;
        size_t multiplier = 1;
        for (int i = 0; i < 4; ++i) {
            uint8_t enc = 0;
            if (!conn->readExact(&enc, 1)) return 0;
            remLen += static_cast<size_t>(enc & 0x7F) * multiplier;
            if ((enc & 0x80) == 0) break;
            multiplier *= 128;
        }
        return remLen;
    }

}// namespace simple_socket

#endif//MQTT_HPP
