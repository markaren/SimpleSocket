
#include "UDPSocket.hpp"

#ifdef _WIN32
#include <WS2tcpip.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif


#include <stdexcept>

struct UDPSocket::Impl {
    Impl() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("Failed to initialize winsock");
        }
#endif

        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }

    bool sendTo(const std::string& address, uint16_t port, const std::string& data) {
        sockaddr_in to{};
        to.sin_family = AF_INET;
        to.sin_port = htons(port);
        if (!inet_pton(AF_INET, address.c_str(), &to.sin_addr)) {
            return false;
        }

        return (sendto(sockfd, data.c_str(), data.size(), 0, (struct sockaddr*) &to, sizeof(to)) != SOCKET_ERROR);
    }

    int recvFrom(const std::string& address, uint16_t port, std::vector<unsigned char>& buffer) {
        sockaddr_in from{};
        from.sin_family = AF_INET;
        from.sin_port = htons(port);
        if (!inet_pton(AF_INET, address.c_str(), &from.sin_addr)) {
            return false;
        }
        socklen_t fromLength = sizeof(from);

        return recvfrom(sockfd, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0, (struct sockaddr*) &from, &fromLength);
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
    bool closed{false};
    SOCKET sockfd;
};


UDPSocket::UDPSocket(): pimpl_(std::make_unique<Impl>()) {
}

void UDPSocket::sendTo(const std::string& address, uint16_t port, const std::string& data) {

    pimpl_->sendTo(address, port, data);
}

int UDPSocket::recvFrom(const std::string& address, uint16_t port, std::vector<unsigned char>& buffer) {

    return pimpl_->recvFrom(address, port, buffer);
}

void UDPSocket::close() {
}