
#include "WSASession.hpp"

#if WIN32
#include <WinSock2.h>
#include <system_error>
#endif

struct WSASession::Impl {
    Impl() {
#if WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {

            throw std::system_error(WSAGetLastError(), std::system_category(), "Failed to initialize winsock");
        }
#endif
    }

    ~Impl() {
#if WIN32
        WSACleanup();
#endif
    }
};

WSASession::WSASession() : pimpl_(std::make_unique<Impl>()) {}

WSASession::~WSASession() = default;
