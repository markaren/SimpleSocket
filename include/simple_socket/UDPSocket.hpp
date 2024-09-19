
#ifndef SIMPLE_SOCKET_UDPSOCKET_HPP
#define SIMPLE_SOCKET_UDPSOCKET_HPP

#include <memory>
#include <string>
#include <vector>

#ifndef MAX_UDP_PACKET_SIZE
#define MAX_UDP_PACKET_SIZE 65507
#endif

namespace simple_socket {

    class UDPSocket {
    public:
        explicit UDPSocket(int localPort);

        bool sendTo(const std::string& address, uint16_t remotePort, const std::string& data);

        bool sendTo(const std::string& address, uint16_t remotePort, const std::vector<unsigned char>& data);

        int recvFrom(const std::string& address, uint16_t remotePort, std::vector<unsigned char>& buffer);

        [[nodiscard]] std::string recvFrom(const std::string& address, uint16_t remotePort);

        void close();

        ~UDPSocket();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_UDPSOCKET_HPP
