
#include "simple_socket/TCPSocket.hpp"

#include "simple_socket/SimpleConnection.hpp"
#include "simple_socket/Socket.hpp"

#ifdef SIMPLE_SOCKET_WITH_TLS
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#ifdef _WIN32
#include <ws2def.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#endif


using namespace simple_socket;

namespace {
    SOCKET createSocket() {
        SOCKET sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd == INVALID_SOCKET) {
            throwSocketError("Failed to create socket");
        }

        // const int optval = 1;
        // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval));

        return sockfd;
    }

    std::pair<std::string, uint16_t> parseHostPort(const std::string& input) {
        const size_t colonPos = input.find(':');
        if (colonPos == std::string::npos) {
            throw std::invalid_argument("Invalid input format. Expected 'host:port'.");
        }

        std::string host = input.substr(0, colonPos);
        std::string portStr = input.substr(colonPos + 1);

        // Convert port string to uint16_t
        uint16_t port;
        try {
            port = static_cast<uint16_t>(std::stoi(portStr));
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid port number.");
        }

        return std::make_pair(host, port);
    }

}// namespace


struct TCPServer::Impl {

    Impl(int port, int backlog, bool useTLS = false, const std::string& cert_file = "", const std::string& key_file = "")
        : socket(createSocket()), useTLS(useTLS) {

        if (useTLS) {
#ifdef SIMPLE_SOCKET_WITH_TLS
            initTLS(cert_file, key_file);
#else
            throw std::runtime_error("TLS support is not enabled in this build.");
#endif
        }


        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        if (::bind(socket.sockfd_, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {

            throwSocketError("Bind failed");
        }

        if (::listen(socket.sockfd_, backlog) < 0) {

            throwSocketError("Listen failed");
        }
    }

    std::unique_ptr<SimpleConnection> accept() {
        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);
        SOCKET new_sock = ::accept(socket.sockfd_, reinterpret_cast<sockaddr*>(&client_addr), &addrlen);

        if (new_sock == INVALID_SOCKET) {

            throwSocketError("Accept failed");
        }

#ifdef SIMPLE_SOCKET_WITH_TLS
        if (useTLS) {
            SSL* ssl = SSL_new(ctx);
            SSL_set_fd(ssl, static_cast<int>(new_sock));

            if (SSL_accept(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_free(ssl);
                closeSocket(new_sock);
                throw std::runtime_error("TLS handshake failed");
            }

            return std::make_unique<TLSConnection>(new_sock, ssl);
        }
#endif

        return std::make_unique<Socket>(new_sock);
    }

    void close() {

        socket.close();
#ifdef SIMPLE_SOCKET_WITH_TLS
        if (ctx) {
            SSL_CTX_free(ctx);
            ctx = nullptr;
        }
#endif
    }

private:
#ifdef _WIN32
    WSASession session;
#endif

    Socket socket;
    bool useTLS{false};
#ifdef SIMPLE_SOCKET_WITH_TLS
    SSL_CTX* ctx = nullptr;

    void initTLS(const std::string& cert_file, const std::string& key_file) {
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();

        const SSL_METHOD* method = TLS_server_method();
        ctx = SSL_CTX_new(method);
        if (!ctx)
            throw std::runtime_error("Failed to create SSL_CTX");

        if (SSL_CTX_use_certificate_file(ctx, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0)
            throw std::runtime_error("Failed to load certificate");

        if (SSL_CTX_use_PrivateKey_file(ctx, key_file.c_str(), SSL_FILETYPE_PEM) <= 0)
            throw std::runtime_error("Failed to load private key");

        if (!SSL_CTX_check_private_key(ctx))
            throw std::runtime_error("Private key does not match certificate");
    }
#endif
};

TCPServer::TCPServer(uint16_t port, int backlog, bool useTLS, const std::string& cert_file, const std::string& key_file)
    : pimpl_(std::make_unique<Impl>(port, backlog, useTLS, cert_file, key_file)) {}

[[nodiscard]] std::unique_ptr<SimpleConnection> TCPServer::accept() {

    return pimpl_->accept();
}

void TCPServer::close() {

    pimpl_->close();
}

TCPServer::~TCPServer() = default;


[[nodiscard]] std::unique_ptr<SimpleConnection> TCPClientContext::connect(const std::string& ip, uint16_t port, bool useTLS) {

    SOCKET sock = createSocket();

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {

        // Fallback: resolve hostname
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(ip.c_str(), nullptr, &hints, &res) != 0 || !res) {
            closeSocket(sock);
            return nullptr;
        }
        serv_addr.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;
        freeaddrinfo(res);
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {

        return nullptr;
    }
    if (useTLS) {
#ifdef SIMPLE_SOCKET_WITH_TLS

        SSL_library_init();
        SSL_load_error_strings();
        const SSL_METHOD* method = TLS_client_method();
        SSL_CTX* ctx = SSL_CTX_new(method);
        if (!ctx) return nullptr;

        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, static_cast<int>(sock));
        SSL_set_tlsext_host_name(ssl, ip.c_str());
        if (SSL_connect(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            closeSocket(sock);
            return nullptr;
        }
        return std::make_unique<TLSConnection>(sock, ssl, ctx);
#else
        throw std::runtime_error("TLS support is not enabled in this build.");
#endif
    }
    return std::make_unique<Socket>(sock);
}

std::unique_ptr<SimpleConnection> TCPClientContext::connect(const std::string& host) {
    const auto [ip, port] = parseHostPort(host);
    return connect(ip, port);
}
