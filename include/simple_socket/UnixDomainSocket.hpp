
#ifndef SIMPLE_SOCKET_UNIXDOMAINSOCKET_HPP
#define SIMPLE_SOCKET_UNIXDOMAINSOCKET_HPP

#include "simple_socket/Socket.hpp"

#include <memory>

namespace simple_socket {

    class UnixDomainClient {
    public:
        static std::unique_ptr<SocketConnection> connect(const std::string& domain);
    };

    class UnixDomainServer {
    public:
        explicit UnixDomainServer(const std::string& domain, int backlog = 1);

        std::unique_ptr<SocketConnection> accept();

        void close();

        ~UnixDomainServer();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_UNIXDOMAINSOCKET_HPP
