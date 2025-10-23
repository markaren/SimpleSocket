
#ifndef SIMPLE_SOCKET_TLSCONNECTION_HPP
#define SIMPLE_SOCKET_TLSCONNECTION_HPP

#include "simple_socket/SimpleConnection.hpp"
#include "simple_socket/socket_common.hpp"

#include <openssl/err.h>
#include <openssl/ssl.h>

#ifndef _WIN32
#include <fcntl.h>
#endif

namespace simple_socket {

    class TLSConnection: public SimpleConnection {
    public:
        TLSConnection(SOCKET sock, const std::string& ip)
            : sockfd_(sock) {

            SSL_library_init();
            SSL_load_error_strings();
            const SSL_METHOD* method = TLS_client_method();
            ctx_ = SSL_CTX_new(method);
            if (!ctx_) throw std::runtime_error("Failed to create SSL context");

            ssl_ = SSL_new(ctx_);
            SSL_set_fd(ssl_, static_cast<int>(sock));
            SSL_set_tlsext_host_name(ssl_, ip.c_str());
            if (SSL_connect(ssl_) <= 0) {
                ERR_print_errors_fp(stderr);
                throw std::runtime_error("Failed to connect to TLS host");
            }

            set_nonblocking(sockfd_);
            // Ensure AUTO_RETRY is off for non-blocking semantics
            SSL_clear_mode(ssl_, SSL_MODE_AUTO_RETRY);
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
                    continue;// retry
                }
                return false;// fatal
            }
            return true;
        }

        int read(uint8_t* buf, size_t len) override {
            if (!ssl_) return -1;

            for (;;) {
                const int n = SSL_read(ssl_, buf, static_cast<int>(len));
                if (n > 0) return n;

                const int err = SSL_get_error(ssl_, n);
                if (err == SSL_ERROR_ZERO_RETURN) return 0;// clean TLS shutdown
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                    // retry until caller cancels via close()
                    continue;
                }
                return -1;// fatal
            }
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
            TLSConnection::close();
        }

    private:
        SOCKET sockfd_;
        SSL* ssl_;
        SSL_CTX* ctx_;

        // Helper: put socket into non-blocking mode
        static void set_nonblocking(SOCKET s) {
#ifdef _WIN32
            u_long mode = 1;
            ioctlsocket(s, FIONBIO, &mode);
#else
            int flags = fcntl(s, F_GETFL, 0);
            if (flags >= 0) fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
        }
    };


}

#endif //TLSCONNECTION_HPP
