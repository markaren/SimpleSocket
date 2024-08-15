
#ifndef SIMPLE_SOCKET_COMMON_HPP
#define SIMPLE_SOCKET_COMMON_HPP


#ifdef _WIN32
#include "WSASession.hpp"
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
using SOCKET = int;
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

#include <system_error>


inline void throwSocketError(const std::string& msg) {

#ifdef _WIN32
    throw std::system_error(WSAGetLastError(), std::system_category(), msg);
#else
    throw std::system_error(errno, std::generic_category(), msg);
#endif
}

inline void closeSocket(SOCKET socket) {

    if (socket != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(socket);
#else
        shutdown(socket, SHUT_RD);
        close(socket);
#endif
        socket = INVALID_SOCKET;
    }
}


#endif//SIMPLE_SOCKET_COMMON_HPP
