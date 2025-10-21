
#include "simple_socket/WebSocket.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/socket_common.hpp"

#include "simple_socket/util/string_utils.hpp"
#include "simple_socket/util/uuid.hpp"

#include "simple_socket/ws/WebSocketConnection.hpp"
#include "simple_socket/ws/WebSocketHandshake.hpp"

#include <algorithm>
#include <atomic>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

using namespace simple_socket;

namespace {

    void handshake(SimpleConnection& conn) {
        // 1) Read full HTTP request headers
        std::string request;
        std::vector<unsigned char> buffer(1024);
        constexpr size_t kMaxHeaderBytes = 16 * 1024;
        for (;;) {
            if (request.find("\r\n\r\n") != std::string::npos) break;
            const int n = conn.read(buffer);
            if (n <= 0) throwSocketError("Failed to read handshake request from client.");
            request.append(reinterpret_cast<const char*>(buffer.data()), static_cast<size_t>(n));
            if (request.size() > kMaxHeaderBytes) throwSocketError("Handshake request headers too large.");
        }

        // 2) Split header block
        const auto hdrEnd = request.find("\r\n\r\n");
        const std::string headerBlock = request.substr(0, hdrEnd);

        // 3) Parse request line
        std::istringstream iss(headerBlock);
        std::string line;
        if (!std::getline(iss, line)) throwSocketError("Bad HTTP request.");
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::string method, path, version;
        {
            std::istringstream rl(line);
            rl >> method >> path >> version;
        }
        if (toLower(method) != "get" || toLower(version).rfind("http/1.1", 0) != 0) {
            throwSocketError("Handshake failed: expected GET HTTP/1.1.");
        }

        // 4) Parse headers (names to lowercase, values trimmed)
        std::vector<std::pair<std::string, std::string>> headers;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto pos = line.find(':');
            if (pos == std::string::npos) continue;
            std::string name = toLower(trim(line.substr(0, pos)));
            std::string value = trim(line.substr(pos + 1));
            headers.emplace_back(std::move(name), std::move(value));
        }
        auto getHeader = [&](const char* key) -> const std::string* {
            const std::string k = toLower(key);
            for (auto& kv : headers)
                if (kv.first == k) return &kv.second;
            return nullptr;
        };

        // 5) Validate required headers
        const std::string* hUpgrade = getHeader("Upgrade");
        if (!hUpgrade || toLower(*hUpgrade) != "websocket") {
            throwSocketError("Handshake failed: missing/invalid Upgrade: websocket.");
        }
        const std::string* hConn = getHeader("Connection");
        if (!hConn || toLower(*hConn).find("upgrade") == std::string::npos) {
            throwSocketError("Handshake failed: missing/invalid Connection: Upgrade.");
        }
        const std::string* hVersion = getHeader("Sec-WebSocket-Version");
        if (!hVersion || trim(*hVersion) != "13") {
            throwSocketError("Handshake failed: unsupported Sec-WebSocket-Version.");
        }
        const std::string* hKey = getHeader("Sec-WebSocket-Key");
        if (!hKey || trim(*hKey).empty()) {
            throwSocketError("Handshake failed: missing Sec-WebSocket-Key.");
        }

        // 6) Compute Sec-WebSocket-Accept
        const std::string clientKey = trim(*hKey);
        char secWebSocketAccept[29] = {};
        WebSocketHandshake::generate(clientKey.data(), secWebSocketAccept);// writes 28 bytes; buffer is NUL-terminated

        // 7) Send 101 Switching Protocols
        std::ostringstream response;
        response << "HTTP/1.1 101 Switching Protocols\r\n"
                 << "Upgrade: websocket\r\n"
                 << "Connection: Upgrade\r\n"
                 << "Sec-WebSocket-Accept: " << secWebSocketAccept << "\r\n\r\n";

        const std::string responseStr = response.str();
        if (!conn.write(responseStr)) {
            throwSocketError("Failed to send handshake response.");
        }
    }
}// namespace

struct WebSocket::Impl {

    explicit Impl(WebSocket* scope, uint16_t port, const std::string& cert_file = "", const std::string& key_file = "")
        : scope(scope), socket(port, 1, !cert_file.empty() && !key_file.empty(), cert_file, key_file) {}

    void run() {

        std::vector<std::unique_ptr<WebSocketConnectionImpl>> connections;

        while (!stop_) {

            try {
                WebSocketCallbacks callbacks{scope->onOpen, scope->onClose, scope->onMessage};
                auto ws = std::make_unique<WebSocketConnectionImpl>(callbacks, socket.accept(), WebSocketConnectionImpl::Role::Server);
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

WebSocket::WebSocket(uint16_t port, const std::string& cert_file, const std::string& key_file)
    : pimpl_(std::make_unique<Impl>(this, port, cert_file, key_file)) {}


void WebSocket::start() {
    pimpl_->start();
}

void WebSocket::stop() {
    pimpl_->stop();
}

WebSocket::~WebSocket() = default;
