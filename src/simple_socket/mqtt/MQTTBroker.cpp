
#include "simple_socket/mqtt/MQTTBroker.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/mqtt/mqtt_common.hpp"

#ifdef SIMPLE_SOCKET_WITH_WEBSOCKETS
#include "simple_socket/ws/WebSocket.hpp"
#include "simple_socket/mqtt/WsMqttWrapper.hpp"
#endif

#include <iostream>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>


using namespace simple_socket;


struct MQTTBroker::Impl {

    explicit Impl(int port)
        : server_(port), stop_(false) {}

#ifdef SIMPLE_SOCKET_WITH_WEBSOCKETS
    explicit Impl(int port, int wsPort)
        : server_(port), ws_(wsPort), stop_(false) {}
#endif

    void start() {
        listener_ = std::thread([this] { acceptLoop(); });
#ifdef SIMPLE_SOCKET_WITH_WEBSOCKETS
        if (ws_) wsListener_ = std::thread([this] { wsAcceptLoop(); });
#endif
    }

    void stop() {
        stop_ = true;
        server_.close();
        if (listener_.joinable()) listener_.join();
#ifdef SIMPLE_SOCKET_WITH_WEBSOCKETS
        if (ws_) ws_->stop();
        if (wsListener_.joinable()) wsListener_.join();
#endif
    }

private:
    struct Client {
        std::unique_ptr<SimpleConnection> conn;
        std::string clientId;
        std::unordered_set<std::string> topics;
    };

#ifdef SIMPLE_SOCKET_WITH_WEBSOCKETS
    std::optional<WebSocket> ws_;
#endif

    TCPServer server_;

    std::atomic<bool> stop_;
    std::thread listener_;
    std::thread wsListener_;

    std::mutex subsMutex_;
    std::unordered_map<std::string, std::vector<Client*>> subscribers_;
    std::vector<Client*> clients_;

#ifdef SIMPLE_SOCKET_WITH_WEBSOCKETS
    void wsAcceptLoop() {

        std::unordered_map<WebSocketConnection*, WsWrapper*> connections;

        ws_->onOpen = [this, &connections](WebSocketConnection* conn) {
            auto client = std::make_unique<Client>();
            Client* clientPtr = client.get();
            clients_.push_back(clientPtr);
            auto wrapper = std::make_unique<WsWrapper>(conn);
            connections[conn] = wrapper.get();
            client->conn = std::move(wrapper);

            std::thread(&Impl::handleClient, this, std::move(client)).detach();
        };
        ws_->onMessage = [&connections](WebSocketConnection* conn, const std::string& msg) {
            connections[conn]->push_back(msg);
        };
        ws_->onClose = [&connections](WebSocketConnection* conn) {
            connections[conn]->close();
        };
        ws_->start();

        while (!stop_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // remove closed connections
            for (auto it = connections.begin(); it != connections.end();) {
                if (it->second->closed()) {
                    it = connections.erase(it);
                } else {
                    ++it;
                }
            }
        }
        ws_->stop();
    }
#endif

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
            if (!c->conn->write(connack)) return;

            // Main loop
            clientLoop(c.get());

            cleanupClient(*c);
        } catch (const std::exception& e) {
            std::cerr << "Client error: " << e.what() << std::endl;
            cleanupClient(*c);
        }
    }

    void handlePublish(Client* c, uint8_t hdr, std::vector<uint8_t>& buf) {
        size_t p = 0;

        // Topic length
        if (buf.size() < 2) return;
        uint16_t tlen = (static_cast<uint16_t>(buf[p]) << 8) | buf[p + 1];
        p += 2;
        if (p + tlen > buf.size()) return;
        std::string topic(reinterpret_cast<const char*>(&buf[p]), tlen);
        p += tlen;

        // QoS from fixed header
        const auto qos = static_cast<uint8_t>((hdr >> 1) & 0x03);
        if (qos > 0) {
            // Skip Packet Identifier for QoS1/2
            if (p + 2 > buf.size()) return;
            p += 2;
            // Optional: send PUBACK for QoS1, PUBREC/PUBREL/PUBCOMP for QoS2
            // (not implemented here)
        }

        if (p > buf.size()) return;
        std::string message(reinterpret_cast<const char*>(&buf[p]), buf.size() - p);

        {
            std::lock_guard lock(subsMutex_);
            if (subscribers_.empty() || !subscribers_.contains(topic)) return;
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

        std::lock_guard lock(subsMutex_);
        for (auto it = subscribers_.begin(); it != subscribers_.end();) {
            if (it->first == topic) {
                auto& subs = it->second;
                bool erased = false;
                for (auto* sub : subs) {
                    if (!sub->conn->write(packet)) {
                        it = subscribers_.erase(it);
                        erased = true;
                        break;// exit inner loop
                    }
                }
                if (!erased) ++it;
            } else {
                ++it;
            }
        }
    }

    void handleSubscriber(Client* c, uint8_t hdr, std::vector<uint8_t>& buf, bool& running) {


        size_t p = 0;
        if (buf.size() < 5) return;// pid(2) + topic len(2) + qos(1)
        uint16_t pid = (static_cast<uint16_t>(buf[p]) << 8) | buf[p + 1];
        p += 2;

        uint16_t tlen = (static_cast<uint16_t>(buf[p]) << 8) | buf[p + 1];
        p += 2;
        if (p + tlen + 1 > buf.size()) return;
        std::string topic(reinterpret_cast<const char*>(&buf[p]), tlen);
        p += tlen;
        /*uint8_t reqQos =*/(void) buf[p++];// ignore, grant QoS 0

        {
            std::lock_guard lock(subsMutex_);
            subscribers_[topic].push_back(c);
            c->topics.insert(topic);
        }

        // SUBACK echoing Packet Identifier, grant QoS 0
        const std::vector<uint8_t> suback = {
                SUBACK, 0x03,
                static_cast<uint8_t>(pid >> 8), static_cast<uint8_t>(pid & 0xFF),
                0x00};
        if (!c->conn->write(suback)) {
            running = false;
        }
    }

    void clientLoop(Client* c) {
        bool running = true;
        while (running && !stop_) {
            uint8_t hdr = 0;
            if (!c->conn->readExact(&hdr, 1)) break;
            size_t rem = decodeRemainingLength(c->conn.get());
            std::vector<uint8_t> buf(rem);
            if (rem > 0 && !c->conn->readExact(buf.data(), rem)) break;

            const auto typeNibble = static_cast<uint8_t>(hdr & 0xF0);
            const auto flagsNibble = static_cast<uint8_t>(hdr & 0x0F);

            switch (typeNibble) {
                case (PUBLISH & 0xF0): {
                    handlePublish(c, hdr, buf);
                } break;

                case (SUBSCRIBE & 0xF0): {
                    // Must have flags 0x02 per spec
                    if (flagsNibble != 0x02) break;
                    handleSubscriber(c, hdr, buf, running);
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
                        std::erase(vec, c);
                    }

                    // UNSUBACK echoing Packet Identifier
                    const std::vector<uint8_t> unsuback = {
                            UNSUBACK, 0x02,
                            static_cast<uint8_t>(pid >> 8), static_cast<uint8_t>(pid & 0xFF)};
                    if (!c->conn->write(unsuback)) {
                        running = false;
                    }
                } break;

                case PINGREQ: {
                    if (flagsNibble != 0x00) break;
                    const std::vector<uint8_t> pingresp = {PINGRESP, 0x00};
                    if (!c->conn->write(pingresp)) {
                        running = false;
                    }
                } break;

                case DISCONNECT:
                    running = false;
                    break;

                default:
                    // ignore unsupported packets
                    break;
            }
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


MQTTBroker::MQTTBroker(int tcpPort)
    : pimpl_(std::make_unique<Impl>(tcpPort)) {}

#ifdef SIMPLE_SOCKET_WITH_WEBSOCKETS
MQTTBroker::MQTTBroker(int tcpPort, int wsPort)
    : pimpl_(std::make_unique<Impl>(tcpPort, wsPort)) {}
#endif

void MQTTBroker::start() {
    pimpl_->start();
}

void MQTTBroker::stop() {
    pimpl_->stop();
}

MQTTBroker::~MQTTBroker() {
    stop();
}
