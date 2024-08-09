
#include "port_query.hpp"

#include "common.hpp"

#include <algorithm>

int getAvailablePort(int startPort, int endPort, const std::vector<int>& excludePorts) {

#ifdef WIN32
    WSASession session;
#endif

    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == SOCKET_ERROR) {
        return -1;
    }

    const int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval));

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    for (int port = startPort; port <= endPort; ++port) {

        if (std::find(excludePorts.begin(), excludePorts.end(), port) != excludePorts.end()) {
            continue;
        }

        serv_addr.sin_port = htons(port);
        if (bind(sockfd, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) != SOCKET_ERROR) {
            closeSocket(sockfd);

            return port;
        }
    }

    closeSocket(sockfd);

    return -1;  // No available port found
}
