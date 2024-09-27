
#ifndef SIMPLE_SOCKET_MODBUSCLIENT_HPP
#define SIMPLE_SOCKET_MODBUSCLIENT_HPP

#include <memory>
#include <string>
#include <vector>

namespace simple_socket {

    class ModbusClient {

    public:
        ModbusClient(const std::string& host, uint16_t port);

        ModbusClient(const ModbusClient&) = delete;
        ModbusClient& operator=(const ModbusClient&) = delete;
        ModbusClient(ModbusClient&&) = delete;
        ModbusClient& operator=(ModbusClient&&) = delete;

        std::vector<uint16_t> read_holding_registers(uint16_t address, uint16_t count, uint8_t unit_id = 1);

        uint16_t read_uint16(uint16_t address, uint8_t unit_id = 1);

        uint32_t read_uint32(uint16_t address, uint8_t unit_id = 1);

        float read_float(uint16_t address, uint8_t unit_id = 1);

        bool write_single_register(uint16_t address, uint16_t value, uint8_t unit_id = 1);

        bool write_multiple_registers(uint16_t address, const uint16_t* values, size_t size, uint8_t unitID = 1);

        template <typename ArrayLike>
        bool write_multiple_registers(uint16_t address, const ArrayLike& values, uint8_t unitID = 1) {
            return write_multiple_registers(address, values.data(), values.size(), unitID);
        }

        ~ModbusClient();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_MODBUSCLIENT_HPP
