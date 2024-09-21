
#ifndef SIMPLE_SOCKET_WSASESSION_HPP
#define SIMPLE_SOCKET_WSASESSION_HPP

#include <WinSock2.h>
#include <memory>
#include <mutex>
#include <system_error>

namespace {

    int ref_count_ = 0;
    std::mutex mutex_;

}// namespace

namespace simple_socket {

    class WSASession {
    public:
        WSASession() {
            std::lock_guard lock(mutex_);
            if (ref_count_ == 0) {
                WSADATA wsaData;
                if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                    throw std::system_error(WSAGetLastError(), std::system_category(), "Failed to initialize winsock");
                }
            }
            ++ref_count_;
        }

        ~WSASession() {
            std::lock_guard lock(mutex_);
            if (--ref_count_ == 0) {
                WSACleanup();
            }
        }
    };

}// namespace simple_socket


#endif//SIMPLE_SOCKET_WSASESSION_HPP
