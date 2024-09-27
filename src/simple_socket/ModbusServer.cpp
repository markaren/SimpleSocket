
#include "simple_socket/modbus/ModbusServer.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/modbus/HoldingRegister.hpp"

#include <array>
#include <thread>

using namespace simple_socket;

namespace {

    // Send exception response
    void sendException(SimpleConnection& connection, uint8_t deviceAddress, uint8_t functionCode, uint8_t exceptionCode) {
        std::array<uint8_t, 3> response{};
        response[0] = deviceAddress;
        response[1] = functionCode | 0x80;// Set error flag
        response[2] = exceptionCode;

        connection.write(response);
    }

    void processRequest(SimpleConnection& conn, const std::vector<uint8_t>& request, HoldingRegister& reg) {
        const int8_t headerSize = 6;
        const uint8_t functionCode = request[headerSize + 1];
        const uint16_t startAddress = (request[headerSize + 2] << 8) | request[headerSize + 3];
        const uint16_t quantity = (request[headerSize + 4] << 8) | request[headerSize + 5];

        switch (functionCode) {
            case 0x03: {// Read Holding Registers
                if (startAddress + quantity > reg.size()) {
                    sendException(conn, request[headerSize + 0], functionCode, 0x02);// Illegal Data Address
                    return;
                }

                // Prepare response: device address, function code, byte count
                std::vector response(request.begin(), request.begin() + 8);
                response.push_back(quantity * 2);// Byte count (2 bytes per register)

                // Append register values to the response
                for (uint16_t i = 0; i < quantity; ++i) {
                    const uint16_t regValue = reg.getUint16(startAddress + i);
                    response.push_back(regValue >> 8);  // High byte
                    response.push_back(regValue & 0xFF);// Low byte
                }

                // Send the response
                conn.write(response);
                break;
            }

                // Add support for other function codes (e.g., 0x06, 0x10)

            default:
                sendException(conn, request[0], functionCode, 0x01);// Illegal Function
                break;
        }
    }

}// namespace

struct ModbusServer::Impl {

    Impl(uint16_t port, HoldingRegister& reg)
        : server_(port), register_(&reg) {}

    void start() {
        thread_ = std::jthread([this] {
            try {
                while (!stop_) {
                    auto conn = server_.accept();
                    clients_.emplace_back(&Impl::clientThread, this, std::move(conn));
                }
            } catch (const std::exception&) {}
        });
    }

    void stop() {
        stop_ = true;
        server_.close();
    }

    void clientThread(std::unique_ptr<SimpleConnection> conn) {

        std::array<uint8_t, 6> mbap{};
        std::vector<uint8_t> request;
        while (!stop_) {

            if (!conn->readExact(mbap)) {
                std::cerr << "Error reading request or connection closed\n";
                break;// Exit the loop if there’s an error or the connection is closed
            }

            // Extract the length from the MBAP header (bytes 4 and 5)
            // Length field specifies the number of bytes following the Unit Identifier
            const uint16_t length = (mbap[4] << 8) | mbap[5];

            // The total frame length is the 7 bytes MBAP header + the length in MBAP (following Unit Identifier)
            request.resize(length);// Resize request to fit the full frame (MBAP + Modbus request)
            conn->readExact(request);

            request.insert(request.begin(), mbap.begin(), mbap.end());
            processRequest(*conn, request, *register_);
        }
    }

private:
    TCPServer server_;
    HoldingRegister* register_;

    std::atomic_bool stop_;
    std::jthread thread_;
    std::vector<std::jthread> clients_;
};

ModbusServer::ModbusServer(HoldingRegister& reg, uint16_t port)
    : pimpl_(std::make_unique<Impl>(port, reg)) {}

void ModbusServer::start() {
    pimpl_->start();
}

void ModbusServer::stop() {
    pimpl_->stop();
}

ModbusServer::~ModbusServer() = default;
