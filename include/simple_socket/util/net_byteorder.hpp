
#ifndef SIMPLE_SOCKET_NET_BYTE_ORDER_HPP
#define SIMPLE_SOCKET_NET_BYTE_ORDER_HPP

#include <cstdint>

namespace simple_socket {

    uint16_t ss_htons(uint16_t hostshort);
    uint16_t ss_ntohs(uint16_t netshort);

    uint32_t ss_htonl(uint32_t netlong);
    uint32_t ss_ntohl(uint32_t netlong);


}// namespace simple_socket

#endif//SIMPLE_SOCKET_NET_BYTE_ORDER_HPP
