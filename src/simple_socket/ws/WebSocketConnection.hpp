
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

    enum : uint8_t {
        WS_CONT = 0x0,
        WS_TEXT = 0x1,
        WS_BIN = 0x2,
        WS_CLOSE = 0x8,
        WS_PING = 0x9,
        WS_PONG = 0xA
    };

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

        void send(const uint8_t* message, size_t len) override {
            const auto frame = buildBin(message, len, role_);
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
            return buildFrame(WS_TEXT, reinterpret_cast<const uint8_t*>(s.data()), s.size(), role);
        }

        static std::vector<uint8_t> buildBin(const uint8_t* msg, size_t len, Role role) {
            return buildFrame(WS_BIN, msg, len, role);
        }

        static std::vector<uint8_t> buildClose(uint16_t code, Role role) {
            uint8_t p[2] = {static_cast<uint8_t>(code >> 8), static_cast<uint8_t>(code & 0xFF)};
            return buildFrame(WS_CLOSE, p, 2, role);
        }

        static std::vector<uint8_t> buildPong(const std::vector<uint8_t>& payload, Role role) {
            return buildFrame(WS_PONG, payload.data(), payload.size(), role);
        }

        void listen() {
            std::vector<uint8_t> rx;     // accumulated bytes from socket
            std::vector<uint8_t> message;// assembling fragmented messages
            bool continued = false;
            uint8_t startOpcode = 0;

            while (!closed_) {
                const int recv = conn_->read(buffer);
                if (recv <= 0) break;

                rx.insert(rx.end(), buffer.begin(), buffer.begin() + recv);

                size_t pos = 0;
                while (true) {
                    if (rx.size() - pos < 2) break;

                    const uint8_t b0 = rx[pos];
                    const uint8_t b1 = rx[pos + 1];
                    const bool fin = (b0 & 0x80) != 0;
                    const uint8_t opcode = static_cast<uint8_t>(b0 & 0x0F);
                    const bool isMasked = (b1 & 0x80) != 0;
                    uint64_t payloadLen = (b1 & 0x7F);

                    // Masking rules
                    if (role_ == Role::Server && !isMasked) {
                        std::lock_guard lg(tx_mtx_);
                        conn_->write(buildClose(1002, role_));
                        close(false);
                        return;
                    }
                    if (role_ == Role::Client && isMasked) {
                        std::lock_guard lg(tx_mtx_);
                        conn_->write(buildClose(1002, role_));
                        close(false);
                        return;
                    }

                    size_t hdr = 2;
                    if (payloadLen == 126) {
                        if (rx.size() - pos < hdr + 2) break;
                        payloadLen = (static_cast<uint64_t>(rx[pos + 2]) << 8) | rx[pos + 3];
                        hdr += 2;
                    } else if (payloadLen == 127) {
                        if (rx.size() - pos < hdr + 8) break;
                        payloadLen = 0;
                        for (int i = 0; i < 8; ++i) payloadLen = (payloadLen << 8) | rx[pos + 2 + i];
                        hdr += 8;
                    }

                    const size_t need = hdr + (isMasked ? 4 : 0) + payloadLen;
                    if (rx.size() - pos < need) break;

                    size_t off = pos + hdr;
                    uint8_t mask[4] = {0, 0, 0, 0};
                    if (isMasked) {
                        for (int i = 0; i < 4; ++i) mask[i] = rx[off + i];
                        off += 4;
                    }

                    std::vector<uint8_t> chunk;
                    chunk.resize(payloadLen);
                    for (size_t i = 0; i < chunk.size(); ++i) {
                        uint8_t b = rx[off + i];
                        if (isMasked) b ^= mask[i & 0x03];
                        chunk[i] = b;
                    }

                    pos = off + payloadLen;

                    if (opcode == WS_CLOSE) {
                        {
                            std::lock_guard lg(tx_mtx_);
                            conn_->write(buildClose(1000, role_));
                        }
                        close(false);
                        return;
                    } else if (opcode == WS_PING) {
                        std::lock_guard lg(tx_mtx_);
                        conn_->write(buildPong(chunk, role_));
                        continue;
                    } else if (opcode == WS_PONG) {
                        continue;
                    } else if (opcode == WS_CONT) {
                        if (!continued) {
                            std::lock_guard lg(tx_mtx_);
                            conn_->write(buildClose(1002, role_));
                            close(false);
                            return;
                        }
                        message.insert(message.end(), chunk.begin(), chunk.end());
                        if (fin) {
                            // deliver completed fragmented message
                            if (startOpcode == WS_TEXT) {
                                std::string s(message.begin(), message.end());
                                if (callbacks_.onMessage) callbacks_.onMessage(this, s);
                            } else if (startOpcode == WS_BIN) {
                                std::string s(reinterpret_cast<const char*>(message.data()), message.size());
                                if (callbacks_.onMessage) callbacks_.onMessage(this, s);
                            }
                            message.clear();
                            continued = false;
                            startOpcode = 0;
                        }
                        continue;
                    } else if (opcode == WS_TEXT || opcode == WS_BIN) {
                        if (continued) {
                            std::lock_guard lg(tx_mtx_);
                            conn_->write(buildClose(1002, role_));
                            close(false);
                            return;
                        }
                        if (fin) {
                            // single-frame message
                            if (opcode == WS_TEXT) {
                                std::string s(chunk.begin(), chunk.end());
                                if (callbacks_.onMessage) callbacks_.onMessage(this, s);
                            } else {
                                std::string s(reinterpret_cast<const char*>(chunk.data()), chunk.size());
                                if (callbacks_.onMessage) callbacks_.onMessage(this, s);
                            }
                        } else {
                            // start fragmented message
                            message = std::move(chunk);
                            continued = true;
                            startOpcode = opcode;
                        }
                        continue;
                    } else {
                        // Unknown opcode: ignore this frame
                        continue;
                    }
                }

                // discard parsed bytes, keep remainder for next read
                if (pos > 0) rx.erase(rx.begin(), rx.begin() + pos);
            }
        }
    };
}// namespace simple_socket

#endif//SIMPLE_SOCKET_WEBSOCKET_CONNECTION_HPP
