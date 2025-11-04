
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif

#include "simple_socket/util/net_byteorder.hpp"

uint16_t simple_socket::ss_htons(uint16_t hostshort) {
    return ::htons(hostshort);
}

uint16_t simple_socket::ss_ntohs(uint16_t netshort) {
    return ::ntohs(netshort);
}

uint32_t simple_socket::ss_htonl(uint32_t netlong) {
    return ::htonl(netlong);
}

uint32_t simple_socket::ss_ntohl(uint32_t netlong) {
    return ::ntohl(netlong);
}
