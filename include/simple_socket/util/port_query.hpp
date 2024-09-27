
#ifndef SIMPLE_SOCKET_PORT_QUERY_HPP
#define SIMPLE_SOCKET_PORT_QUERY_HPP

#include <cstdint>
#include <optional>
#include <vector>

namespace simple_socket {

    std::optional<uint16_t> getAvailablePort(uint16_t startPort, uint16_t endPort, const std::vector<uint16_t>& excludePorts = {});

}

#endif//SIMPLE_SOCKET_PORT_QUERY_HPP
