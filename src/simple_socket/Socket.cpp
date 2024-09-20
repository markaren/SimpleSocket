
#include "simple_socket/Socket.hpp"
#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/UnixDomainSocket.hpp"

#include "simple_socket/common.hpp"

namespace simple_socket {

#ifdef _WIN32
    WSASession session;
#endif

    SOCKET createSocket(int domain, int protocol) {
        SOCKET sockfd = socket(domain, SOCK_STREAM, protocol);
        if (sockfd == INVALID_SOCKET) {
            throwSocketError("Failed to create socket");
        }
        if (domain == AF_INET) {
            const int optval = 1;
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval));
        }

        return sockfd;
    }

    struct Socket: SocketConnection {

        explicit Socket(SOCKET socket)
            : sockfd_(socket) {}

        int read(std::vector<unsigned char>& buffer) override {

            return read(buffer.data(), buffer.size());
        }

        int read(unsigned char* buffer, size_t size) override {

#ifdef _WIN32
            const auto read = recv(sockfd_, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0);
#else
            const auto read = ::read(sockfd_, buffer, size);
#endif

            return (read != SOCKET_ERROR) && (read != 0) ? read : -1;
        }

        bool readExact(std::vector<unsigned char>& buffer) override {

            return readExact(buffer.data(), buffer.size());
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

        bool write(const std::string& buffer) override {

#ifdef _WIN32
            return send(sockfd_, buffer.data(), static_cast<int>(buffer.size()), 0) != SOCKET_ERROR;
#else
            return ::write(sockfd_, buffer.data(), buffer.size()) != SOCKET_ERROR;
#endif
        }

        bool write(const std::vector<unsigned char>& buffer) override {

#ifdef _WIN32
            return send(sockfd_, reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()), 0) != SOCKET_ERROR;
#else
            return ::write(sockfd_, buffer.data(), buffer.size()) != SOCKET_ERROR;
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

    std::unique_ptr<SocketConnection> TCPClient::connect(const std::string& ip, int port) {

        SOCKET sock = createSocket(AF_INET, IPPROTO_TCP);

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {

            return nullptr;
        }

        if (::connect(sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) >= 0) {

            return std::make_unique<Socket>(sock);
        }

        return nullptr;
    }


    struct TCPServer::Impl {

        Impl(int port, int backlog)
            : socket(createSocket(AF_INET, IPPROTO_TCP)) {

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

        std::unique_ptr<SocketConnection> accept() {
            sockaddr_in client_addr{};
            socklen_t addrlen = sizeof(client_addr);
            SOCKET new_sock = ::accept(socket.sockfd_, reinterpret_cast<sockaddr*>(&client_addr), &addrlen);

            if (new_sock == INVALID_SOCKET) {

                throwSocketError("Accept failed");
            }

            return std::make_unique<Socket>(new_sock);
        }

        void close() {

            socket.close();
        }

        Socket socket;
    };

    TCPServer::TCPServer(int port, int backlog)
        : pimpl_(std::make_unique<Impl>(port, backlog)) {}

    std::unique_ptr<SocketConnection> TCPServer::accept() {

        return pimpl_->accept();
    }

    void TCPServer::close() {

        pimpl_->close();
    }

    TCPServer::~TCPServer() = default;


    struct UnixDomainServer::Impl {

        explicit Impl(const std::string& domain, int backlog)
            : socket(createSocket(AF_UNIX, 0)) {

            unlinkPath(domain);

            sockaddr_un addr{};
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, domain.c_str(), sizeof(addr.sun_path) - 1);

            if (::bind(socket.sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {

                throwSocketError("Bind failed");
            }

            if (::listen(socket.sockfd_, backlog) < 0) {

                throwSocketError("Listen failed");
            }
        }

        std::unique_ptr<Socket> accept() {

            SOCKET new_sock = ::accept(socket.sockfd_, nullptr, nullptr);
            if (new_sock == INVALID_SOCKET) {

                throwSocketError("Accept failed");
            }

            return std::make_unique<Socket>(new_sock);
        }

        void close() {

            socket.close();
        }

        Socket socket;
    };

    UnixDomainServer::UnixDomainServer(const std::string& domain, int backlog)
        : pimpl_(std::make_unique<Impl>(domain, backlog)) {}

    void UnixDomainServer::close() {

        pimpl_->close();
    }

    std::unique_ptr<SocketConnection> UnixDomainServer::accept() {

        return pimpl_->accept();
    }

    UnixDomainServer::~UnixDomainServer() = default;


    std::unique_ptr<SocketConnection> UnixDomainClient::connect(const std::string& domain) {

        SOCKET sockfd = createSocket(AF_UNIX, 0);

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, domain.c_str(), sizeof(addr.sun_path) - 1);

        if (::connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) >= 0) {

            return std::make_unique<Socket>(sockfd);
        }

        return nullptr;
    }

}// namespace simple_socket
