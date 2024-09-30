
#include "simple_socket/WebSocket.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/socket_common.hpp"

#include "simple_socket/ws/WebSocketConnection.hpp"
#include "simple_socket/ws/WebSocketHandshakeKeyGen.hpp"

#include <sstream>

using namespace simple_socket;

namespace {
    void performHandshake(SimpleConnection& conn, const std::string& host, uint16_t port) {
        const std::string key;
        char output[28] = {};// 28 chars for base64-encoded output
        WebSocketHandshakeKeyGen::generate(key, output);

        std::ostringstream request;
        request << "GET "
                << "/ws"
                << " HTTP/1.1\r\n"
                << "Host: " << host << ":" << port << "\r\n"
                << "Upgrade: websocket\r\n"
                << "Connection: Upgrade\r\n"
                << "Sec-WebSocket-Key: " << key << "\r\n"
                << "Sec-WebSocket-Version: 13\r\n\r\n";

        const std::string requestStr = request.str();
        if (!conn.write(requestStr)) {
            throwSocketError("Failed to send handshake request");
        }

        std::vector<uint8_t> buffer(1024);
        const auto bytesReceived = conn.read(buffer);
        if (bytesReceived == -1) {
            throwSocketError("Failed to read handshake response from server.");
        }

        const std::string response(buffer.begin(), buffer.begin() + bytesReceived);
        if (response.find(" 101 ") == std::string::npos) {
            throwSocketError("Handshake failed with the server.");
        }
    }
}// namespace

struct WebSocketClient::Impl {

    std::unique_ptr<WebSocketConnectionImpl> conn;

    explicit Impl(WebSocketClient* scope)
        : scope_(scope) {}

    void connect(const std::string& host, uint16_t port) {

        auto c = ctx_.connect(host, port);

        conn = std::make_unique<WebSocketConnectionImpl>(WebSocketCallbacks{scope_->onOpen, scope_->onClose, scope_->onMessage}, std::move(c));
        conn->run([host, port](SimpleConnection& conn) {
            performHandshake(conn, host, port);
        });
    }

    void send(const std::string& message) {
        conn->send(message);
    }

    void close() {
        conn->close(true);
    }

    ~Impl() {
        close();
    }

private:
    TCPClientContext ctx_;
    WebSocketClient* scope_;
};

WebSocketClient::WebSocketClient()
    : pimpl_(std::make_unique<Impl>(this)) {}


void WebSocketClient::connect(const std::string& host, uint16_t port) {
    pimpl_->connect(host, port);
}

void WebSocketClient::send(const std::string& message) {
    pimpl_->send(message);
}

void WebSocketClient::close() {
    pimpl_->close();
}

WebSocketClient::~WebSocketClient() = default;
