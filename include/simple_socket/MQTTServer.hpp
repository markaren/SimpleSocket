
#ifndef SIMPLE_SOCKET_MQTTSERVER_HPP
#define SIMPLE_SOCKET_MQTTSERVER_HPP

#include <memory>

namespace simple_socket {
    class MQTTServer {
    public:
        explicit MQTTServer(int port);

        void start();

        void stop();

        ~MQTTServer();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };
}

#endif//SIMPLE_SOCKET_MQTTSERVER_HPP
