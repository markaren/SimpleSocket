
#include "WebSocket.hpp"
#include "TCPSocket.hpp"
#include "SHA1.hpp"

#include <iostream>
#include <sstream>
#include <thread>

namespace {

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
