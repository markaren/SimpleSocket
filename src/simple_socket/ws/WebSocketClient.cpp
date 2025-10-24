
#include "simple_socket/ws/WebSocket.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/socket_common.hpp"

#include "simple_socket/util/string_utils.hpp"

#include "simple_socket/ws/WebSocketConnection.hpp"
#include "simple_socket/ws/WebSocketHandshakeKeyGen.hpp"
#include "simple_socket/ws/WebSocketHandshakeCommon.hpp"

#include <array>
#include <sstream>

using namespace simple_socket;

namespace {

    // Minimal Base64 (sufficient for 16 random bytes -> 24-char key)
    std::string base64Encode(const uint8_t* data, size_t len) {
        static constexpr char tbl[] =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        out.reserve(((len + 2) / 3) * 4);
        size_t i = 0;
        while (i + 3 <= len) {
            uint32_t v = static_cast<uint32_t>(data[i]) << 16 | static_cast<uint32_t>(data[i + 1]) << 8 | static_cast<uint32_t>(data[i + 2]);
            out.push_back(tbl[(v >> 18) & 0x3F]);
            out.push_back(tbl[(v >> 12) & 0x3F]);
            out.push_back(tbl[(v >> 6) & 0x3F]);
            out.push_back(tbl[v & 0x3F]);
            i += 3;
        }
        if (i < len) {
            uint32_t v = static_cast<uint32_t>(data[i]) << 16;
            out.push_back(tbl[(v >> 18) & 0x3F]);
            out.push_back(tbl[(v >> 12) & 0x3F]);
            if (i + 1 < len) {
                v |= static_cast<uint32_t>(data[i + 1]) << 8;
                out.push_back(tbl[(v >> 6) & 0x3F]);
                out.push_back('=');
            } else {
                out.push_back('=');
                out.push_back('=');
            }
        }
        return out;
    }

    std::string generateSecWebSocketKey() {
        std::array<uint8_t, 16> rnd{};
        std::random_device rd;
        for (auto& b : rnd) b = static_cast<uint8_t>(rd());
        return base64Encode(rnd.data(), rnd.size());// 24 chars
    }

    bool isDefaultPort(bool tls, uint16_t port) {
        return (!tls && port == 80) || (tls && port == 443);
    }

    bool looksLikeIpv6(const std::string& h) {
        if (!h.empty() && h.front() == '[' && h.back() == ']') return true;
        return h.find(':') != std::string::npos;
    }

    std::string bracketIfIpv6(const std::string& host) {
        if (host.size() >= 2 && host.front() == '[' && host.back() == ']') return host;
        return looksLikeIpv6(host) ? ("[" + host + "]") : host;
    }

    std::pair<std::string, uint16_t> parseWebSocketURL(const std::string& url) {
        if (url.substr(0, 5) == "ws://") {
            const auto host_port = url.substr(5);
            const auto colonPos = host_port.find(':');
            if (colonPos == std::string::npos) {
                return {host_port, 80};
            }
            const auto host = host_port.substr(0, colonPos);
            const auto portStr = host_port.substr(colonPos + 1);
            const auto port = static_cast<uint16_t>(std::stoi(portStr));
            return {host, port};
        }
        if (url.substr(0, 6) == "wss://") {
            const auto host_port = url.substr(6);
            const auto colonPos = host_port.find(':');
            if (colonPos == std::string::npos) {
                return {host_port, 443};
            }
            const auto host = host_port.substr(0, colonPos);
            const auto portStr = host_port.substr(colonPos + 1);
            const auto port = static_cast<uint16_t>(std::stoi(portStr));
            return {host, port};
        }
        throw std::invalid_argument("Invalid WebSocket URL: " + url);
    }

    void performHandshake(SimpleConnection& conn, const std::string& url, const std::string& host, uint16_t port) {

        std::string path = "/";
        const auto schemePos = url.find("://");
        const auto pathPos = url.find('/', schemePos == std::string::npos ? 0 : schemePos + 3);
        if (pathPos != std::string::npos) path = url.substr(pathPos);

        const bool tls = url.rfind("wss://", 0) == 0;
        const std::string hostHeader = isDefaultPort(tls, port)
                                               ? bracketIfIpv6(host)
                                               : bracketIfIpv6(host) + ":" + std::to_string(port);

        // generate a fresh 24-char Sec-WebSocket-Key
        const std::string secKey = generateSecWebSocketKey();

        std::ostringstream request;
        request << "GET " << path << " HTTP/1.1\r\n"
                << "Host: " << host << ":" << port << "\r\n"
                << "Upgrade: websocket\r\n"
                << "Connection: Upgrade\r\n"
                << "Sec-WebSocket-Key: " << secKey << "\r\n"
                << "Sec-WebSocket-Version: 13\r\n\r\n";

        const std::string requestStr = request.str();
        if (!conn.write(requestStr)) {
            throwSocketError("Failed to send handshake request");
        }

        const auto raw = readHttpHeaderBlock(conn);
        const auto resp = parseHttpHeaders(raw);

        // validate response
        if (resp.startLine.find(" 101 ") == std::string::npos &&
            resp.startLine.rfind(" 101", resp.startLine.size() - 3) == std::string::npos) {
            throwSocketError("Handshake failed: expected HTTP 101");
        }
        auto itUp = resp.headers.find("upgrade");
        if (itUp == resp.headers.end() || toLower(itUp->second) != "websocket") {
            throwSocketError("Handshake failed: invalid Upgrade header");
        }
        auto itConn = resp.headers.find("connection");
        if (itConn == resp.headers.end() || toLower(itConn->second).find("upgrade") == std::string::npos) {
            throwSocketError("Handshake failed: invalid Connection header");
        }
        auto itAcc = resp.headers.find("sec-websocket-accept");
        if (itAcc == resp.headers.end()) throwSocketError("Handshake failed: missing Sec-WebSocket-Accept");

        // Compute expected accept from the exact client key we sent
        char acceptBuf[29] = {};
        WebSocketHandshakeKeyGen::generate(secKey, acceptBuf);// writes 28 chars
        const std::string expectedAccept(acceptBuf, acceptBuf + 28);
        if (trim(itAcc->second) != expectedAccept) {
            throwSocketError("Handshake failed: Sec-WebSocket-Accept mismatch");
        }
    }
}// namespace

struct WebSocketClient::Impl {

    std::unique_ptr<WebSocketConnectionImpl> conn;

    explicit Impl(WebSocketClient* scope)
        : scope_(scope) {}

    void connect(const std::string& url, int bufferSize = 1024) {

        const bool useTLS = url.rfind("wss://", 0) == 0;

        const auto [host, port] = parseWebSocketURL(url);
        auto c = ctx_.connect(host, port, useTLS);

        WebSocketCallbacks callbacks{scope_->onOpen, scope_->onClose, scope_->onMessage};
        conn = std::make_unique<WebSocketConnectionImpl>(callbacks, std::move(c), WebSocketConnectionImpl::Role::Client);
        conn->setBufferSize(bufferSize);
        conn->run([url, host, port](SimpleConnection& conn) {
            performHandshake(conn, url, host, port);
        });
    }

    void send(const std::string& message) {

        conn->send(message);
    }

    void send(const uint8_t* message, size_t len) {

        conn->send(message, len);
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
