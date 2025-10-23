
#ifndef SIMPLE_SOCKET_WEBSOCKET_CONNECTION_HPP
#define SIMPLE_SOCKET_WEBSOCKET_CONNECTION_HPP

#include <atomic>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "simple_socket/ws/WebSocket.hpp"

namespace simple_socket {

    struct WebSocketCallbacks {
        std::function<void(WebSocketConnection*)>& onOpen;
        std::function<void(WebSocketConnection*)>& onClose;
        std::function<void(WebSocketConnection*, const std::string&)>& onMessage;

        WebSocketCallbacks(std::function<void(WebSocketConnection*)>& onOpen,
                           std::function<void(WebSocketConnection*)>& onClose,
                           std::function<void(WebSocketConnection*,
                                              const std::string&)>& onMessage)
            : onOpen(onOpen),
              onClose(onClose),
              onMessage(onMessage) {}
    };

    struct WebSocketConnectionImpl: WebSocketConnection {

        enum class Role { Client,
                          Server };

        explicit WebSocketConnectionImpl(const WebSocketCallbacks& callbacks,
                                         std::unique_ptr<SimpleConnection> conn,
                                         Role role)
            : role_(role),
              conn_(std::move(conn)),
              callbacks_(callbacks),
              buffer(1024) {}

        void setBufferSize(size_t size) {
            buffer.resize(size);
        }

        void run(const std::function<void(SimpleConnection&)>& handshake) {

            handshake(*conn_);
            if (callbacks_.onOpen) {
                callbacks_.onOpen(this);
            }

            thread_ = std::thread([this] {
                listen();
            });
        }

        void send(const std::string& message) override {
            const auto frame = buildText(message, role_);
            std::lock_guard lg(tx_mtx_);
            conn_->write(frame);
        }

        void close(bool self) {
            if (closed_.exchange(true)) return;

            if (self) {
                // Empty close payload; mask only if client role
                std::vector<uint8_t> closeFrame = buildClose(/*code=*/1000, role_);
                std::lock_guard lg(tx_mtx_);
                conn_->write(closeFrame);
            }

            conn_->close();

            if (thread_.joinable() && std::this_thread::get_id() != thread_.get_id()) {
                thread_.join();
            }

            if (callbacks_.onClose) {
                callbacks_.onClose(this);
            }
        }

        [[nodiscard]] bool closed() const {
            return closed_;
        }

        ~WebSocketConnectionImpl() override {
            close(true);
            if (thread_.joinable()) {
                thread_.join();
            }
        }

    private:
        Role role_;
        std::mutex tx_mtx_;// serialize writes only
        std::atomic_bool closed_{false};

        WebSocket* socket_{};
        std::unique_ptr<SimpleConnection> conn_;
        WebSocketCallbacks callbacks_;
        std::thread thread_;


        std::vector<unsigned char> buffer;


        static std::vector<uint8_t> buildFrame(uint8_t opcode,
                                               const uint8_t* data,
                                               size_t len,
                                               Role role) {
            std::vector<uint8_t> out;
            out.reserve(2 + 4 + len);
            out.push_back(0x80 | (opcode & 0x0F));// FIN=1
            const bool mask = (role == Role::Client);

            if (len <= 125) {
                out.push_back(static_cast<uint8_t>(len) | (mask ? 0x80 : 0x00));
            } else if (len <= 0xFFFF) {
                out.push_back(126 | (mask ? 0x80 : 0x00));
                out.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
                out.push_back(static_cast<uint8_t>(len & 0xFF));
            } else {
                out.push_back(127 | (mask ? 0x80 : 0x00));
                for (int i = 7; i >= 0; --i)
                    out.push_back(static_cast<uint8_t>((static_cast<uint64_t>(len) >> (i * 8)) & 0xFF));
            }

            uint8_t m[4] = {0, 0, 0, 0};
            if (mask) {
                std::random_device rd;
                for (auto& b : m) b = static_cast<uint8_t>(rd());
                out.insert(out.end(), m, m + 4);
            }

            const size_t start = out.size();
            out.resize(start + len);
            for (size_t i = 0; i < len; ++i) {
                uint8_t b = data ? data[i] : 0;
                if (mask) b ^= m[i & 0x03];
                out[start + i] = b;
            }
            return out;
        }

        static std::vector<uint8_t> buildText(const std::string& s, Role role) {
            return buildFrame(0x1, reinterpret_cast<const uint8_t*>(s.data()), s.size(), role);
        }

        static std::vector<uint8_t> buildClose(uint16_t code, Role role) {
            uint8_t p[2] = {static_cast<uint8_t>(code >> 8), static_cast<uint8_t>(code & 0xFF)};
            return buildFrame(0x8, p, 2, role);
        }

        static std::vector<uint8_t> buildPong(const std::vector<uint8_t>& payload, Role role) {
            return buildFrame(0xA, payload.data(), payload.size(), role);
        }

        void listen() {
            while (!closed_) {
                const auto recv = conn_->read(buffer);
                if (recv <= 0) break;

                std::vector<uint8_t> frame{buffer.begin(), buffer.begin() + recv};
                if (frame.size() < 2) continue;

                const uint8_t b0 = frame[0];
                const uint8_t b1 = frame[1];
                const uint8_t opcode = b0 & 0x0F;
                const bool isMasked = (b1 & 0x80) != 0;
                uint64_t payloadLen = (b1 & 0x7F);

                // Role‑based masking validation
                if (role_ == Role::Server && !isMasked) {
                    // Protocol error: client must mask; tear down
                    std::lock_guard lg(tx_mtx_);
                    conn_->write(buildClose(1002, role_));// 1002 protocol error
                    break;
                }
                if (role_ == Role::Client && isMasked) {
                    // Protocol error: server must not mask
                    std::lock_guard lg(tx_mtx_);
                    conn_->write(buildClose(1002, role_));
                    break;
                }

                size_t pos = 2;
                if (payloadLen == 126) {
                    if (frame.size() < 4) continue;
                    payloadLen = (static_cast<uint64_t>(frame[2]) << 8) | frame[3];
                    pos += 2;
                } else if (payloadLen == 127) {
                    if (frame.size() < 10) continue;
                    payloadLen = 0;
                    for (int i = 0; i < 8; ++i) payloadLen = (payloadLen << 8) | frame[2 + i];
                    pos += 8;
                }

                if (frame.size() < pos + (isMasked ? 4 : 0) + payloadLen) continue;

                uint8_t mask[4] = {0, 0, 0, 0};
                if (isMasked)
                    for (int i = 0; i < 4; ++i) mask[i] = frame[pos++];

                std::vector<uint8_t> payload(frame.begin() + pos, frame.begin() + pos + payloadLen);
                if (isMasked) {
                    for (size_t i = 0; i < payload.size(); ++i) {
                        payload[i] ^= mask[i & 0x03];
                    }
                }

                switch (opcode) {
                    case 0x1: {// Text
                        std::string message(payload.begin(), payload.end());
                        if (callbacks_.onMessage) callbacks_.onMessage(this, message);
                    } break;

                    case 0x8: {// Close
                        // Echo a close if we didn't initiate one
                        std::vector<uint8_t> closeResp = buildClose(1000, role_);
                        {
                            std::lock_guard lg(tx_mtx_);
                            conn_->write(closeResp);
                        }
                        close(false);
                    } break;

                    case 0x9: {// Ping
                        std::vector<uint8_t> pongFrame = buildPong(payload, role_);
                        std::lock_guard lg(tx_mtx_);
                        conn_->write(pongFrame);
                    } break;

                    case 0xA:// Pong
                        break;

                    default:
                        std::cerr << "Unsupported opcode: " << static_cast<int>(opcode) << std::endl;
                        break;
                }
            }
        }
    };
}// namespace simple_socket

#endif//SIMPLE_SOCKET_WEBSOCKET_CONNECTION_HPP
