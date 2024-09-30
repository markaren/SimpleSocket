
#include "simple_socket/WebSocket.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/socket_common.hpp"

#include "simple_socket/util/uuid.hpp"

#include "simple_socket/ws/WebSocketConnection.hpp"
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

    void handshake(SimpleConnection& conn) {
        std::vector<unsigned char> buffer(1024);
        const auto bytesReceived = conn.read(buffer);
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
        if (!conn.write(responseStr)) {
            throwSocketError("Failed to send handshake response");
        }
    }
}// namespace

struct WebSocket::Impl {

    explicit Impl(WebSocket* scope, uint16_t port)
        : scope(scope), socket(port) {}

    void run() {

        std::vector<std::unique_ptr<WebSocketConnectionImpl>> connections;

        while (!stop_) {

            try {
                auto ws = std::make_unique<WebSocketConnectionImpl>(WebSocketCallbacks{scope->onOpen, scope->onClose, scope->onMessage}, socket.accept());
                ws->run(handshake);
                connections.emplace_back(std::move(ws));
            } catch (std::exception&) {
                // std::cerr << ex.what() << std::endl;
            }

            //cleanup connections
            for (auto it = connections.begin(); it != connections.end();) {

                if ((*it)->closed()) {
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
