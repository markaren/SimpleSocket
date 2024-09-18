
#include "TCPSocket.hpp"

#include "common.hpp"


struct TCPSocket::Impl {

    Impl(): sockfd_(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) {

        if (sockfd_ == INVALID_SOCKET) {
            throwSocketError("Failed to create socket");
        }

        const int optval = 1;
        setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval));
    }

    bool connect(const std::string& ip, int port) const {
        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {

            return false;
        }

        return ::connect(sockfd_, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) >= 0;
    }

    void bind(int port) const {

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        if (::bind(sockfd_, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {

            throwSocketError("Bind failed");
        }
    }

    void listen(int backlog) const {

        if (::listen(sockfd_, backlog) < 0) {

            throwSocketError("Listen failed");
        }
    }

    [[nodiscard]] std::unique_ptr<TCPConnection> accept() const {

        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);
        SOCKET new_sock = ::accept(sockfd_, reinterpret_cast<sockaddr*>(&client_addr), &addrlen);

        if (new_sock == INVALID_SOCKET) {

            throwSocketError("Accept failed");
        }

        auto conn = std::make_unique<TCPSocket>();
        conn->pimpl_->assign(new_sock);

        return conn;
    }

    bool read(unsigned char* buffer, size_t size, size_t* bytesRead) const {

#ifdef _WIN32
        const auto read = recv(sockfd_, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0);
#else
        const auto read = ::read(sockfd_, buffer, size);
#endif
        if (bytesRead) *bytesRead = read;

        return (read != SOCKET_ERROR) && (read != 0);
    }

    bool readExact(unsigned char* buffer, size_t size) const {

        size_t totalBytesReceived = 0;
        while (totalBytesReceived < size) {
            const auto remainingBytes = size - totalBytesReceived;
#ifdef _WIN32
            const auto read = recv(sockfd_, reinterpret_cast<char*>(buffer + totalBytesReceived), static_cast<int>(remainingBytes), 0);
#else
            const auto read = ::read(sockfd_, buffer + totalBytesReceived, remainingBytes);
#endif
            if (read == SOCKET_ERROR || read == 0) {

                return false;
            }
            totalBytesReceived += read;
        }

        return totalBytesReceived == size;
    }

    bool write(const std::string& buffer) const {

        const auto size = static_cast<int>(buffer.size());

        return send(sockfd_, buffer.data(), size, 0) != SOCKET_ERROR;
    }

    bool write(const std::vector<unsigned char>& buffer) const {

#ifdef _WIN32
        return send(sockfd_, reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()), 0) != SOCKET_ERROR;
#else
        return ::write(sockfd_, buffer.data(), buffer.size()) != SOCKET_ERROR;
#endif
    }

    void close() const {

        closeSocket(sockfd_);
    }

    ~Impl() {

        close();
    }

private:
#ifdef _WIN32
    WSASession session_;
#endif

    SOCKET sockfd_;

    void assign(SOCKET new_sock) {
        close();

        sockfd_ = new_sock;
    }
};

TCPSocket::TCPSocket(): pimpl_(std::make_unique<Impl>()) {}

int TCPSocket::read(std::vector<unsigned char>& buffer) {

    size_t bytesRead = 0;
    const auto success = pimpl_->read(buffer.data(), buffer.size(), &bytesRead);

    return success ? static_cast<int>(bytesRead) : -1;
}

int TCPSocket::read(unsigned char* buffer, size_t size) {

    size_t bytesRead = 0;
    const auto success = pimpl_->read(buffer, size, &bytesRead);

    return success ? static_cast<int>(bytesRead) : -1;
}

bool TCPSocket::readExact(std::vector<unsigned char>& buffer) {

    return pimpl_->readExact(buffer.data(), buffer.size());
}

bool TCPSocket::readExact(unsigned char* buffer, size_t size) {

    return pimpl_->readExact(buffer, size);
}

bool TCPSocket::write(const std::string& buffer) {

    if (buffer.empty()) return false;

    return pimpl_->write(buffer);
}

bool TCPSocket::write(const std::vector<unsigned char>& buffer) {

    return pimpl_->write(buffer);
}

void TCPSocket::close() {

    pimpl_->close();
}

TCPSocket::~TCPSocket() = default;

bool TCPClient::connect(const std::string& ip, int port) {

    return pimpl_->connect(ip, port);
}

std::unique_ptr<TCPConnection> TCPServer::accept() {

    return pimpl_->accept();
}

TCPServer::TCPServer(int port, int backlog) {

    pimpl_->bind(port);
    pimpl_->listen(backlog);
}
