

#include "simple_socket/WebSocket.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/socket_common.hpp"

#include "simple_socket/util/string_utils.hpp"

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

    struct HttpResponse {
        std::string statusLine;
        std::unordered_map<std::string, std::string> headers;// lower-cased names, trimmed values
    };

    HttpResponse parseHttpResponseHeaders(const std::string& raw) {
        HttpResponse r;
        const auto hdrEnd = raw.find("\r\n\r\n");
        const auto headerBlock = raw.substr(0, hdrEnd);
        std::istringstream iss(headerBlock);
        std::string line;

        if (!std::getline(iss, line)) throwSocketError("Bad HTTP response");
        if (!line.empty() && line.back() == '\r') line.pop_back();
        r.statusLine = line;

        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto pos = line.find(':');
            if (pos == std::string::npos) continue;
            auto name = toLower(trim(line.substr(0, pos)));
            auto value = trim(line.substr(pos + 1));
            r.headers[name] = value;
        }
        return r;
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
        char secKey[40] = {};
        WebSocketHandshakeKeyGen::generate(key, secKey);

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

        // Read until end of headers
        std::string response;
        std::vector<uint8_t> buf(512);
        constexpr size_t kMaxHeaderBytes = 16 * 1024;
        while (response.find("\r\n\r\n") == std::string::npos) {
            const int n = conn.read(buf);
            if (n <= 0) throwSocketError("Failed to read handshake response");
            response.append(reinterpret_cast<const char*>(buf.data()), static_cast<size_t>(n));
            if (response.size() > kMaxHeaderBytes) throwSocketError("Handshake headers too large");
        }

        // Parse headers
        const auto resp = parseHttpResponseHeaders(response);

        // Validate status line (HTTP/1.1 101 ...)
        if (resp.statusLine.find(" 101 ") == std::string::npos &&
            resp.statusLine.rfind(" 101", resp.statusLine.size() - 3) == std::string::npos) {
            throwSocketError("Handshake failed: expected HTTP 101");
        }

        // Validate Upgrade: websocket
        auto itUp = resp.headers.find("upgrade");
        if (itUp == resp.headers.end() || toLower(itUp->second) != "websocket") {
            throwSocketError("Handshake failed: invalid Upgrade header");
        }

        // Validate Connection includes "upgrade"
        auto itConn = resp.headers.find("connection");
        if (itConn == resp.headers.end() ||
            toLower(itConn->second).find("upgrade") == std::string::npos) {
            throwSocketError("Handshake failed: invalid Connection header");
        }

        // Validate Sec-WebSocket-Accept
        auto itAcc = resp.headers.find("sec-websocket-accept");
        if (itAcc == resp.headers.end()) throwSocketError("Handshake failed: missing Sec-WebSocket-Accept");

        char acceptBuf[29] = {};                              // 28 chars + NUL
        WebSocketHandshakeKeyGen::generate(secKey, acceptBuf);// computes the 28-char base64 accept
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
        conn = std::make_unique<WebSocketConnectionImpl>(callbacks, std::move(c));
        conn->setBufferSize(bufferSize);
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
