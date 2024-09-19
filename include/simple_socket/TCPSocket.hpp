
#ifndef SIMPLE_SOCKET_TCPSOCKET_HPP
#define SIMPLE_SOCKET_TCPSOCKET_HPP

#include "simple_socket/Socket.hpp"

namespace simple_socket {

    class TCPClient: public Socket {
    public:
        bool connect(const std::string& ip, int port);
    };

    class TCPServer: public Socket {
    public:
        explicit TCPServer(int port, int backlog = 1);

        std::unique_ptr<SocketConnection> accept();
    };

}// namespace simple_socket

#endif// SIMPLE_SOCKET_TCPSOCKET_HPP
