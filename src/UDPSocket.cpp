
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
        sockaddr_in server{};
        if (!inet_pton(AF_INET, address.c_str(), &server.sin_addr)) {
            return false;
        }
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        return (sendto(sockfd, data.c_str(), data.size(), 0, (struct sockaddr*) &server, sizeof(server)) != SOCKET_ERROR);
    }

private:
    SOCKET sockfd;
};


UDPSocket::UDPSocket(): pimpl_(std::make_unique<Impl>()) {
}

void UDPSocket::sendTo(const std::string& address, uint16_t port, const std::string& data) {

    pimpl_->sendTo(address, port, data);
}

//std::string UDPSocket::recvFrom() {
//    char buffer[1024];
//    struct sockaddr_in from;
//    socklen_t fromLength = sizeof(from);
//
//    int received = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &fromLength);
//    if (received == SOCKET_ERROR) {
//        std::cerr << "recvfrom() failed";
//        exit(-1);
//    }
//
//    return std::string(buffer, received);
//}
