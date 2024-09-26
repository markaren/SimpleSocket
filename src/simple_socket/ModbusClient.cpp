
#include "simple_socket/modbus/ModbusClient.hpp"

#include "simple_socket/TCPSocket.hpp"

#include <stdexcept>

using namespace simple_socket;

namespace {
    std::vector<uint8_t> makeRequest(uint16_t transactionID, uint8_t unitID, uint8_t functionCode, const std::vector<uint8_t>& data) {
        std::vector<uint8_t> request_;
        // MBAP Header
        request_.resize(7 + 1 + data.size());     // 7 bytes MBAP + data size
        request_[0] = (transactionID >> 8) & 0xFF;// Transaction ID High byte
        request_[1] = transactionID & 0xFF;       // Transaction ID Low byte
        request_[2] = 0x00;                       // Protocol ID (always 0 for Modbus TCP)
        request_[3] = 0x00;
        request_[4] = (data.size() + 2) >> 8;// Length field (2 bytes: unitID + functionCode + data)
        request_[5] = (data.size() + 2) & 0xFF;
        request_[6] = unitID;// Unit ID

        // Modbus PDU (Protocol Data Unit)
        request_[7] = functionCode;// Function code (e.g., 0x03 for Read Holding Registers)

        // Data part of the PDU
        std::copy(data.begin(), data.end(), request_.begin() + 8);

        return request_;
    }


    // Parse the response for reading registers
    std::vector<uint16_t> parse_registers_response(const std::vector<uint8_t>& response, uint16_t count) {
        // Check if response has the minimum size: 9 bytes for the header + data bytes
        if (response.size() < 9 + count * 2) {
            throw std::runtime_error("Invalid response size or insufficient data");
        }

        // The first 9 bytes are part of the MBAP header and function code
        const uint8_t byte_count = response[8];// The number of data bytes

        // Check if the byte count matches the expected number of bytes
        if (byte_count != count * 2) {
            throw std::runtime_error("Byte count mismatch in Modbus response");
        }

        // Extract the registers from the response
        std::vector<uint16_t> registers;
        for (size_t i = 0; i < count; ++i) {
            const uint16_t reg_value = (response[9 + i * 2] << 8) | response[9 + i * 2 + 1];// Combine high and low bytes
            registers.emplace_back(reg_value);
        }

        return registers;
    }

    // Validate the response of a write operation
    bool validate_write_response(const std::vector<uint8_t>& response, uint16_t address, uint16_t value) {
        if (response.size() < 12) {
            return false;
        }
        const uint16_t resp_address = (response[8] << 8) | response[9];
        const uint16_t resp_value = (response[10] << 8) | response[11];
        return resp_address == address && resp_value == value;
    }


}// namespace

struct ModbusClient::Impl {

    Impl(const std::string& host, uint16_t port) {
        conn = ctx.connect(host, port);
        if (!conn) {
            throw std::runtime_error("Error");
        }
    }

    std::vector<uint16_t> read_holding_registers(uint16_t address, uint16_t count, uint8_t unit_id) {
        std::vector request_data{
                static_cast<unsigned char>((address >> 8) & 0xFF),// High byte of address
                static_cast<unsigned char>(address & 0xFF),       // Low byte of address
                static_cast<unsigned char>((count >> 8) & 0xFF),  // High byte of count
                static_cast<unsigned char>(count & 0xFF)          // Low byte of count
        };
        const auto request = makeRequest(next_transaction_id_++, unit_id, 0x03, request_data);// 0x03 is function code for reading holding registers

        if (!conn->write(request)) {
            throw std::runtime_error("Failed to send Modbus request");
        }

        std::vector<uint8_t> response = receive_response(count);
        // Handle the response and extract register values
        return parse_registers_response(response, count);
    }

    std::vector<uint8_t> receive_response(uint16_t count) {
        std::vector<uint8_t> buffer(256);// Adjust buffer size based on expected response
        const auto bytes_received = conn->read(buffer);
        if (bytes_received < 0) {
            throw std::runtime_error("Failed to receive response");
        }
        buffer.resize(bytes_received);
        return buffer;
    }

    bool write_single_register(uint16_t address, uint16_t value, uint8_t unitID) {
        // Construct the data part (address + value) for the Modbus PDU
        std::vector data{
                static_cast<unsigned char>((address >> 8) & 0xFF),// High byte of the address
                static_cast<unsigned char>(address & 0xFF),       // Low byte of the address
                static_cast<unsigned char>((value >> 8) & 0xFF),  // High byte of the value
                static_cast<unsigned char>(value & 0xFF)          // Low byte of the value
        };

        // Create the Modbus request
        const auto request = makeRequest(next_transaction_id_++, unitID, 0x06, data);// 0x06 for Write Single Register

        // Send request and handle the response
        if (!conn->write(request)) {
            return false;
        }

        std::vector<uint8_t> response = receive_response(1);
        // Validate the response (should echo the request)
        return validate_write_response(response, address, value);
    }

    bool write_multiple_registers(uint16_t address, const std::vector<uint16_t>& values, uint8_t unitID) {
        // Construct the data part for the Modbus PDU: address, number of registers, and values
        std::vector data = {
                static_cast<unsigned char>((address >> 8) & 0xFF),      // High byte of the address
                static_cast<unsigned char>(address & 0xFF),             // Low byte of the address
                static_cast<unsigned char>((values.size() >> 8) & 0xFF),// High byte of number of registers
                static_cast<unsigned char>(values.size() & 0xFF),       // Low byte of number of registers
                static_cast<unsigned char>(values.size() * 2)           // Byte count (2 bytes per register)
        };

        // Append register values (each register is 2 bytes)
        for (const auto& value : values) {
            data.emplace_back(static_cast<unsigned char>((value >> 8) & 0xFF));// High byte of value
            data.emplace_back(static_cast<unsigned char>(value & 0xFF));       // Low byte of value
        }

        // Create the Modbus request
        const auto request = makeRequest(next_transaction_id_++, unitID, 0x10, data);// 0x10 for Write Multiple Registers

        // Send request and handle the response
        if (!conn->write(request)) {
            return false;
        }

        const auto response = receive_response(values.size());
        // Validate the response (should echo the address and number of registers written)
        return validate_write_response(response, address, values.size());
    }

    TCPClientContext ctx;
    std::unique_ptr<SimpleConnection> conn;

    uint16_t next_transaction_id_ = 1;
};


ModbusClient::ModbusClient(const std::string& host, uint16_t port)
    : pimpl_(std::make_unique<Impl>(host, port)) {}

std::vector<uint16_t> ModbusClient::read_holding_registers(uint16_t address, uint16_t count, uint8_t unit_id) {
    return pimpl_->read_holding_registers(address, count, unit_id);
}

bool ModbusClient::write_single_register(uint16_t address, uint16_t value, uint8_t unit_id) {
    return pimpl_->write_single_register(address, value, unit_id);
}

bool ModbusClient::write_multiple_registers(uint16_t address, const std::vector<uint16_t>& values, uint8_t unitID) {
    return pimpl_->write_multiple_registers(address, values, unitID);
}

ModbusClient::~ModbusClient() = default;
