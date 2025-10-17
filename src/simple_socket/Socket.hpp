
#ifndef SIMPLE_SOCKET_SOCKET_HPP
#define SIMPLE_SOCKET_SOCKET_HPP

#include "simple_socket/SimpleConnection.hpp"
#include "simple_socket/socket_common.hpp"

#ifdef SIMPLE_SOCKET_WITH_TLS
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif


namespace simple_socket {

    struct Socket: SimpleConnection {

        explicit Socket(SOCKET socket)
            : sockfd_(socket) {}

        int read(unsigned char* buffer, size_t size) override {

#ifdef _WIN32
            const auto read = recv(sockfd_, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0);
#else
            const auto read = ::read(sockfd_, buffer, size);
#endif

            return (read != SOCKET_ERROR) && (read != 0) ? read : -1;
        }

        bool readExact(unsigned char* buffer, size_t size) override {

            size_t totalBytesReceived = 0;
            while (totalBytesReceived < size) {
                const auto remainingBytes = size - totalBytesReceived;
#ifdef _WIN32
                const auto read = recv(sockfd_, reinterpret_cast<char*>(buffer + totalBytesReceived), static_cast<int>(remainingBytes), 0);
#else
                const auto read = ::read(sockfd_, buffer + totalBytesReceived, remainingBytes);
#endif
                if (read == SOCKET_ERROR || read == 0) {

                    return false;
                }
                totalBytesReceived += read;
            }

            return totalBytesReceived == size;
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

#ifdef SIMPLE_SOCKET_WITH_TLS
    class TLSConnection: public SimpleConnection {
    public:
        TLSConnection(SOCKET sock, SSL* ssl): sockfd_(sock), ssl_(ssl) {}


        bool write(const uint8_t* buf, size_t len) override {
            const auto written = SSL_write(ssl_, buf, static_cast<int>(len));
            return written > 0;
        }

        int read(uint8_t* buf, size_t len) override {
            const auto read = SSL_read(ssl_, buf, static_cast<int>(len));
            return (read > 0) ? read : -1;
        }

        bool readExact(uint8_t* buffer, size_t size) override {
            size_t totalBytesReceived = 0;
            while (totalBytesReceived < size) {
                const int read = SSL_read(ssl_, buffer + totalBytesReceived,
                                          static_cast<int>(size - totalBytesReceived));
                if (read <= 0)
                    return false;
                totalBytesReceived += read;
            }
            return totalBytesReceived == size;
        }

        void close() override {
            if (ssl_) {
                SSL_shutdown(ssl_);
                SSL_free(ssl_);
                ssl_ = nullptr;
            }
            closeSocket(sockfd_);
        }

        ~TLSConnection() override {
            close();
        }

    private:
        SOCKET sockfd_;
        SSL* ssl_;
    };
#endif

}// namespace simple_socket

#endif//SIMPLE_SOCKET_SOCKET_HPP
