
#ifndef SIMPLE_SOCKET_NET_BYTE_ORDER_HPP
#define SIMPLE_SOCKET_NET_BYTE_ORDER_HPP

#include <cstdint>

namespace simple_socket {

    uint32_t ss_htons(uint16_t hostshort);
    uint16_t ss_ntohs(uint32_t netshort);

}// namespace simple_socket

#endif//SIMPLE_SOCKET_NET_BYTE_ORDER_HPP
