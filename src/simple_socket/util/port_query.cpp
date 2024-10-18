
#include "simple_socket/util/port_query.hpp"

#include "simple_socket/socket_common.hpp"

#include <optional>
#include <algorithm>

using namespace simple_socket;

std::optional<uint16_t> simple_socket::getAvailablePort(uint16_t startPort, uint16_t endPort, const std::vector<uint16_t>& excludePorts) {

#ifdef _WIN32
    WSASession session;
#endif

    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == SOCKET_ERROR) {
        return -1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    for (int port = startPort; port <= endPort; ++port) {

        if (std::ranges::find(excludePorts, port) != excludePorts.end()) {
            continue;
        }

        serv_addr.sin_port = htons(port);
        if (bind(sockfd, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) != SOCKET_ERROR) {
            closeSocket(sockfd);

            return port;
        }
    }

    closeSocket(sockfd);

    return std::nullopt;// No available port found
}
