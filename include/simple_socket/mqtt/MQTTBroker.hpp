
#ifndef SIMPLE_SOCKET_MQTTSERVER_HPP
#define SIMPLE_SOCKET_MQTTSERVER_HPP

#include <memory>

namespace simple_socket {

    class MQTTBroker {
    public:
        explicit MQTTBroker(int port);

        void start();

        void stop();

        ~MQTTBroker();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };
}// namespace simple_socket

#endif//SIMPLE_SOCKET_MQTTSERVER_HPP
