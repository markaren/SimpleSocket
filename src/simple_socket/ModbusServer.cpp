
#include "simple_socket/modbus/ModbusServer.hpp"

#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/modbus/HoldingRegister.hpp"

#include <array>
#include <thread>

using namespace simple_socket;

namespace {

    // CRC calculation
    uint16_t calculateCRC(const uint8_t* data, size_t length) {
        uint16_t crc = 0xFFFF;
        for (size_t pos = 0; pos < length; pos++) {
            crc ^= static_cast<uint16_t>(data[pos]);
            for (int i = 8; i != 0; i--) {
                if ((crc & 0x0001) != 0) {
                    crc >>= 1;
                    crc ^= 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
        return crc;
    }

    bool validateCRC(const unsigned char* frame, size_t length) {
        uint16_t calculatedCRC = calculateCRC(frame, length - 2);             // Exclude last 2 bytes (CRC)
        uint16_t receivedCRC = (frame[length - 2] | (frame[length - 1] << 8));// Extract CRC from frame

        return calculatedCRC == receivedCRC;
    }

    // Send response
    void sendResponse(SimpleConnection& connection, const uint8_t* response, size_t length) {
        // Compute CRC
        uint16_t crc = calculateCRC(response, length);

        // Use a vector to hold the full response (data + CRC)
        std::vector<uint8_t> finalResponse(length + 2);

        // Copy the response data into the vector
        std::memcpy(finalResponse.data(), response, length);

        // Append the CRC (low byte first, then high byte)
        finalResponse[length] = crc & 0xFF;
        finalResponse[length + 1] = (crc >> 8) & 0xFF;

        // Send the final response with CRC
        connection.write(finalResponse.data(), finalResponse.size());
    }


    // Send exception response
    void sendException(SimpleConnection& connection, uint8_t deviceAddress, uint8_t functionCode, uint8_t exceptionCode) {
        unsigned char response[5];
        response[0] = deviceAddress;
        response[1] = functionCode | 0x80;// Set error flag
        response[2] = exceptionCode;

        sendResponse(connection, response, 5);
    }

    int determineFrameLength(const uint8_t* request) {
        uint8_t functionCode = request[1];

        // Example: Add logic for different function codes
        switch (functionCode) {
            case 0x03:                // Read Holding Registers (e.g., request + CRC is 8 bytes + data size)
                return 8;             // Fixed for a basic request; modify as per the actual length of data field
            case 0x06:                // Write Single Register
                return 8;             // Fixed length
            case 0x10:                // Write Multiple Registers
                                      // Compute based on byte count in the request (e.g., request[6])
                return 9 + request[6];// Base length + number of data bytes
            default:
                return 8;// Default to header size
        }
    }

    void processRequest(SimpleConnection& conn, const std::vector<uint8_t>& request, HoldingRegister& reg) {
        const uint8_t functionCode = request[1];
        const uint16_t startAddress = (request[2] << 8) | request[3];
        const uint16_t quantity = (request[4] << 8) | request[5];

        switch (functionCode) {
            case 0x03: {  // Read Holding Registers
                if (startAddress + quantity > reg.size()) {
                    sendException(conn, request[0], functionCode, 0x02);  // Illegal Data Address
                    return;
                }

                // Prepare response: device address, function code, byte count
                std::vector<uint8_t> response;
                response.push_back(request[0]);  // Device address
                response.push_back(functionCode);  // Function code
                response.push_back(quantity * 2);  // Byte count (2 bytes per register)

                // Append register values to the response
                for (uint16_t i = 0; i < quantity; ++i) {
                    const uint16_t regValue = reg.getUint16(startAddress + i);
                    response.push_back(regValue >> 8);  // High byte
                    response.push_back(regValue & 0xFF);  // Low byte
                }

                // Send the response
                sendResponse(conn, response.data(), response.size());
                break;
            }

            // Add support for other function codes (e.g., 0x06, 0x10)

            default:
                sendException(conn, request[0], functionCode, 0x01);  // Illegal Function
            break;
        }
    }

}// namespace

struct ModbusServer::Impl {

    Impl(uint16_t port, HoldingRegister& reg)
        : server_(port), register_(&reg) {}

    void start() {
        thread_ = std::jthread([this] {
            auto conn = server_.accept();
            clients_.emplace_back(&Impl::clientThread, this, std::move(conn));
        });
    }

    void stop() {
        stop_ = true;
        server_.close();
    }

    void clientThread(std::unique_ptr<SimpleConnection> conn) {

        std::array<uint8_t, 8> rtu{};
        std::vector<uint8_t> request;
        while (!stop_) {

            if (!conn->readExact(rtu)) {
                std::cerr << "Error reading request or connection closed\n";
                break;// Exit the loop if there’s an error or the connection is closed
            }

            // Read remaining bytes based on function code or specific frame type (length can vary)
            const int frameLength = determineFrameLength(rtu.data());// Determine based on the function code
            request.resize(frameLength);
            conn->readExact(request);

            // Validate CRC (last two bytes of the frame)
            if (!validateCRC(request.data(), request.size())) {
                std::cerr << "CRC error in Modbus request\n";
                sendException(*conn, request[0], request[1], 0x04);// Send CRC error (exception code 0x04)
                continue;
            }
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
