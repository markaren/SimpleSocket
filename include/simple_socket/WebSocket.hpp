
#ifndef SIMPLE_SOCKET_WEBSOCKET_HPP
#define SIMPLE_SOCKET_WEBSOCKET_HPP

#include <functional>
#include <memory>
#include <string>

namespace simple_socket {

    class WebSocketConnection {

    public:
        WebSocketConnection();

        const std::string& uuid();

        virtual void send(const std::string& msg) = 0;

        virtual ~WebSocketConnection() = default;

    private:
        std::string uuid_;
    };

    class WebSocket {

    public:
        std::function<void(WebSocketConnection*)> onOpen;
        std::function<void(WebSocketConnection*)> onClose;
        std::function<void(WebSocketConnection*, const std::string&)> onMessage;

        explicit WebSocket(uint16_t port, const std::string& cert_file = "", const std::string& key_file = "");

        void start();

        void stop();

        ~WebSocket();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

    class WebSocketClient {

    public:
        std::function<void(WebSocketConnection*)> onOpen;
        std::function<void(WebSocketConnection*)> onClose;
        std::function<void(WebSocketConnection*, const std::string&)> onMessage;

        WebSocketClient();

        void connect(const std::string& url);

        void send(const std::string& msg);

        void close();

        ~WebSocketClient();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_WEBSOCKET_HPP
