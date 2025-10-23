
#ifndef SIMPLE_SOCKET_MQTTCLIENT_HPP
#define SIMPLE_SOCKET_MQTTCLIENT_HPP

#include <functional>
#include <memory>
#include <string>

namespace simple_socket {

    class MQTTClient {
    public:
        MQTTClient(const std::string& host, int port, const std::string& clientId);

        void connect(bool tls = false);

        void close();

        void subscribe(const std::string& topic, const std::function<void(std::string)>& callback);

        void unsubscribe(const std::string& topic);

        void publish(const std::string& topic, const std::string& message);

        void run();

        ~MQTTClient();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_MQTTCLIENT_HPP
