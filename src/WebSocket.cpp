
#include "WebSocket.hpp"
#include "TCPSocket.hpp"
#include "SHA1.hpp"

#include <iostream>
#include <sstream>
#include <thread>

namespace {

    std::vector<uint8_t> createFrame(const std::string& message) {
        std::vector<uint8_t> frame;
        frame.push_back(0x81); // FIN, text frame

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

    void handleFrame(TCPConnection* clientSocket, const std::vector<uint8_t>& frame) {
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

        std::vector<uint8_t> payload(frame.begin() + pos, frame.begin() + pos + payloadLen);
        if (isMasked) {
            for (size_t i = 0; i < payload.size(); ++i) {
                payload[i] ^= mask[i % 4];
            }
        }

        switch (opcode) {
            case 0x1: // Text frame
            {
                std::string message(payload.begin(), payload.end());
                std::cout << "Received message: " << message << std::endl;

                auto responseFrame = createFrame(message);
                clientSocket->write(responseFrame);
            }
            break;
            case 0x8: // Close frame
                std::cout << "Received close frame" << std::endl;
                clientSocket->close();
                break;
            case 0x9: // Ping frame
                std::cout << "Received ping frame" << std::endl;
                {
                    std::vector<uint8_t> pongFrame = {0x8A}; // FIN, Pong frame
                    pongFrame.push_back(static_cast<uint8_t>(payload.size()));
                    pongFrame.insert(pongFrame.end(), payload.begin(), payload.end());
                    clientSocket->write(pongFrame);
                }
                break;
            case 0xA: // Pong frame
                std::cout << "Received pong frame" << std::endl;
                break;
            default:
                std::cerr << "Unsupported opcode: " << static_cast<int>(opcode) << std::endl;
                break;
        }
    }

    std::string generateAcceptKey(const std::string& clientKey) {
        SHA1 sha;
        const std::string MAGIC_KEY = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        sha.update(clientKey + MAGIC_KEY);

        return sha.final();
    }

    // Base64 encoding function
    std::string base64Encode(const unsigned char* data, size_t length) {
        static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string encoded;
        encoded.reserve((length + 2) / 3 * 4);

        for (size_t i = 0; i < length; i += 3) {
            unsigned char b1 = data[i];
            unsigned char b2 = i + 1 < length ? data[i + 1] : 0;
            unsigned char b3 = i + 2 < length ? data[i + 2] : 0;

            encoded.push_back(table[b1 >> 2]);
            encoded.push_back(table[((b1 & 0x3) << 4) | (b2 >> 4)]);
            encoded.push_back(i + 1 < length ? table[((b2 & 0xF) << 2) | (b3 >> 6)] : '=');
            encoded.push_back(i + 2 < length ? table[b3 & 0x3F] : '=');
        }

        return encoded;
    }

}

struct WebSocket::Impl {

public:
    explicit Impl(WebSocket* scope, uint16_t port)
        : scope(scope), socket(port) {

        thread = std::thread([this] {
            run();
        });
    }

    void run() {

        std::vector<WebSocketConnection> connections;

        while (!stop) {
            auto conn = socket.accept();
            if (conn) {
                std::vector<unsigned char> buffer(1024);
                auto bytesReceived = conn->read(buffer);
                if (bytesReceived == -1) {
                    continue;
                }

                std::string request(buffer.begin(), buffer.begin() + bytesReceived);
                std::string::size_type keyPos = request.find("Sec-WebSocket-Key: ");
                if (keyPos == std::string::npos) {
                    std::cerr << "Client handshake request is invalid." << std::endl;
                    continue;
                }
                keyPos += 19;
                std::string::size_type keyEnd = request.find("\r\n", keyPos);
                std::string clientKey = request.substr(keyPos, keyEnd - keyPos);

                std::string acceptKey = generateAcceptKey(clientKey);

                std::ostringstream response;
                response << "HTTP/1.1 101 Switching Protocols\r\n"
                         << "Upgrade: websocket\r\n"
                         << "Connection: Upgrade\r\n"
                         << "Sec-WebSocket-Accept: " << acceptKey << "\r\n\r\n";

                std::string responseStr = response.str();
                conn->write(responseStr);

                connections.emplace_back(std::move(conn));

                scope->onOpen(&connections.back());
            }
        }
    }

    ~Impl() {
        stop = true;
        if (thread.joinable()) {
            thread.join();
        }
    }

    bool stop = false;
    TCPServer socket;
    std::thread thread;
    WebSocket* scope;
};


WebSocket::WebSocket(uint16_t port)
    : pimpl_(std::make_unique<Impl>(this, port)) {}

void WebSocket::stop() {
    pimpl_->stop = true;
}

WebSocket::~WebSocket() = default;
