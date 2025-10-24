
#include "simple_socket/ws/WebSocket.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/socket_common.hpp"

#include "simple_socket/util/string_utils.hpp"
#include "simple_socket/util/uuid.hpp"

#include "simple_socket/ws/WebSocketConnection.hpp"
#include "simple_socket/ws/WebSocketHandshake.hpp"
#include "simple_socket/ws/WebSocketHandshakeCommon.hpp"

#include <algorithm>
#include <atomic>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

using namespace simple_socket;

namespace {

    void handshake(SimpleConnection& conn) {

        const auto raw = readHttpHeaderBlock(conn);
        const auto http = parseHttpHeaders(raw);

        auto getHeader = [&http](const std::string& name) -> const std::string* {
            const auto it = http.headers.find(toLower(name));
            if (it != http.headers.end()) {
                return &it->second;
            }
            return nullptr;
        };

        // Validate required headers
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

        // Compute Sec-WebSocket-Accept
        const std::string clientKey = trim(*hKey);
        char secWebSocketAccept[29] = {};
        WebSocketHandshake::generate(clientKey.data(), secWebSocketAccept);// writes 28 bytes; buffer is NUL-terminated

        //Send 101 Switching Protocols
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

    explicit Impl(WebSocket* scope, uint16_t port)
        : scope(scope), socket(port) {}

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

WebSocket::WebSocket(uint16_t port)
    : pimpl_(std::make_unique<Impl>(this, port)) {}


void WebSocket::start() {
    pimpl_->start();
}

void WebSocket::stop() {
    pimpl_->stop();
}

WebSocket::~WebSocket() = default;
