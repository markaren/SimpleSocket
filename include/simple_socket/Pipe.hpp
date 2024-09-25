
#ifndef SIMPLE_SOCKET_PIPE_HPP
#define SIMPLE_SOCKET_PIPE_HPP

#include "SimpleConnection.hpp"

#include <memory>
#include <string>

namespace simple_socket {

    class NamedPipe {

    public:
        static std::unique_ptr<SimpleConnection> listen(const std::string& name);
        static std::unique_ptr<SimpleConnection> connect(const std::string& name, long timeOut = 5000);
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_PIPE_HPP
