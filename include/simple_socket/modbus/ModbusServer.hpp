
#ifndef SIMPLE_SOCKET_MODBUSSERVER_HPP
#define SIMPLE_SOCKET_MODBUSSERVER_HPP

#include "HoldingRegister.hpp"


#include <memory>

namespace simple_socket {

    class ModbusServer {

    public:
        explicit ModbusServer(HoldingRegister& reg, uint16_t port);

        ModbusServer(const ModbusServer&) = delete;
        ModbusServer& operator=(const ModbusServer&) = delete;
        ModbusServer(ModbusServer&&) = delete;
        ModbusServer& operator=(ModbusServer&&) = delete;

        void start();

        void stop();

        ~ModbusServer();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_MODBUSSERVER_HPP
