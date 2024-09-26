
#include "simple_socket/ModbusClient.hpp"
#include "simple_socket/byte_conversion.hpp"

#include <iostream>

using namespace simple_socket;

int main() {

    ModbusClient client("127.0.0.1", 5020);

    const auto result = client.read_holding_registers(6, 2);

    std::cout << decode_float(result) << std::endl;

    client.write_single_register(3, 256);
}
