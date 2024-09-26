
#include "simple_socket/modbus/ModbusServer.hpp"

using namespace simple_socket;

int main() {

    HoldingRegister holding_register(5);
    holding_register.setUint16(0, 1);
    holding_register.setFloat(1, 2.5);
    holding_register.setUint32(3, 11311111);

    ModbusServer server(holding_register, 502);
    server.start();

    do {
        std::cout << '\n'
                  << "Press a key to continue...";
    } while (std::cin.get() != '\n');
}