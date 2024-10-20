
#include "simple_socket/TCPSocket.hpp"

#include "simple_socket/SimpleConnection.hpp"
#include "simple_socket/Socket.hpp"

using namespace simple_socket;

namespace {
    SOCKET createSocket() {
        SOCKET sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd == INVALID_SOCKET) {
            throwSocketError("Failed to create socket");
        }

        // const int optval = 1;
        // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval));

        return sockfd;
    }

    std::pair<std::string, uint16_t> parseHostPort(const std::string& input) {
        const size_t colonPos = input.find(':');
        if (colonPos == std::string::npos) {
            throw std::invalid_argument("Invalid input format. Expected 'host:port'.");
        }

        std::string host = input.substr(0, colonPos);
        std::string portStr = input.substr(colonPos + 1);

        // Convert port string to uint16_t
        uint16_t port;
        try {
            port = static_cast<uint16_t>(std::stoi(portStr));
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid port number.");
        }

        return std::make_pair(host, port);
    }

}// namespace


struct TCPServer::Impl {

    Impl(int port, int backlog)
        : socket(createSocket()) {

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        if (::bind(socket.sockfd_, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {

            throwSocketError("Bind failed");
        }

        if (::listen(socket.sockfd_, backlog) < 0) {

            throwSocketError("Listen failed");
        }
    }

    std::unique_ptr<SimpleConnection> accept() {
        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);
        SOCKET new_sock = ::accept(socket.sockfd_, reinterpret_cast<sockaddr*>(&client_addr), &addrlen);

        if (new_sock == INVALID_SOCKET) {

            throwSocketError("Accept failed");
        }

        return std::make_unique<Socket>(new_sock);
    }

    void close() {

        socket.close();
    }

private:
#ifdef _WIN32
    WSASession session;
#endif

    Socket socket;
};

TCPServer::TCPServer(uint16_t port, int backlog)
    : pimpl_(std::make_unique<Impl>(port, backlog)) {}

[[nodiscard]] std::unique_ptr<SimpleConnection> TCPServer::accept() {

    return pimpl_->accept();
}

void TCPServer::close() {

    pimpl_->close();
}

TCPServer::~TCPServer() = default;


[[nodiscard]] std::unique_ptr<SimpleConnection> TCPClientContext::connect(const std::string& ip, uint16_t port) {

    SOCKET sock = createSocket();

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {

        return nullptr;
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) >= 0) {

        return std::make_unique<Socket>(sock);
    }

    return nullptr;
}

std::unique_ptr<SimpleConnection> TCPClientContext::connect(const std::string& host) {
    const auto [ip, port] = parseHostPort(host);
    return connect(ip, port);
}
