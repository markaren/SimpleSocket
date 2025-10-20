

#include "simple_socket/WebSocket.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/socket_common.hpp"

#include "simple_socket/ws/WebSocketConnection.hpp"
#include "simple_socket/ws/WebSocketHandshakeKeyGen.hpp"

#include <sstream>

using namespace simple_socket;

namespace {

    std::pair<std::string, uint16_t> parseWebSocketURL(const std::string& url) {
        if (url.substr(0, 5) == "ws://") {
            const auto host_port = url.substr(5);
            const auto colonPos = host_port.find(':');
            if (colonPos == std::string::npos) {
                return {host_port, 80};
            } else {
                const auto host = host_port.substr(0, colonPos);
                const auto portStr = host_port.substr(colonPos + 1);
                const auto port = static_cast<uint16_t>(std::stoi(portStr));
                return {host, port};
            }
        } else if (url.substr(0, 6) == "wss://") {
            const auto host_port = url.substr(6);
            const auto colonPos = host_port.find(':');
            if (colonPos == std::string::npos) {
                return {host_port, 443};
            } else {
                const auto host = host_port.substr(0, colonPos);
                const auto portStr = host_port.substr(colonPos + 1);
                const auto port = static_cast<uint16_t>(std::stoi(portStr));
                return {host, port};
            }
        } else {
            throw std::invalid_argument("Invalid WebSocket URL: " + url);
        }
    }

    void performHandshake(SimpleConnection& conn, const std::string& url, const std::string& host, uint16_t port) {

        // Extract path from URL
        std::string path = "/";
        auto pathPos = url.find('/', url.find("://") + 3);
        if (pathPos != std::string::npos) {
            path = url.substr(pathPos);
        }

        // Generate handshake key
        char key[25] = {};
        char base64Key[40] = {};
        WebSocketHandshakeKeyGen::generate(key, base64Key);

        std::ostringstream request;
        request << "GET " << path << " HTTP/1.1\r\n"
                << "Host: " << host << ":" << port << "\r\n"
                << "Upgrade: websocket\r\n"
                << "Connection: Upgrade\r\n"
                << "Sec-WebSocket-Key: " << base64Key << "\r\n"
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

    void connect(const std::string& url) {

        bool useTLS = url.find("wss://") != std::string::npos;

        const auto [host, port] = parseWebSocketURL(url);
        auto c = ctx_.connect(host, port, useTLS);

        conn = std::make_unique<WebSocketConnectionImpl>(WebSocketCallbacks {scope_->onOpen, scope_->onClose, scope_->onMessage}, std::move(c));
        conn->run([url, host, port](SimpleConnection& conn) {
            performHandshake(conn, url, host, port);
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


void WebSocketClient::connect(const std::string& url) {
    pimpl_->connect(url);
}

void WebSocketClient::send(const std::string& message) {
    pimpl_->send(message);
}

void WebSocketClient::close() {
    pimpl_->close();
}

WebSocketClient::~WebSocketClient() = default;
