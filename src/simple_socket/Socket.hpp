
#ifndef SIMPLE_SOCKET_SOCKET_HPP
#define SIMPLE_SOCKET_SOCKET_HPP

#include "simple_socket/SimpleConnection.hpp"
#include "simple_socket/socket_common.hpp"

namespace simple_socket {

    struct Socket: SimpleConnection {

        explicit Socket(SOCKET socket)
            : sockfd_(socket) {}

        size_t read(unsigned char* buffer, size_t size) override {

#ifdef _WIN32
            const auto read = recv(sockfd_, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0);
#else
            const auto read = ::read(sockfd_, buffer, size);
#endif

            return (read != SOCKET_ERROR) && (read != 0) ? read : -1;
        }

        bool write(const unsigned char* data, size_t size) override {

#ifdef _WIN32
            return send(sockfd_, reinterpret_cast<const char*>(data), static_cast<int>(size), 0) != SOCKET_ERROR;
#else
            return ::write(sockfd_, data, size) != SOCKET_ERROR;
#endif
        }

        void close() override {

            closeSocket(sockfd_);
        }

        ~Socket() override {

            closeSocket(sockfd_);
        }

        SOCKET sockfd_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_SOCKET_HPP
