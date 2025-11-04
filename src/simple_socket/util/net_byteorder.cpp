
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif

#include "simple_socket/util/net_byteorder.hpp"

using namespace simple_socket;

uint32_t ss_htons(uint16_t hostshort) {
    return ::htons(hostshort);
}

uint16_t ss_ntohs(uint32_t netshort) {
    return ::ntohs(static_cast<uint16_t>(netshort));
}
