
#ifndef SIMPLE_SOCKET_TCPSOCKET_HPP
#define SIMPLE_SOCKET_TCPSOCKET_HPP

#include "simple_socket/SimpleConnection.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace simple_socket {

    class TCPClientContext {
    public:
        TCPClientContext();

        TCPClientContext(const TCPClientContext& other) = delete;
        TCPClientContext& operator=(const TCPClientContext& other) = delete;
        TCPClientContext(TCPClientContext&& other) = delete;
        TCPClientContext& operator=(TCPClientContext&& other) = delete;

        std::unique_ptr<SimpleConnection> connect(const std::string& ip, uint16_t port);

        ~TCPClientContext();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

    class TCPServer {
    public:
        explicit TCPServer(uint16_t port, int backlog = 1);

        TCPServer(const TCPServer&) = delete;
        TCPServer& operator=(const TCPServer&) = delete;
        TCPServer(TCPServer&&) = delete;
        TCPServer& operator=(TCPServer&&) = delete;

        std::unique_ptr<SimpleConnection> accept();

        void close();

        ~TCPServer();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif// SIMPLE_SOCKET_TCPSOCKET_HPP
