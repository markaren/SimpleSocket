
#ifndef SIMPLE_SOCKET_UNIXDOMAINSOCKET_HPP
#define SIMPLE_SOCKET_UNIXDOMAINSOCKET_HPP

#include "simple_socket/Socket.hpp"

namespace simple_socket {

    class UnixDomainClient: public Socket {
    public:
        bool connect(const std::string& domain);
    };

    class UnixDomainServer: public Socket {
    public:
        explicit UnixDomainServer(const std::string& domain, int backlog = 1);

        std::unique_ptr<SocketConnection> accept();
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_UNIXDOMAINSOCKET_HPP
