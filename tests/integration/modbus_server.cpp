
#include "simple_socket/modbus/ModbusServer.hpp"

#include <iostream>

using namespace simple_socket;

int main() {

    HoldingRegister holding_register(5);

    holding_register.setUint16(0, 8);
    holding_register.setFloat(1, 2.5);
    holding_register.setUint32(3, 11311111);

    ModbusServer server(holding_register, 502);
    server.start();

    std::atomic_bool stop{false};
    std::thread t([&] {
        while (!stop) {
            holding_register.setUint16(0, holding_register.getUint16(0)+1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });


    std::cout << "Press a key to continue..." << std::endl;
    std::cin.get();

    server.stop();
    stop = true;

    if (t.joinable()) t.join();
}
