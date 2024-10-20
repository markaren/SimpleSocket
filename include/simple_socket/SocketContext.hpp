
#ifndef SIMPLE_SOCKET_SOCKETCONTEXT_HPP
#define SIMPLE_SOCKET_SOCKETCONTEXT_HPP

#include "simple_socket/SimpleConnection.hpp"

#include <memory>
#include <string>

namespace simple_socket {

    class SocketContext {
    public:
        SocketContext();

        SocketContext(const SocketContext& other) = delete;
        SocketContext& operator=(const SocketContext& other) = delete;
        SocketContext(SocketContext&& other) = delete;
        SocketContext& operator=(SocketContext&& other) = delete;

        [[nodiscard]] virtual std::unique_ptr<SimpleConnection> connect(const std::string&) = 0;

        virtual ~SocketContext();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_SOCKETCONTEXT_HPP
