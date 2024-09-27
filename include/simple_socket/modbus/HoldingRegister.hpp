
#ifndef SIMPLE_SOCKET_HOLDINGREGISTER_HPP
#define SIMPLE_SOCKET_HOLDINGREGISTER_HPP

#include <cstdint>
#include <mutex>
#include <vector>

namespace simple_socket {

    class HoldingRegister {
    public:
        // Constructor to initialize with a specific number of registers
        explicit HoldingRegister(size_t numRegisters);

        size_t size() const;

        // Method to set a uint16_t value at a specific register index
        void setUint16(size_t index, uint16_t value);

        // Method to get a uint16_t value from a specific register index
        uint16_t getUint16(size_t index) const;

        // Method to set a uint32_t value at a specific register index (requires 2 registers)
        void setUint32(size_t index, uint32_t value);

        // Method to get a uint32_t value from two registers
        uint32_t getUint32(size_t index) const;

        // Method to set a float value (requires 2 registers)
        void setFloat(size_t index, float value);

        // Method to get a float value from two registers
        float getFloat(size_t index) const;

        // Method to print all register values (for debugging)
        void printRegisters() const;

    private:
        mutable std::mutex m_;
        std::vector<uint16_t> registers_;

        // Check if the register access is within bounds
        void checkBounds(size_t index, size_t count) const;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_HOLDINGREGISTER_HPP
