
#ifndef SIMPLE_SOCKET_MQTTCLIENT_HPP
#define SIMPLE_SOCKET_MQTTCLIENT_HPP

#include "SimpleConnection.hpp"
#include "TCPSocket.hpp"

#include <memory>
#include <string>

namespace simple_socket {

    class MQTTClient {
    public:
        MQTTClient(const std::string& host, int port, std::string  clientId);

        void connect();

        void subscribe(const std::string& topic);

        void publish(const std::string& topic, const std::string& message);

        void loop();

    private:
        TCPClientContext ctx_;
        std::unique_ptr<SimpleConnection> conn_;
        std::string clientId_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_MQTTCLIENT_HPP
