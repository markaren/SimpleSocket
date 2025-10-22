
#include "simple_socket/MQTTClient.hpp"

#include "simple_socket/TCPSocket.hpp"

#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
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
        UNSUBSCRIBE = 0xA2,
        UNSUBACK = 0xB0,
        PINGREQ = 0xC0,
        PINGRESP = 0xD0,
        DISCONNECT = 0xE0
    };

    std::vector<uint8_t> encodeShortString(const std::string& s) {
        std::vector<uint8_t> data;
        data.reserve(s.size() + 2);
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

    void connackHandler(SimpleConnection* conn_) {

        // Robust CONNACK handling
        uint8_t header1 = 0;
        if (!conn_->readExact(&header1, 1)) {
            throw std::runtime_error("MQTT: failed to read CONNACK header");
        }
        if ((header1 & 0xF0) != CONNACK) {
            throw std::runtime_error("MQTT: expected CONNACK packet");
        }

        // Decode Remaining Length (1..4 bytes varint)
        size_t remLen = 0;
        size_t multiplier = 1;
        for (int i = 0; i < 4; ++i) {
            uint8_t enc = 0;
            if (!conn_->readExact(&enc, 1)) {
                throw std::runtime_error("MQTT: failed to read CONNACK length");
            }
            remLen += static_cast<size_t>(enc & 0x7F) * multiplier;
            if ((enc & 0x80) == 0) break;
            multiplier *= 128;
            if (i == 3) {
                throw std::runtime_error("MQTT: malformed Remaining Length");
            }
        }

        if (remLen < 2) {
            throw std::runtime_error("MQTT: CONNACK too short");
        }

        std::vector<uint8_t> vh(remLen);
        if (!conn_->readExact(vh.data(), vh.size())) {
            throw std::runtime_error("MQTT: failed to read CONNACK payload");
        }

        const auto sessionPresent = static_cast<uint8_t>(vh[0] & 0x01);
        const uint8_t rc = vh[1];// Return Code (v3.1.1) or Reason Code (v5)

        if (rc == 0x00) {
            // Success; `sessionPresent` is informational
            (void) sessionPresent;
            return;
        }

        auto reasonText = [](uint8_t code) -> std::string {
            switch (code) {
                case 0x01:
                    return "unacceptable protocol version";
                case 0x02:
                    return "identifier rejected";
                case 0x03:
                    return "server unavailable";
                case 0x04:
                    return "bad username or password";
                case 0x05:
                    return "not authorized";
                default:
                    return "connection refused";
            }
        };

        const std::string msg{"MQTT connection failed (code " +
                              std::to_string(rc) + "): " + reasonText(rc)};
        throw std::runtime_error(msg);
    }

}// namespace


struct MQTTClient::Impl {

    Impl(std::string host, int port, std::string clientId)
        : clientId_(std::move(clientId)), host_(std::move(host)), port_(port) {}

    void connect(bool tls) {

        conn_ = ctx_.connect(host_, port_, tls);

        std::vector<uint8_t> payload = encodeShortString("MQTT");// protocol name
        payload.push_back(0x04);                                 // version 3.1.1
        payload.push_back(0x02);                                 // flags: clean session
        payload.push_back(0x00);                                 // keepalive MSB
        payload.push_back(0x3C);                                 // keepalive LSB = 60 sec

        auto cid = encodeShortString(clientId_);
        payload.insert(payload.end(), cid.begin(), cid.end());

        std::vector<uint8_t> packet = {CONNECT};
        auto len = encodeRemainingLength(payload.size());
        packet.insert(packet.end(), len.begin(), len.end());
        packet.insert(packet.end(), payload.begin(), payload.end());

        conn_->write(packet);

        connackHandler(conn_.get());
    }

    void subscribe(const std::string& topic, const std::function<void(std::string)>& callback) {
        if (!conn_) {
            throw std::runtime_error("MQTTClient: not connected");
        }

        std::vector<uint8_t> payload;
        payload.push_back(0x00);// Packet ID MSB
        payload.push_back(0x01);// Packet ID LSB

        auto t = encodeShortString(topic);
        payload.insert(payload.end(), t.begin(), t.end());
        payload.push_back(0x00);// QoS 0

        std::vector<uint8_t> packet = {SUBSCRIBE};
        auto len = encodeRemainingLength(payload.size());
        packet.insert(packet.end(), len.begin(), len.end());
        packet.insert(packet.end(), payload.begin(), payload.end());

        conn_->write(packet);

        std::lock_guard lock(mutex_);
        callbacks_.emplace(topic, callback);
    }

    void unsubscribe(const std::string& topic) {
        if (!conn_) return;

        // Packet Identifier (non‑zero, wraps)
        static uint16_t pid = 1;
        if (++pid == 0) ++pid;

        // Payload: Packet Identifier + Topic Filter (MQTT short string)
        std::vector<uint8_t> payload;
        payload.push_back(static_cast<uint8_t>(pid >> 8));
        payload.push_back(static_cast<uint8_t>(pid & 0xFF));

        auto t = encodeShortString(topic);
        payload.insert(payload.end(), t.begin(), t.end());

        std::vector<uint8_t> packet = {UNSUBSCRIBE};
        const auto len = encodeRemainingLength(payload.size());
        packet.insert(packet.end(), len.begin(), len.end());
        packet.insert(packet.end(), payload.begin(), payload.end());

        conn_->write(packet);

        std::lock_guard lock(mutex_);
        callbacks_.erase(topic);
    }


    void publish(const std::string& topic, const std::string& message) {

        if (!conn_) {
            throw std::runtime_error("MQTTClient: not connected");
        }

        auto t = encodeShortString(topic);
        std::vector<uint8_t> payload;
        payload.insert(payload.end(), t.begin(), t.end());
        payload.insert(payload.end(), message.begin(), message.end());

        std::vector<uint8_t> packet = {PUBLISH};
        auto len = encodeRemainingLength(payload.size());
        packet.insert(packet.end(), len.begin(), len.end());
        packet.insert(packet.end(), payload.begin(), payload.end());

        conn_->write(packet);
    }

    void run() {

        if (!conn_) {
            throw std::runtime_error("MQTTClient: not connected");
        }

        thread_ = std::thread([this] {
            while (!stop_) {
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
                    if (i == 3) return;// malformed Remaining Length
                }

                // Read remaining bytes (variable header + payload)
                std::vector<uint8_t> payload(remLen);
                if (remLen > 0 && !conn_->readExact(payload.data(), payload.size())) break;

                const uint8_t packetType = header1 & 0xF0;
                const uint8_t flags = header1 & 0x0F;

                if (packetType == PINGRESP) {
                    // Must have RL=0; ignore payload if any and mark acked
                    continue;
                }

                if (packetType == PUBLISH) {
                    size_t pos = 0;

                    // Topic length (2 bytes)
                    if (payload.size() < 2) continue;// malformed
                    const uint16_t topicLen = (static_cast<uint16_t>(payload[0]) << 8) | payload[1];
                    pos += 2;

                    // Bounds check before constructing the topic string
                    if (pos + topicLen > payload.size()) continue;// avoid crash
                    const char* topicPtr = reinterpret_cast<const char*>(payload.data() + pos);
                    std::string topic(topicPtr, topicLen);
                    pos += topicLen;

                    // Skip Packet Identifier for QoS > 0
                    const auto qos = static_cast<uint8_t>((flags >> 1) & 0x03);
                    if (qos > 0) {
                        if (pos + 2 > payload.size()) continue;// malformed
                        pos += 2;
                    }

                    // Remaining is the application message
                    if (pos > payload.size()) continue;
                    const char* msgPtr = reinterpret_cast<const char*>(payload.data() + pos);
                    const std::string msg(msgPtr, payload.size() - pos);

                    std::lock_guard lock(mutex_);
                    if (callbacks_.contains(topic)) {
                        callbacks_.at(topic)(msg);
                    }
                }
            }
        });
    }

    void close() {

        stop_ = true;
        if (thread_.joinable()) {
            thread_.join();
        }

        if (conn_) {
            std::vector<uint8_t> packet = {DISCONNECT, 0x00};
            conn_->write(packet);
            conn_->close();
        }
    }

private:
    TCPClientContext ctx_;
    std::unique_ptr<SimpleConnection> conn_;

    std::thread thread_;
    std::mutex mutex_;

    std::atomic_bool stop_{false};
    std::unordered_map<std::string, std::function<void(std::string)>> callbacks_;

    std::string clientId_;
    std::string host_;
    int port_;
};

MQTTClient::MQTTClient(const std::string& host, int port, const std::string& clientId)
    : pimpl_(std::make_unique<Impl>(host, port, clientId)) {}

void MQTTClient::connect(bool tls) {

    pimpl_->connect(tls);
}

void MQTTClient::close() {
    pimpl_->close();
}

void MQTTClient::subscribe(const std::string& topic, const std::function<void(std::string)>& callback) {
    pimpl_->subscribe(topic, callback);
}

void MQTTClient::unsubscribe(const std::string& topic) {
    pimpl_->unsubscribe(topic);
}

void MQTTClient::publish(const std::string& topic, const std::string& message) {

    pimpl_->publish(topic, message);
}

void MQTTClient::run() {
    pimpl_->run();
}

MQTTClient::~MQTTClient() {
    close();
}
