
#ifndef SIMPLE_SOCKET_WEBSOCKET_HPP
#define SIMPLE_SOCKET_WEBSOCKET_HPP

#include <memory>
#include <string>
#include <functional>

class WebSocketConnection {

public:
    virtual void send(const std::string& msg) = 0;

    virtual ~WebSocketConnection() = default;
};

class WebSocket {

public:
    std::function<void(WebSocketConnection*)> onOpen;
    std::function<void(WebSocketConnection*)> onClose;
    std::function<void(WebSocketConnection*, const std::string&)> onMessage;

    explicit WebSocket(uint16_t port);

    void stop() const;

    ~WebSocket();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

#endif//SIMPLE_SOCKET_WEBSOCKET_HPP
