
#include "simple_socket/modbus/HoldingRegister.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace simple_socket;

HoldingRegister::HoldingRegister(size_t numRegisters)
    : registers_(numRegisters, 0) {}

size_t HoldingRegister::size() const {
    std::lock_guard lck(m_);
    return registers_.size();
}

void HoldingRegister::setUint16(size_t index, uint16_t value) {
    std::lock_guard lck(m_);
    checkBounds(index, 1);// Check if index is within bounds
    registers_[index] = value;
}

uint16_t HoldingRegister::getUint16(size_t index) const {
    std::lock_guard lck(m_);
    checkBounds(index, 1);
    return registers_[index];
}

void HoldingRegister::setUint32(size_t index, uint32_t value) {
    std::lock_guard lck(m_);
    checkBounds(index, 2);// Check if two registers are available

    registers_[index] = static_cast<uint16_t>(value >> 16);       // High 16 bits
    registers_[index + 1] = static_cast<uint16_t>(value & 0xFFFF);// Low 16 bits
}

uint32_t HoldingRegister::getUint32(size_t index) const {
    std::lock_guard lck(m_);
    checkBounds(index, 2);
    uint32_t high = static_cast<uint32_t>(registers_[index]) << 16;
    uint32_t low = registers_[index + 1];
    return high | low;
}

void HoldingRegister::setFloat(size_t index, float value) {
    std::lock_guard lck(m_);
    checkBounds(index, 2);// Check if two registers are available
    uint32_t floatAsInt;
    std::memcpy(&floatAsInt, &value, sizeof(float));// Interpret float as uint32
    setUint32(index, floatAsInt);                   // Store as uint32
}

float HoldingRegister::getFloat(size_t index) const {
    std::lock_guard lck(m_);
    checkBounds(index, 2);
    uint32_t floatAsInt = getUint32(index);// Get value as uint32
    float result;
    std::memcpy(&result, &floatAsInt, sizeof(float));// Convert back to float
    return result;
}

void HoldingRegister::printRegisters() const {
    std::lock_guard lck(m_);
    for (size_t i = 0; i < registers_.size(); ++i) {
        std::cout << "Register[" << i << "] = " << registers_[i] << std::endl;
    }
}

void HoldingRegister::checkBounds(size_t index, size_t count) const {
    if (index + count > registers_.size()) {
        throw std::out_of_range("Register index out of bounds");
    }
}
