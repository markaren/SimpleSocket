
#ifndef SIMPLE_SOCKET_SOCKET_HPP
#define SIMPLE_SOCKET_SOCKET_HPP

#include "simple_socket/SimpleConnection.hpp"
#include "simple_socket/socket_common.hpp"

#ifdef SIMPLE_SOCKET_WITH_TLS
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

// #ifndef _WIN32
// #include <fcntl.h>
// #endif


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


//     // Helper: put socket into non-blocking mode
//     inline void set_nonblocking(SOCKET s) {
// #ifdef _WIN32
//         u_long mode = 1;
//         ioctlsocket(s, FIONBIO, &mode);
// #else
//         int flags = fcntl(s, F_GETFL, 0);
//         if (flags >= 0) fcntl(s, F_SETFL, flags | O_NONBLOCK);
// #endif
//     }


    class TLSConnection: public SimpleConnection {
    public:
        TLSConnection(SOCKET sock, SSL* ssl, SSL_CTX* ctx = nullptr)
            : sockfd_(sock), ssl_(ssl), ctx_(ctx) {

            // set_nonblocking(sockfd_);
            // // Ensure AUTO_RETRY is off for non-blocking semantics
            // SSL_clear_mode(ssl_, SSL_MODE_AUTO_RETRY);
        }


        bool write(const uint8_t* buf, size_t len) override {
            if (!ssl_) return false;

            size_t total = 0;
            while (total < len) {
                const int n = SSL_write(ssl_, buf + total, static_cast<int>(len - total));
                if (n > 0) {
                    total += static_cast<size_t>(n);
                    continue;
                }
                const int err = SSL_get_error(ssl_, n);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    continue; // retry
                }
                return false; // fatal
            }
            return true;
        }

        int read(uint8_t* buf, size_t len) override {
            if (!ssl_) return -1;

            for (;;) {
                const int n = SSL_read(ssl_, buf, static_cast<int>(len));
                if (n > 0) return n;

                const int err = SSL_get_error(ssl_, n);
                if (err == SSL_ERROR_ZERO_RETURN) return 0; // clean TLS shutdown
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    // retry until caller cancels via close()
                    continue;
                }
                return -1; // fatal
            }
        }

        bool readExact(uint8_t* buffer, size_t size) override {
            throw std::runtime_error("TLSConnection::readExact not implemented");
        }

        void close() override {

            closeSocket(sockfd_);

            if (ssl_) {
                SSL_shutdown(ssl_);
                SSL_free(ssl_);
                ssl_ = nullptr;
            }
            if (ctx_) {
                SSL_CTX_free(ctx_);
                ctx_ = nullptr;
            }
        }

        ~TLSConnection() override {
            close();
        }

    private:
        SOCKET sockfd_;
        SSL* ssl_;
        SSL_CTX* ctx_;

    };
#endif

}// namespace simple_socket

#endif//SIMPLE_SOCKET_SOCKET_HPP
