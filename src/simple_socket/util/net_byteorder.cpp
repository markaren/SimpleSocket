
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif

#include "simple_socket/util/net_byteorder.hpp"

uint32_t simple_socket::ss_htons(uint16_t hostshort) {
    return ::htons(hostshort);
}

uint16_t simple_socket::ss_ntohs(uint32_t netshort) {
    return ::ntohs(static_cast<uint16_t>(netshort));
}
