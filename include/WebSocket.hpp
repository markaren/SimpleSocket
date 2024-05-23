
#ifndef SIMPLE_SOCKET_WEBSOCKET_HPP
#define SIMPLE_SOCKET_WEBSOCKET_HPP

#include <memory>
#include <string>

#include "TCPSocket.hpp"

class WebSocketConnection {

public:

    explicit WebSocketConnection(std::unique_ptr<TCPConnection> connection)
        : connection_(std::move(connection)) {}

    void write(const std::string& msg) {
        connection_->write(msg);
    }

private:
    friend class WebSocket;
    std::unique_ptr<TCPConnection> connection_;

};

class WebSocket {

public:

    explicit WebSocket(uint16_t port);

    virtual void onMessage(const std::string& msg) = 0;

    virtual void onOpen(WebSocketConnection* con) = 0;

    virtual void onClose(WebSocketConnection* con) = 0;

    void stop();

    ~WebSocket();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

#endif//SIMPLE_SOCKET_WEBSOCKET_HPP
