
#ifndef SIMPLE_SOCKET_UDPSOCKET_HPP
#define SIMPLE_SOCKET_UDPSOCKET_HPP


#include <memory>
#include <string>
#include <vector>

class UDPSocket{
public:
    UDPSocket(int port);

    bool sendTo(const std::string& address, uint16_t port, const std::string& data);

    int recvFrom(const std::string& address, uint16_t port, std::vector<unsigned char>& buffer);

    void close();

    ~UDPSocket();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

#endif//SIMPLE_SOCKET_UDPSOCKET_HPP
