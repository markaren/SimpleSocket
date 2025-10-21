
#ifndef SIMPLE_SOCKET_SIMPLEHTTPFETCHER_HPP
#define SIMPLE_SOCKET_SIMPLEHTTPFETCHER_HPP

#include "TCPSocket.hpp"


#include <optional>
#include <string>

namespace simple_socket {

    class SimpleHttpFetcher {
    public:
        std::optional<std::string> fetch(const std::string& url);

    private:
        TCPClientContext context;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_SIMPLEHTTPFETCHER_HPP
