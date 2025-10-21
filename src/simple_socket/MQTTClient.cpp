
#include "simple_socket/MQTTClient.hpp"

#include <array>
#include <iostream>
#include <utility>
#include <vector>

using namespace simple_socket;

namespace {


    enum PacketType {
        CONNECT = 0x10,
        CONNACK = 0x20,
        PUBLISH = 0x30,
        SUBSCRIBE = 0x82,
        SUBACK = 0x90,
        PINGREQ = 0xC0,
        PINGRESP = 0xD0,
        DISCONNECT = 0xE0
    };

    std::vector<uint8_t> encodeString(const std::string& s) {
        std::vector<uint8_t> data;
        data.push_back(static_cast<uint8_t>(s.size() >> 8));
        data.push_back(static_cast<uint8_t>(s.size() & 0xFF));
        data.insert(data.end(), s.begin(), s.end());
        return data;
    }

    std::vector<uint8_t> encodeRemainingLength(size_t len) {
        std::vector<uint8_t> bytes;
        do {
            uint8_t byte = len % 128;
            len /= 128;
            if (len > 0) byte |= 128;
            bytes.push_back(byte);
        } while (len > 0);
        return bytes;
    }

}// namespace

MQTTClient::MQTTClient(const std::string& host, int port, std::string clientId)
    : conn_(ctx_.connect(host, port)), clientId_(std::move(clientId)) {}

void MQTTClient::connect() {

    std::vector<uint8_t> payload = encodeString("MQTT");// protocol name
    payload.push_back(0x04);                            // version 3.1.1
    payload.push_back(0x02);                            // flags: clean session
    payload.push_back(0x00);                            // keepalive MSB
    payload.push_back(0x3C);                            // keepalive LSB = 60 sec

    auto cid = encodeString(clientId_);
    payload.insert(payload.end(), cid.begin(), cid.end());

    std::vector<uint8_t> packet = {CONNECT};
    auto len = encodeRemainingLength(payload.size());
    packet.insert(packet.end(), len.begin(), len.end());
    packet.insert(packet.end(), payload.begin(), payload.end());

    conn_->write(packet);
    std::array<uint8_t, 4> buffer{};
    conn_->readExact(buffer);
    if (buffer[0] == CONNACK && buffer[3] == 0x00) {
        std::cout << "✅ Connected to broker." << std::endl;
    } else {
        std::cerr << "❌ Connection failed." << std::endl;
    }
}

void MQTTClient::subscribe(const std::string& topic) {
    std::vector<uint8_t> payload;
    payload.push_back(0x00);// Packet ID MSB
    payload.push_back(0x01);// Packet ID LSB

    auto t = encodeString(topic);
    payload.insert(payload.end(), t.begin(), t.end());
    payload.push_back(0x00);// QoS 0

    std::vector<uint8_t> packet = {SUBSCRIBE};
    auto len = encodeRemainingLength(payload.size());
    packet.insert(packet.end(), len.begin(), len.end());
    packet.insert(packet.end(), payload.begin(), payload.end());

    conn_->write(packet);
    std::cout << "📡 Subscribed to topic: " << topic << std::endl;
}

void MQTTClient::publish(const std::string& topic, const std::string& message) {
    auto t = encodeString(topic);
    std::vector<uint8_t> payload;
    payload.insert(payload.end(), t.begin(), t.end());
    payload.insert(payload.end(), message.begin(), message.end());

    std::vector<uint8_t> packet = {PUBLISH};
    auto len = encodeRemainingLength(payload.size());
    packet.insert(packet.end(), len.begin(), len.end());
    packet.insert(packet.end(), payload.begin(), payload.end());

    conn_->write(packet);
    std::cout << "➡️  Published: " << message << " → " << topic << std::endl;
}

void MQTTClient::loop() {
   while (true) {
        // Fixed header: byte 1
        uint8_t header1 = 0;
        if (!conn_->readExact(&header1, 1)) break;

        // Remaining Length (MQTT varint, 1..4 bytes)
        size_t remLen = 0;
        size_t multiplier = 1;
        for (int i = 0; i < 4; ++i) {
            uint8_t enc = 0;
            if (!conn_->readExact(&enc, 1)) return;
            remLen += static_cast<size_t>(enc & 0x7F) * multiplier;
            if ((enc & 0x80) == 0) break;
            multiplier *= 128;
            if (i == 3) return; // malformed Remaining Length
        }

        // Read remaining bytes (variable header + payload)
        std::vector<uint8_t> payload(remLen);
        if (remLen > 0 && !conn_->readExact(payload.data(), payload.size())) break;

        const uint8_t packetType = header1 & 0xF0;
        const uint8_t flags = header1 & 0x0F;

        if (packetType == PUBLISH) {
            size_t pos = 0;

            // Topic length (2 bytes)
            if (payload.size() < 2) continue; // malformed
            const uint16_t topicLen = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
            pos += 2;

            // Bounds check before constructing the topic string
            if (pos + topicLen > payload.size()) continue; // avoid crash
            const char* topicPtr = reinterpret_cast<const char*>(payload.data() + pos);
            std::string topic(topicPtr, topicLen);
            pos += topicLen;

            // Skip Packet Identifier for QoS > 0
            const uint8_t qos = static_cast<uint8_t>((flags >> 1) & 0x03);
            if (qos > 0) {
                if (pos + 2 > payload.size()) continue; // malformed
                pos += 2;
            }

            // Remaining is the application message
            if (pos > payload.size()) continue;
            const char* msgPtr = reinterpret_cast<const char*>(payload.data() + pos);
            std::string msg(msgPtr, payload.size() - pos);

            std::cout << "📥 Message on [" << topic << "]: " << msg << std::endl;
        }
    }
}
