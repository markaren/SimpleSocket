
#ifndef SIMPLE_SOCKET_SOCKETINCLUDES_HPP
#define SIMPLE_SOCKET_SOCKETINCLUDES_HPP


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

#include <system_error>


inline void throwError(const std::string& msg) {

#ifdef _WIN32
    throw std::system_error(WSAGetLastError(), std::system_category(), msg);
#else
    throw std::system_error(errno, std::generic_category(), msg);
#endif
}

inline void closeSocket(SOCKET socket) {

#ifdef WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}


#endif//SIMPLE_SOCKET_SOCKETINCLUDES_HPP