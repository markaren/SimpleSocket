
#include "WebSocket.hpp"
#include "SocketIncludes.hpp"
#include "TCPSocket.hpp"
#include "WebSocketHandshake.hpp"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <sstream>
#include <thread>

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

struct WebSocketConnection::Impl {

    Impl(WebSocket* socket, WebSocketConnection* scope, std::unique_ptr<TCPConnection> conn)
        : socket(socket), scope(scope), conn(std::move(conn)) {

        handshake();

        thread = std::thread([this] {
            listen();
        });
    }

    void handshake() {
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
        std::string::size_type keyEnd = request.find("\r\n", keyPos);
        std::string clientKey = request.substr(keyPos, keyEnd - keyPos);

        char secWebSocketAccept[29] = {};
        WebSocketHandshake::generate(clientKey.data(), secWebSocketAccept);

        std::ostringstream response;
        response << "HTTP/1.1 101 Switching Protocols\r\n"
                 << "Upgrade: websocket\r\n"
                 << "Connection: Upgrade\r\n"
                 << "Sec-WebSocket-Accept: " << secWebSocketAccept << "\r\n\r\n";

        std::string responseStr = response.str();
        if (!conn->write(responseStr)) {
            throwSocketError("Failed to send handshake response");
        }
    }

    void send(const std::string& message) {
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
            bool isFinal = (frame[0] & 0x80) != 0;
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
                    socket->onMessage(scope, message);
                } break;
                case 0x8:// Close frame
                    if (!closed) {
                        scope->pimpl_->
                        socket->onClose(scope);
                        conn->close();
                        closed = true;
                    }
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

    void close() {
        if (!closed) {
            closed = true;

            socket->onClose(scope);

            std::vector<uint8_t> closeFrame = {0x88};// FIN, Close frame
            closeFrame.push_back(0);
            conn->write(closeFrame);
            conn->close();
        }
    }

    ~Impl() {
        close();
        if (thread.joinable()) {
            thread.join();
        }
    }

    bool closed = false;
    WebSocket* socket;
    WebSocketConnection* scope;
    std::unique_ptr<TCPConnection> conn;
    std::thread thread;
};

WebSocketConnection::WebSocketConnection(WebSocket* socket, std::unique_ptr<TCPConnection> conn)
    : pimpl_(std::make_unique<Impl>(socket, this, std::move(conn))) {}

void WebSocketConnection::send(const std::string& message) const {

    pimpl_->send(message);
}

WebSocketConnection::~WebSocketConnection() = default;


struct WebSocket::Impl {

    explicit Impl(WebSocket* scope, uint16_t port)
        : socket(port), scope(scope) {

        thread = std::thread([this] {
            run();
        });
    }

    void run() {

        while (!stop_) {

            try {
                auto ws = std::make_unique<WebSocketConnection>(scope, socket.accept());
                scope->onOpen(ws.get());
                connections.emplace_back(std::move(ws));
            } catch (std::exception& ex) {
                std::cerr << ex.what() << std::endl;
            }
        }

        std::cout << "WS closed" << std::endl;
    }

    void onClose(WebSocketConnection* conn) {
        scope->onClose(conn);
        const auto it = std::find_if(connections.begin(), connections.end(), [conn](auto& c) {
            return c.get() == conn;
        });
        if (it != connections.end()) {
            connections.erase(it);
        }
    }

    void stop() {
        stop_ = true;
        socket.close();
        if (thread.joinable()) {
            thread.join();
        }
    }

    ~Impl() {
        stop();
    }

    std::atomic_bool stop_ = false;
    TCPServer socket;
    std::thread thread;
    WebSocket* scope;

    std::vector<std::unique_ptr<WebSocketConnection>> connections;
};


WebSocket::WebSocket(uint16_t port)
    : pimpl_(std::make_unique<Impl>(this, port)) {}

void WebSocket::stop() const {
    pimpl_->stop();
}

WebSocket::~WebSocket() {
    stop();
}
