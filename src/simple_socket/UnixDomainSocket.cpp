
#include "simple_socket/UnixDomainSocket.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/Socket.hpp"

using namespace simple_socket;


namespace {

    SOCKET createSocket() {
        SOCKET sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd == INVALID_SOCKET) {
            throwSocketError("Failed to create socket");
        }

        return sockfd;
    }

    void unlinkPath(const std::string& path) {
#ifdef _WIN32
        DeleteFile(path.c_str());
#else
        unlink(path.c_str());
#endif
    }

}


struct UnixDomainServer::Impl {

    explicit Impl(const std::string& domain, int backlog)
        : socket(createSocket()) {

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

private:
#ifdef _WIN32
    WSASession session;
#endif

    Socket socket;
};


UnixDomainServer::UnixDomainServer(const std::string& domain, int backlog)
       : pimpl_(std::make_unique<Impl>(domain, backlog)) {}

void UnixDomainServer::close() {

    pimpl_->close();
}

std::unique_ptr<SimpleConnection> UnixDomainServer::accept() {

    return pimpl_->accept();
}

UnixDomainServer::~UnixDomainServer() = default;


std::unique_ptr<SimpleConnection> UnixDomainClientContext::connect(const std::string& domain) {

    SOCKET sockfd = createSocket();

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, domain.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) >= 0) {

        return std::make_unique<Socket>(sockfd);
    }

    return nullptr;
}
