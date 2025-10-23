
#include "simple_socket/mqtt/MQTTBroker.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/mqtt/mqtt_common.hpp"

#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>


using namespace simple_socket;


struct MQTTBroker::Impl {

    explicit Impl(int port)
        : server_(port), stop_(false) {}

    void start() {
        listener_ = std::thread([this] { acceptLoop(); });
    }

    void stop() {
        stop_ = true;
        server_.close();
        if (listener_.joinable()) listener_.join();
    }

private:
    struct Client {
        std::unique_ptr<SimpleConnection> conn;
        std::string clientId;
        std::unordered_set<std::string> topics;
    };

    TCPServer server_;
    std::atomic<bool> stop_;
    std::thread listener_;

    std::mutex subsMutex_;
    std::unordered_map<std::string, std::vector<Client*>> subscribers_;
    std::vector<Client*> clients_;


    void acceptLoop() {

        while (!stop_) {
            try {
                auto conn = server_.accept();

                auto client = std::make_unique<Client>();
                client->conn = std::move(conn);
                Client* clientPtr = client.get();
                clients_.push_back(clientPtr);

                std::thread(&Impl::handleClient, this, std::move(client)).detach();
            } catch (std::exception&) {
                break;
            }
        }
    }

    void handleClient(std::unique_ptr<Client> c) {
        try {
            // CONNECT
            uint8_t header = 0;
            if (!c->conn->readExact(&header, 1)) return;
            if (header != CONNECT) return;// CONNECT must be 0x10

            size_t remLen = decodeRemainingLength(c->conn.get());
            std::vector<uint8_t> payload(remLen);
            if (remLen > 0 && !c->conn->readExact(payload.data(), payload.size())) return;

            // Parse CONNECT variable header + payload safely
            size_t pos = 0;
            if (payload.size() < 2) return;
            uint16_t protoLen = (static_cast<uint16_t>(payload[pos]) << 8) | payload[pos + 1];
            pos += 2;
            if (pos + protoLen + 4 > payload.size()) return;// name + level + flags + keepalive
            pos += protoLen;                                // skip protocol name
            pos += 4;                                       // skip level(1), flags(1), keepalive(2)

            if (pos + 2 > payload.size()) return;
            uint16_t cidLen = (static_cast<uint16_t>(payload[pos]) << 8) | payload[pos + 1];
            pos += 2;
            if (pos + cidLen > payload.size()) return;
            c->clientId.assign(reinterpret_cast<char*>(payload.data() + pos), cidLen);

            // CONNACK
            const std::vector<uint8_t> connack = {CONNACK, 0x02, 0x00, 0x00};
            c->conn->write(connack);

            // Main loop
            bool running = true;
            while (running && !stop_) {
                uint8_t hdr = 0;
                if (!c->conn->readExact(&hdr, 1)) break;
                size_t rem = decodeRemainingLength(c->conn.get());
                std::vector<uint8_t> buf(rem);
                if (rem > 0 && !c->conn->readExact(buf.data(), rem)) break;

                const uint8_t typeNibble = static_cast<uint8_t>(hdr & 0xF0);
                const uint8_t flagsNibble = static_cast<uint8_t>(hdr & 0x0F);

                switch (typeNibble) {
                    case (PUBLISH & 0xF0): {
                        size_t p = 0;

                        // Topic length
                        if (buf.size() < 2) return;
                        uint16_t tlen = (static_cast<uint16_t>(buf[p]) << 8) | buf[p + 1];
                        p += 2;
                        if (p + tlen > buf.size()) return;
                        std::string topic(reinterpret_cast<const char*>(&buf[p]), tlen);
                        p += tlen;

                        // QoS from fixed header
                        const uint8_t qos = static_cast<uint8_t>((hdr >> 1) & 0x03);
                        if (qos > 0) {
                            // Skip Packet Identifier for QoS1/2
                            if (p + 2 > buf.size()) return;
                            p += 2;
                            // Optional: send PUBACK for QoS1, PUBREC/PUBREL/PUBCOMP for QoS2
                            // (not implemented here)
                        }

                        if (p > buf.size()) return;
                        std::string message(reinterpret_cast<const char*>(&buf[p]), buf.size() - p);

                        // Snapshot subscribers to avoid holding the lock during writes
                        std::vector<Client*> targets;
                        {
                            std::lock_guard lock(subsMutex_);
                            auto it = subscribers_.find(topic);
                            if (it == subscribers_.end()) return;
                            targets = it->second;
                        }

                        // Build QoS 0 PUBLISH to subscribers
                        auto packetTopic = encodeShortString(topic);
                        std::vector<uint8_t> pl;
                        pl.reserve(packetTopic.size() + message.size());
                        pl.insert(pl.end(), packetTopic.begin(), packetTopic.end());
                        pl.insert(pl.end(), message.begin(), message.end());

                        std::vector<uint8_t> packet;
                        packet.reserve(1 + 4 + pl.size());
                        packet.push_back(PUBLISH);// 0x30 (QoS0)
                        auto len = encodeRemainingLength(pl.size());
                        packet.insert(packet.end(), len.begin(), len.end());
                        packet.insert(packet.end(), pl.begin(), pl.end());

                        for (auto* sub : targets) {
                            // std::cout << "MQTTBroker: delivering message on topic '" << topic << "' to client '" << sub->clientId << "'" << std::endl;
                            sub->conn->write(packet);
                        }
                    } break;

                    case (SUBSCRIBE & 0xF0): {
                        // Must have flags 0x02 per spec
                        if (flagsNibble != 0x02) break;
                        size_t p = 0;
                        if (buf.size() < 5) break;// pid(2) + topic len(2) + qos(1)
                        uint16_t pid = (static_cast<uint16_t>(buf[p]) << 8) | buf[p + 1];
                        p += 2;

                        uint16_t tlen = (static_cast<uint16_t>(buf[p]) << 8) | buf[p + 1];
                        p += 2;
                        if (p + tlen + 1 > buf.size()) break;
                        std::string topic(reinterpret_cast<const char*>(&buf[p]), tlen);
                        p += tlen;
                        /*uint8_t reqQos =*/(void) buf[p++];// ignore, grant QoS 0

                        {
                            std::lock_guard lock(subsMutex_);
                            subscribers_[topic].push_back(c.get());
                            c->topics.insert(topic);
                        }

                        // SUBACK echoing Packet Identifier, grant QoS 0
                        const std::vector<uint8_t> suback = {
                                SUBACK, 0x03,
                                static_cast<uint8_t>(pid >> 8), static_cast<uint8_t>(pid & 0xFF),
                                0x00};
                        c->conn->write(suback);
                    } break;

                    case (UNSUBSCRIBE & 0xF0): {
                        if (flagsNibble != 0x02) break;
                        size_t p = 0;
                        if (buf.size() < 4) break;// pid(2) + topic len(2)
                        uint16_t pid = (static_cast<uint16_t>(buf[p]) << 8) | buf[p + 1];
                        p += 2;

                        uint16_t tlen = (static_cast<uint16_t>(buf[p]) << 8) | buf[p + 1];
                        p += 2;
                        if (p + tlen > buf.size()) break;
                        std::string topic(reinterpret_cast<const char*>(&buf[p]), tlen);

                        {
                            std::lock_guard lock(subsMutex_);
                            c->topics.erase(topic);
                            auto& vec = subscribers_[topic];
                            std::erase(vec, c.get());
                        }

                        // UNSUBACK echoing Packet Identifier
                        const std::vector<uint8_t> unsuback = {
                                UNSUBACK, 0x02,
                                static_cast<uint8_t>(pid >> 8), static_cast<uint8_t>(pid & 0xFF)};
                        c->conn->write(unsuback);
                    } break;

                    case PINGREQ: {
                        if (flagsNibble != 0x00) break;
                        const std::vector<uint8_t> pingresp = {PINGRESP, 0x00};
                        c->conn->write(pingresp);
                    } break;

                    case DISCONNECT:
                        running = false;
                        break;

                    default:
                        // ignore unsupported packets
                        break;
                }
            }

            cleanupClient(*c);
        } catch (const std::exception& e) {
            std::cerr << "Client error: " << e.what() << std::endl;
            cleanupClient(*c);
        }
    }

    void cleanupClient(Client& c) {
        std::lock_guard lock(subsMutex_);
        for (auto& topic : c.topics) {
            auto& vec = subscribers_[topic];
            std::erase(vec, &c);
        }
        c.conn->close();
    }
};


MQTTBroker::MQTTBroker(int port)
    : pimpl_(std::make_unique<Impl>(port)) {}

void MQTTBroker::start() {
    pimpl_->start();
}

void MQTTBroker::stop() {
    pimpl_->stop();
}

MQTTBroker::~MQTTBroker() {
    stop();
}
