
#ifndef SIMPLE_SOCKET_WEBSOCKET_CONNECTION_HPP
#define SIMPLE_SOCKET_WEBSOCKET_CONNECTION_HPP

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include "simple_socket/WebSocket.hpp"

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

        explicit WebSocketConnectionImpl(const WebSocketCallbacks& callbacks, std::unique_ptr<SimpleConnection> conn)
            : callbacks_(callbacks), conn_(std::move(conn)) {}

        void run(const std::function<void(SimpleConnection&)>& handshake) {

            handshake(*conn_);
            if (callbacks_.onOpen) callbacks_.onOpen(this);

            thread_ = std::thread([this] {
                listen();
            });
        }

        void send(const std::string& message) override {
            const auto frame = createFrame(message);
            conn_->write(frame);
        }

        void close(bool self) {
            if (!closed_) {
                closed_ = true;

                if (callbacks_.onClose) callbacks_.onClose(this);

                if (self) {
                    std::vector<uint8_t> closeFrame = {0x88};// FIN, Close frame
                    closeFrame.push_back(0);
                    conn_->write(closeFrame);
                }
                conn_->close();
            }
        }

        bool closed() const {
            return closed_;
        }

        ~WebSocketConnectionImpl() override {
            close(true);
            if (thread_.joinable()) {
                thread_.join();
            }
        }

    private:
        std::atomic_bool closed_{false};
        WebSocket* socket_{};
        std::unique_ptr<SimpleConnection> conn_;
        WebSocketCallbacks callbacks_;
        std::thread thread_;


        void listen() {
            std::vector<unsigned char> buffer(1024);
            while (!closed_) {

                const auto recv = conn_->read(buffer);
                if (recv == -1) {
                    break;
                }

                std::vector<uint8_t> frame{buffer.begin(), buffer.begin() + recv};

                if (frame.size() < 2) return;

                uint8_t opcode = frame[0] & 0x0F;
                // bool isFinal = (frame[0] & 0x80) != 0;
                bool isMasked = (frame[1] & 0x80) != 0;
                uint64_t payloadLen = frame[1] & 0x7F;

                size_t pos = 2;
                if (payloadLen == 126) {
                    payloadLen = (frame[2] << 8) | frame[3];
                    pos += 2;
                } else if (payloadLen == 127) {
                    payloadLen = 0;
                    for (int i = 0; i < 8; ++i) {
                        payloadLen = (payloadLen << 8) | frame[2 + i];
                    }
                    pos += 8;
                }

                std::vector<uint8_t> mask(4);
                if (isMasked) {
                    for (int i = 0; i < 4; ++i) {
                        mask[i] = frame[pos++];
                    }
                }

                std::vector payload(frame.begin() + pos, frame.begin() + pos + payloadLen);
                if (isMasked) {
                    for (size_t i = 0; i < payload.size(); ++i) {
                        payload[i] ^= mask[i % 4];
                    }
                }

                switch (opcode) {
                    case 0x1:// Text frame
                    {
                        std::string message(payload.begin(), payload.end());
                        if (callbacks_.onMessage) callbacks_.onMessage(this, message);
                    } break;
                    case 0x8:// Close frame
                        close(false);

                        break;
                    case 0x9:// Ping frame
                        {
                            std::vector<uint8_t> pongFrame = {0x8A};// FIN, Pong frame
                            pongFrame.push_back(static_cast<uint8_t>(payload.size()));
                            pongFrame.insert(pongFrame.end(), payload.begin(), payload.end());
                            conn_->write(pongFrame);
                        }
                        break;
                    case 0xA:// Pong frame
                        break;
                    default:
                        std::cerr << "Unsupported opcode: " << static_cast<int>(opcode) << std::endl;
                        break;
                }
            }
        }

        static std::vector<uint8_t> createFrame(const std::string& message) {
            std::vector<uint8_t> frame;
            frame.push_back(0x81);// FIN, text frame

            if (message.size() <= 125) {
                frame.push_back(static_cast<uint8_t>(message.size()));
            } else if (message.size() <= 65535) {
                frame.push_back(126);
                frame.push_back((message.size() >> 8) & 0xFF);
                frame.push_back(message.size() & 0xFF);
            } else {
                frame.push_back(127);
                for (int i = 7; i >= 0; --i) {
                    frame.push_back((message.size() >> (i * 8)) & 0xFF);
                }
            }

            frame.insert(frame.end(), message.begin(), message.end());
            return frame;
        }
    };
};// namespace simple_socket

#endif//SIMPLE_SOCKET_WEBSOCKET_CONNECTION_HPP
