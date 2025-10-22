
#ifndef SIMPLE_SOCKET_MQTTCLIENT_HPP
#define SIMPLE_SOCKET_MQTTCLIENT_HPP

#include "SimpleConnection.hpp"
#include "TCPSocket.hpp"

#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace simple_socket {

    class MQTTClient {
    public:
        MQTTClient(std::string host, int port, std::string clientId);

        void connect(bool tls = false);

        void close();

        void subscribe(const std::string& topic, const std::function<void(std::string)>& callback);

        void unsubscribe(const std::string& topic);

        void publish(const std::string& topic, const std::string& message);

        void run();

        ~MQTTClient();

    private:
        TCPClientContext ctx_;
        std::unique_ptr<SimpleConnection> conn_;
        std::string clientId_;
        std::thread thread_;
        std::atomic_bool stop_{false};

        std::unordered_map<std::string, std::function<void(std::string)>> callbacks_;

        std::string host_;
        int port_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_MQTTCLIENT_HPP
