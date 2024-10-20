
#ifndef SIMPLE_SOCKET_UNIXDOMAINSOCKET_HPP
#define SIMPLE_SOCKET_UNIXDOMAINSOCKET_HPP

#include "simple_socket/SocketContext.hpp"

#include <memory>
#include <string>

namespace simple_socket {

    class UnixDomainClientContext: public SocketContext {
    public:
        [[nodiscard]] std::unique_ptr<SimpleConnection> connect(const std::string& domain) override;
    };

    class UnixDomainServer {
    public:
        explicit UnixDomainServer(const std::string& domain, int backlog = 1);

        [[nodiscard]] std::unique_ptr<SimpleConnection> accept();

        void close();

        ~UnixDomainServer();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_UNIXDOMAINSOCKET_HPP
