
#ifndef SIMPLE_SOCKET_WEBSOCKET_HPP
#define SIMPLE_SOCKET_WEBSOCKET_HPP

#include <memory>
#include <string>

#include "TCPSocket.hpp"

class WebSocket;

class WebSocketConnection {

public:
    explicit WebSocketConnection(WebSocket* socket, std::unique_ptr<TCPConnection> conn);

    void send(const std::string& msg);

    ~WebSocketConnection();

private:
    friend class WebSocket;
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

class WebSocket {

public:
    explicit WebSocket(uint16_t port);

    virtual void onOpen(WebSocketConnection* conn) = 0;

    virtual void onMessage(WebSocketConnection* conn, const std::string& msg) = 0;

    virtual void onClose(WebSocketConnection* conn) = 0;

    void stop();

    ~WebSocket();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

#endif//SIMPLE_SOCKET_WEBSOCKET_HPP
