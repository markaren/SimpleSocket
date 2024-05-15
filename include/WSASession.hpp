
#ifndef SIMPLE_SOCKET_WSASESSION_HPP
#define SIMPLE_SOCKET_WSASESSION_HPP

#include <memory>

class WSASession {
public:
    WSASession();
    ~WSASession();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

#endif//SIMPLE_SOCKET_WSASESSION_HPP
