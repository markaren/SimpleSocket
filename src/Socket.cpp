
#include "Socket.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
using SOCKET = int;
#endif

#include <iostream>
#include <stdexcept>

struct Socket::Impl {

    Impl() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("Failed to initialize winsock");
        }
#endif
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

        auto conn = std::make_unique<Socket>();
        conn->pimpl_->assign(new_sock);

        return conn;
    }

    bool read(unsigned char* buffer, size_t size, size_t* bytesRead) {

#ifdef _WIN32
        auto read = recv(sockfd, reinterpret_cast<char*>(buffer), size, 0);
        if (bytesRead) {
            *bytesRead = read;
        }

        return read != SOCKET_ERROR && read != 0;
#else
        auto read = ::read(sockfd, reinterpret_cast<char*>(buffer), size);
        if (bytesRead)
            *bytesRead = read;
        return read != -1;
#endif
    }

    bool readExact(unsigned char* buffer, size_t size) {

        int totalBytesReceived = 0;
        while (totalBytesReceived < size) {
#ifdef _WIN32
            auto read = recv(sockfd, reinterpret_cast<char*>(buffer), size, 0);
            if (read == SOCKET_ERROR || read == 0) {
                return false;
            }
            totalBytesReceived += read;
#else
            auto read = ::read(sockfd, reinterpret_cast<char*>(buffer), size);
            if (read == -1 || read == 0) {
                return false;
            }
            totalBytesReceived += read;
#endif
        }

        return true;
    }

    bool write(const std::string& buffer) {

#ifdef _WIN32
        return send(sockfd, buffer.data(), buffer.size(), 0) != SOCKET_ERROR;
#else
        return ::write(sockfd, buffer.data(), buffer.size()) != -1;
#endif
    }

    bool write(const std::vector<unsigned char>& buffer) {

#ifdef _WIN32
        return send(sockfd, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0) != SOCKET_ERROR;
#else
        return ::write(sockfd, buffer.data(), buffer.size()) != -1;
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

#ifdef _WIN32
        WSACleanup();
#endif
    }

private:
    SOCKET sockfd;
    bool closed{false};
};

Socket::Socket(): pimpl_(std::make_unique<Impl>()) {}

bool Socket::read(std::vector<unsigned char>& buffer, size_t* bytesRead) {

    return pimpl_->read(buffer.data(), buffer.size(), bytesRead);
}

bool Socket::read(unsigned char* buffer, size_t size, size_t* bytesRead) {

    return pimpl_->read(buffer, size, bytesRead);
}

bool Socket::readExact(std::vector<unsigned char>& buffer) {

    return pimpl_->readExact(buffer.data(), buffer.size());
}

bool Socket::readExact(unsigned char* buffer, size_t size) {

    return pimpl_->readExact(buffer, size);
}

bool Socket::write(const std::string& buffer) {

    return pimpl_->write(buffer);
}

bool Socket::write(const std::vector<unsigned char>& buffer) {

    return pimpl_->write(buffer);
}

bool Socket::connect(const std::string& ip, int port) {

    return pimpl_->connect(ip, port);
}

void Socket::bind(int port) {

    pimpl_->bind(port);
}

void Socket::listen(int backlog) {

    pimpl_->listen(backlog);
}

std::unique_ptr<Connection> Socket::accept() {

    return pimpl_->accept();
}

void Socket::close() {

    pimpl_->close();
}

Socket::~Socket() = default;

bool ClientSocket::connect(const std::string& ip, int port) {

    return Socket::connect(ip, port);
}

void ServerSocket::bind(int port) {

    Socket::bind(port);
}

void ServerSocket::listen(int backlog) {

    Socket::listen(backlog);
}

std::unique_ptr<Connection> ServerSocket::accept() {

    return Socket::accept();
}
