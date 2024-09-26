
#ifndef SIMPLE_SOCKET_BYTEORDER_HPP
#define SIMPLE_SOCKET_BYTEORDER_HPP

namespace simple_socket {

    enum class ByteOrder {
        LITTLE,
        BIG
    };

    static ByteOrder DEFAULT_BYTE_ORDER = ByteOrder::BIG;

}// namespace simple_socket

#endif//SIMPLE_SOCKET_BYTEORDER_HPP
