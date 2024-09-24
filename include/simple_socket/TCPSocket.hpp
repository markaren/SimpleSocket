
#ifndef SIMPLE_SOCKET_TCPSOCKET_HPP
#define SIMPLE_SOCKET_TCPSOCKET_HPP

#include "simple_socket/SimpleConnection.hpp"

#include <memory>

namespace simple_socket {

    class TCPClientContext {
    public:
        TCPClientContext();

        std::unique_ptr<SimpleConnection> connect(const std::string& ip, int port);

        ~TCPClientContext();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

    class TCPServer {
    public:
        explicit TCPServer(int port, int backlog = 1);

        std::unique_ptr<SimpleConnection> accept();

        void close();

        ~TCPServer();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif// SIMPLE_SOCKET_TCPSOCKET_HPP
