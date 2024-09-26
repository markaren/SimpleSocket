
#ifndef SIMPLE_SOCKET_MODBUSCLIENT_HPP
#define SIMPLE_SOCKET_MODBUSCLIENT_HPP

#include <memory>
#include <string>
#include <vector>

namespace simple_socket {

    class ModbusClient {

    public:
        ModbusClient(const std::string& host, uint16_t port);

        std::vector<uint16_t> read_holding_registers(uint16_t address, uint16_t count, uint8_t unit_id = 1);

        bool write_single_register(uint16_t address, uint16_t value, uint8_t unit_id = 1);

        bool ModbusClient::write_multiple_registers(uint16_t address, const std::vector<uint16_t>& values,  uint8_t unitID = 1);

        ~ModbusClient();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_MODBUSCLIENT_HPP
