
#ifndef SIMPLE_SOCKET_HOLDINGREGISTER_HPP
#define SIMPLE_SOCKET_HOLDINGREGISTER_HPP

#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "simple_socket/modbus/modbus_helper.hpp"

namespace simple_socket {

    class HoldingRegister {
    public:
        // Constructor to initialize with a specific number of registers
        explicit HoldingRegister(size_t numRegisters)
            : registers_(numRegisters, 0) {}

        size_t size() const {
            std::lock_guard lck(m_);
            return registers_.size();
        }

        // Method to set a uint16_t value at a specific register index
        void setUint16(size_t index, uint16_t value) {
            std::lock_guard lck(m_);
            checkBounds(index, 1);// Check if index is within bounds
            registers_[index] = value;
        }

        // Method to get a uint16_t value from a specific register index
        uint16_t getUint16(size_t index) const {
            std::lock_guard lck(m_);
            checkBounds(index, 1);
            return registers_[index];
        }

        // Method to set a uint32_t value at a specific register index (requires 2 registers)
        void setUint32(size_t index, uint32_t value) {
            std::lock_guard lck(m_);
            checkBounds(index, 2);// Check if two registers are available

            registers_[index] = static_cast<uint16_t>(value >> 16);       // High 16 bits
            registers_[index + 1] = static_cast<uint16_t>(value & 0xFFFF);// Low 16 bits
        }

        // Method to get a uint32_t value from two registers
        uint32_t getUint32(size_t index) const {
            std::lock_guard lck(m_);
            checkBounds(index, 2);
            uint32_t high = static_cast<uint32_t>(registers_[index]) << 16;
            uint32_t low = registers_[index + 1];
            return high | low;
        }

        // Method to set a float value (requires 2 registers)
        void setFloat(size_t index, float value) {
            std::lock_guard lck(m_);
            checkBounds(index, 2);// Check if two registers are available
            uint32_t floatAsInt;
            std::memcpy(&floatAsInt, &value, sizeof(float));// Interpret float as uint32
            setUint32(index, floatAsInt);                   // Store as uint32
        }

        // Method to get a float value from two registers
        float getFloat(size_t index) const {
            std::lock_guard lck(m_);
            checkBounds(index, 2);
            uint32_t floatAsInt = getUint32(index);// Get value as uint32
            float result;
            std::memcpy(&result, &floatAsInt, sizeof(float));// Convert back to float
            return result;
        }

        // Method to print all register values (for debugging)
        void printRegisters() const {
            std::lock_guard lck(m_);
            for (size_t i = 0; i < registers_.size(); ++i) {
                std::cout << "Register[" << i << "] = " << registers_[i] << std::endl;
            }
        }

    private:
        mutable std::recursive_mutex m_;
        std::vector<uint16_t> registers_;

        // Check if the register access is within bounds
        void checkBounds(size_t index, size_t count) const {
            if (index + count > registers_.size()) {
                throw std::out_of_range("Register index out of bounds");
            }
        }

    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_HOLDINGREGISTER_HPP
