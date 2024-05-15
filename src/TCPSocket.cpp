
#include "TCPSocket.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
using SOCKET = int;
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

#include <iostream>
#include <stdexcept>

struct TCPSocket::Impl {

    Impl(): sockfd(socket(AF_INET, SOCK_STREAM, 0)) {
        if (sockfd == INVALID_SOCKET) {
#ifdef _WIN32
            throw std::system_error(WSAGetLastError(), std::system_category(), "Failed to create socket");
#else
            throw std::system_error(errno, std::generic_category(), "Failed to create socket");
#endif
        }
    }

    bool connect(const std::string& ip, int port) {
        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {

            return false;
        }
        if (::connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {

            return false;
        }

        return true;
    }

    void bind(int port) {

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        if (::bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {

            throw std::runtime_error("Bind failed");
        }
    }

    void listen(int backlog) {

        if (::listen(sockfd, backlog) < 0) {

            throw std::runtime_error("Listen failed");
        }
    }

    std::unique_ptr<Connection> accept() {

        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);
        SOCKET new_sock = ::accept(sockfd, (struct sockaddr*) &client_addr, &addrlen);

#ifdef _WIN32
        if (new_sock == INVALID_SOCKET) {
            throw std::runtime_error("Accept failed");
        }
#else
        if (new_sock < 0) {
            throw std::runtime_error("Accept failed");
        }
#endif

        auto conn = std::make_unique<TCPSocket>();
        conn->pimpl_->assign(new_sock);

        return conn;
    }

    bool read(unsigned char* buffer, size_t size, size_t* bytesRead) {

#ifdef _WIN32
        auto read = recv(sockfd, reinterpret_cast<char*>(buffer), size, 0);
#else
        auto read = ::read(sockfd, reinterpret_cast<char*>(buffer), size);
#endif
        if (bytesRead) {
            *bytesRead = read;
        }

        return read != SOCKET_ERROR;
    }

    bool readExact(unsigned char* buffer, size_t size) {

        int totalBytesReceived = 0;
        while (totalBytesReceived < size) {
#ifdef _WIN32
            auto read = recv(sockfd, reinterpret_cast<char*>(buffer), size, 0);
#else
            auto read = ::read(sockfd, reinterpret_cast<char*>(buffer), size);
#endif
            if (read == SOCKET_ERROR) {
                return false;
            }
            totalBytesReceived += read;
        }

        return true;
    }

    bool write(const std::string& buffer) {

        return send(sockfd, buffer.data(), buffer.size(), 0) != SOCKET_ERROR;
    }

    bool write(const std::vector<unsigned char>& buffer) {

#ifdef _WIN32
        return send(sockfd, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0) != SOCKET_ERROR;
#else
        return ::write(sockfd, buffer.data(), buffer.size()) != SOCKET_ERROR;
#endif
    }

    void assign(SOCKET new_sock) {
        close();

        sockfd = new_sock;
        closed = false;
    }

    void close() {

        if (!closed) {
            closed = true;
#ifdef _WIN32
            closesocket(sockfd);
#else
            ::close(sockfd);
#endif
        }
    }

    ~Impl() {

        close();
    }

private:
    SOCKET sockfd;
    bool closed{false};
};

TCPSocket::TCPSocket(): pimpl_(std::make_unique<Impl>()) {}

int TCPSocket::read(std::vector<unsigned char>& buffer) {

    size_t bytesRead = 0;
    auto success = pimpl_->read(buffer.data(), buffer.size(), &bytesRead);

    return success ? static_cast<int>(bytesRead) : -1;
}

int TCPSocket::read(unsigned char* buffer, size_t size) {

    size_t bytesRead = 0;
    auto success = pimpl_->read(buffer, size, &bytesRead);

    return success ? static_cast<int>(bytesRead) : -1;
}

bool TCPSocket::readExact(std::vector<unsigned char>& buffer) {

    return pimpl_->readExact(buffer.data(), buffer.size());
}

bool TCPSocket::readExact(unsigned char* buffer, size_t size) {

    return pimpl_->readExact(buffer, size);
}

bool TCPSocket::write(const std::string& buffer) {

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

std::unique_ptr<Connection> TCPServer::accept() {

    return pimpl_->accept();
}

TCPServer::TCPServer(int port, int backlog) {
    pimpl_->bind(port);
    pimpl_->listen(backlog);
}
