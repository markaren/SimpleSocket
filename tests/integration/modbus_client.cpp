
#include "simple_socket/modbus/ModbusClient.hpp"
#include "simple_socket/modbus/modbus_helper.hpp"

#include <iostream>

using namespace simple_socket;

int main() {

    ModbusClient client("127.0.0.1", 5020);

    {
        const auto result = client.read_holding_registers(3, 1);
        std::cout << result.front() << std::endl;
    }

    {
        const auto result = client.read_holding_registers(6, 2);
        std::cout << decode_float(result) << std::endl;
    }

    {
        client.write_single_register(3, 1);
    }

    {
        const auto value = encode_uint32(2);
        client.write_multiple_registers(4, value);
    }

    {
        const auto floatValue = encode_float(2.1);
        client.write_multiple_registers(6, floatValue);
    }

    {
        const auto text = client.read_holding_registers(16, 4);
        std::cout << decode_text(text) << std::endl;
    }

    {
        const auto text = client.write_multiple_registers(16, encode_text("1234567890"));
    }
}
