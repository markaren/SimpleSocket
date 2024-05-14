
#ifndef SIMPLE_SOCKET_UDPSOCKET_HPP
#define SIMPLE_SOCKET_UDPSOCKET_HPP


#include <memory>
#include <string>
#include <vector>

class UDPSocket{
public:
    UDPSocket();

    void sendTo(const std::string& address, uint16_t port, const std::string& data);

    std::string recvFrom();

    //    bool readFrom(std::vector<unsigned char>& buffer, size_t* bytesRead);
//
//    bool read(unsigned char* buffer, size_t size, size_t* bytesRead);
//
//    bool readExact(std::vector<unsigned char>& buffer);
//
//    bool readExact(unsigned char* buffer, size_t size);
//
//    bool write(const std::string& message);
//
//    bool write(const std::vector<unsigned char>& data);

    ~UDPSocket();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

#endif//SIMPLE_SOCKET_UDPSOCKET_HPP
