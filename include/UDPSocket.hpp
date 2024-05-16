
#ifndef SIMPLE_SOCKET_UDPSOCKET_HPP
#define SIMPLE_SOCKET_UDPSOCKET_HPP

#include <memory>
#include <string>
#include <vector>

#ifndef MAX_UDP_PACKET_SIZE
#define MAX_UDP_PACKET_SIZE 65507
#endif

class UDPSocket{
public:
    explicit UDPSocket(int port);

    bool sendTo(const std::string& address, uint16_t port, const std::string& data);

    int recvFrom(const std::string& address, uint16_t port, std::vector<unsigned char>& buffer);

    void close();

    ~UDPSocket();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

#endif//SIMPLE_SOCKET_UDPSOCKET_HPP
