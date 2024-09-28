
#include "simple_socket/WebSocket.hpp"

#include "WebSocketHandshakeKeyGen.hpp"
#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/socket_common.hpp"

#include "simple_socket/util/uuid.hpp"
#include "simple_socket/ws/WebSocketHandshake.hpp"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

using namespace simple_socket;

namespace {

    std::vector<uint8_t> createFrame(const std::string& message) {
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
}// namespace

class WebSocketConnectionImpl: public WebSocketConnection {

public:
    WebSocketConnectionImpl(WebSocket* socket, std::unique_ptr<SimpleConnection> conn)
        : socket(socket), conn(std::move(conn)) {

        handshake();

        thread = std::thread([this] {
            listen();
        });
    }

    void handshake() const {
        std::vector<unsigned char> buffer(1024);
        const auto bytesReceived = conn->read(buffer);
        if (bytesReceived == -1) {
            throwSocketError("Failed to read handshake request from client.");
        }

        std::string request(buffer.begin(), buffer.begin() + bytesReceived);
        std::string::size_type keyPos = request.find("Sec-WebSocket-Key: ");
        if (keyPos == std::string::npos) {
            throwSocketError("Client handshake request is invalid.");
        }
        keyPos += 19;
        const auto keyEnd = request.find("\r\n", keyPos);
        const auto clientKey = request.substr(keyPos, keyEnd - keyPos);

        char secWebSocketAccept[29] = {};
        WebSocketHandshake::generate(clientKey.data(), secWebSocketAccept);

        std::ostringstream response;
        response << "HTTP/1.1 101 Switching Protocols\r\n"
                 << "Upgrade: websocket\r\n"
                 << "Connection: Upgrade\r\n"
                 << "Sec-WebSocket-Accept: " << secWebSocketAccept << "\r\n\r\n";

        const std::string responseStr = response.str();
        if (!conn->write(responseStr)) {
            throwSocketError("Failed to send handshake response");
        }
    }

    void send(const std::string& message) override {
        const auto frame = createFrame(message);
        conn->write(frame);
    }

    void listen() {
        std::vector<unsigned char> buffer(1024);
        while (!closed) {

            const auto recv = conn->read(buffer);
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
                    socket->onMessage(this, message);
                } break;
                case 0x8:// Close frame
                    close(false);

                    break;
                case 0x9:// Ping frame
                    std::cout << "Received ping frame" << std::endl;
                    {
                        std::vector<uint8_t> pongFrame = {0x8A};// FIN, Pong frame
                        pongFrame.push_back(static_cast<uint8_t>(payload.size()));
                        pongFrame.insert(pongFrame.end(), payload.begin(), payload.end());
                        conn->write(pongFrame);
                    }
                    break;
                case 0xA:// Pong frame
                    std::cout << "Received pong frame" << std::endl;
                    break;
                default:
                    std::cerr << "Unsupported opcode: " << static_cast<int>(opcode) << std::endl;
                    break;
            }
        }
    }

    void close(bool self) {
        if (!closed) {
            closed = true;

            socket->onClose(this);

            if (!self) {
                std::vector<uint8_t> closeFrame = {0x88};// FIN, Close frame
                closeFrame.push_back(0);
                conn->write(closeFrame);
            }
            conn->close();
        }
    }

    ~WebSocketConnectionImpl() override {
        close(true);
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::atomic_bool closed{false};
    WebSocket* socket;
    std::unique_ptr<SimpleConnection> conn;
    std::thread thread;
};

struct WebSocket::Impl {

    explicit Impl(WebSocket* scope, uint16_t port)
        : scope(scope), socket(port) {}

    void run() {

        std::vector<std::unique_ptr<WebSocketConnectionImpl>> connections;

        while (!stop_) {

            try {
                auto ws = std::make_unique<WebSocketConnectionImpl>(scope, socket.accept());
                scope->onOpen(ws.get());
                connections.emplace_back(std::move(ws));
            } catch (std::exception&) {
                // std::cerr << ex.what() << std::endl;
            }

            //cleanup connections
            for (auto it = connections.begin(); it != connections.end();) {

                if ((*it)->closed) {
                    it = connections.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void start() {
        thread = std::thread([this] {
            run();
        });
    }

    void stop() {
        stop_ = true;
        socket.close();
    }

    ~Impl() {
        stop();
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::atomic_bool stop_{false};

    WebSocket* scope;
    TCPServer socket;
    std::thread thread;
};


WebSocketConnection::WebSocketConnection()
    : uuid_(generateUUID()) {}

const std::string& WebSocketConnection::uuid() {
    return uuid_;
}

WebSocket::WebSocket(uint16_t port)
    : pimpl_(std::make_unique<Impl>(this, port)) {}


void WebSocket::start() {
    pimpl_->start();
}

void WebSocket::stop() {
    pimpl_->stop();
}

WebSocket::~WebSocket() = default;

struct WebsocketClient::Impl {

    Impl(std::string host, uint16_t port)
        : host_(std::move(host)), port_(port) {}

    void connect() {

        tcpSocket_ = ctx_.connect(host_, port_);
        performHandshake();

        listenerThread_ = std::thread([this] {
            listen();
        });
    }

    void send(const std::string& message) {
        if (!connected_) {
            std::cerr << "Not connected, unable to send message." << std::endl;
            return;
        }

        const auto frame = createFrame(message);
        if (!tcpSocket_->write(frame)) {
            std::cerr << "Failed to send message." << std::endl;
        }
    }

    void close() {
        stop_ = true;
        std::vector<uint8_t> closeFrame = {0x88};// FIN, Close frame
        closeFrame.push_back(0);
        tcpSocket_->write(closeFrame);
        tcpSocket_->close();

        if (listenerThread_.joinable()) {
            listenerThread_.join();
        }
    }

    ~Impl() {
        close();
    }

private:
    TCPClientContext ctx_;
    std::unique_ptr<SimpleConnection> tcpSocket_;
    std::atomic_bool connected_;
    std::atomic_bool stop_;
    std::thread listenerThread_;

    std::string host_;
    uint16_t port_;

    void performHandshake() {
        const std::string key;
        char output[28] = {};// 28 chars for base64-encoded output
        WebSocketHandshakeKeyGen::generate(key, output);

        std::ostringstream request;
        request << "GET "
                << "/ws"
                << " HTTP/1.1\r\n"
                << "Host: " << host_ << ":" << port_ << "\r\n"
                << "Upgrade: websocket\r\n"
                << "Connection: Upgrade\r\n"
                << "Sec-WebSocket-Key: " << key << "\r\n"
                << "Sec-WebSocket-Version: 13\r\n\r\n";

        const std::string requestStr = request.str();
        if (!tcpSocket_->write(requestStr)) {
            throwSocketError("Failed to send handshake request");
        }

        std::vector<uint8_t> buffer(1024);
        const auto bytesReceived = tcpSocket_->read(buffer);
        if (bytesReceived == -1) {
            throwSocketError("Failed to read handshake response from server.");
        }

        const std::string response(buffer.begin(), buffer.begin() + bytesReceived);
        if (response.find(" 101 ") == std::string::npos) {
            throwSocketError("Handshake failed with the server.");
        }

        connected_ = true;
    }

    void listen() {
        std::vector<uint8_t> buffer(1024);

        while (!stop_) {
            const auto recv = tcpSocket_->read(buffer);
            if (recv == -1) {
                break;
            }

            std::vector<uint8_t> frame{buffer.begin(), buffer.begin() + recv};

            if (frame.size() < 2) return;

            uint8_t opcode = frame[0] & 0x0F;
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
                    std::cout << "Received: " << message << std::endl;
                } break;
                case 0x8:// Close frame
                    close();
                    break;
                case 0x9:// Ping frame
                    std::cout << "Received ping frame" << std::endl;
                    sendPong(payload);
                    break;
                case 0xA:// Pong frame
                    std::cout << "Received pong frame" << std::endl;
                    break;
                default:
                    std::cerr << "Unsupported opcode: " << static_cast<int>(opcode) << std::endl;
                    break;
            }
        }
    }

    void sendPong(const std::vector<uint8_t>& payload) {
        std::vector<uint8_t> pongFrame = {0x8A};// FIN, Pong frame
        pongFrame.push_back(static_cast<uint8_t>(payload.size()));
        pongFrame.insert(pongFrame.end(), payload.begin(), payload.end());
        tcpSocket_->write(pongFrame);
    }
};

WebsocketClient::WebsocketClient(const std::string& host, uint16_t port)
    : pimpl_(std::make_unique<Impl>(host, port)) {}


void WebsocketClient::connect() {
    pimpl_->connect();
}

void WebsocketClient::close() {
    pimpl_->close();
}

WebsocketClient::~WebsocketClient() = default;
