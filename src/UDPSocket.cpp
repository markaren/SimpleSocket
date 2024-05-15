
#include "UDPSocket.hpp"

#ifdef _WIN32
#include <WS2tcpip.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

#include <stdexcept>
#include <system_error>

struct UDPSocket::Impl {

    explicit Impl(int port): sockfd(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {

        if (sockfd == INVALID_SOCKET) {
#ifdef _WIN32
            throw std::system_error(WSAGetLastError(), std::system_category(), "Failed to create socket");
#else
            throw std::system_error(errno, std::generic_category(), "Failed to create socket");
#endif
        }

        sockaddr_in local{};
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = htons(port);

        if (::bind(sockfd, (sockaddr*) &local, sizeof(local)) == SOCKET_ERROR) {
#if WIN32
            throw std::system_error(WSAGetLastError(), std::system_category(), "Bind failed");
#else
            throw std::system_error(errno, std::generic_category(), "Bind failed");
#endif
        }
    }

    bool sendTo(const std::string& address, uint16_t port, const std::string& data) {
        sockaddr_in to{};
        to.sin_family = AF_INET;
        to.sin_port = htons(port);
        if (!inet_pton(AF_INET, address.c_str(), &to.sin_addr)) {
            return false;
        }

        return sendto(sockfd, data.c_str(), data.size(), 0, (sockaddr*) &to, sizeof(to)) != SOCKET_ERROR;
    }

    int recvFrom(const std::string& address, uint16_t port, std::vector<unsigned char>& buffer) {

        sockaddr_in from{};
        from.sin_family = AF_INET;
        from.sin_port = htons(port);
        if (!inet_pton(AF_INET, address.c_str(), &from.sin_addr)) {
            return false;
        }
        socklen_t fromLength = sizeof(from);

        int receive = recvfrom(sockfd, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0, (sockaddr*) &from, &fromLength);
        if (receive == SOCKET_ERROR) {
            return -1;
        }

        return receive;
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
    bool closed{false};
    SOCKET sockfd;
};


UDPSocket::UDPSocket(int port)
    : pimpl_(std::make_unique<Impl>(port)) {}

bool UDPSocket::sendTo(const std::string& address, uint16_t port, const std::string& data) {

    return pimpl_->sendTo(address, port, data);
}

int UDPSocket::recvFrom(const std::string& address, uint16_t port, std::vector<unsigned char>& buffer) {

    return pimpl_->recvFrom(address, port, buffer);
}

void UDPSocket::close() {

    pimpl_->close();
}

UDPSocket::~UDPSocket() = default;
