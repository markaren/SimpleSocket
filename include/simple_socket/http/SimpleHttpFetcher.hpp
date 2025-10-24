
#ifndef SIMPLE_SOCKET_SIMPLEHTTPFETCHER_HPP
#define SIMPLE_SOCKET_SIMPLEHTTPFETCHER_HPP

#include "simple_socket/TCPSocket.hpp"

#include <optional>
#include <string>
#include <vector>

namespace simple_socket {

    class SimpleHttpFetcher {
    public:
        std::optional<std::string> fetch(const std::string& url);
        std::optional<std::vector<uint8_t>> fetchBytes(const std::string& url);

    private:
        TCPClientContext context;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_SIMPLEHTTPFETCHER_HPP
