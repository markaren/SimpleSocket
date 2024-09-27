
#include "simple_socket/modbus/HoldingRegister.hpp"

#include "simple_socket/modbus/modbus_helper.hpp"

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
    checkBounds(index, 1);
    registers_[index] = value;
}

uint16_t HoldingRegister::getUint16(size_t index) const {
    std::lock_guard lck(m_);
    checkBounds(index, 1);
    return registers_[index];
}

void HoldingRegister::setUint32(size_t index, uint32_t value) {
    std::lock_guard lck(m_);
    checkBounds(index, 2);
    encode_uint32(value, registers_, index);
}

uint32_t HoldingRegister::getUint32(size_t index) const {
    std::lock_guard lck(m_);
    checkBounds(index, 2);
    return decode_uint32(registers_, index);
}

void HoldingRegister::setUint64(size_t index, uint64_t value) {
    std::lock_guard lck(m_);
    checkBounds(index, 4);
    encode_uint64(value, registers_, index);
}

uint64_t HoldingRegister::getUint64(size_t index) const {
    std::lock_guard lck(m_);
    checkBounds(index, 4);
    return decode_uint64(registers_, index);
}

void HoldingRegister::setFloat(size_t index, float value) {
    std::lock_guard lck(m_);
    checkBounds(index, 2);
    encode_float(value, registers_, index);
}

float HoldingRegister::getFloat(size_t index) const {
    std::lock_guard lck(m_);
    checkBounds(index, 2);
    return decode_float(registers_, index);
}

void HoldingRegister::setDouble(size_t index, double value) {
    std::lock_guard lck(m_);
    checkBounds(index, 4);
    encode_double(value, registers_, index);
}

double HoldingRegister::getDouble(size_t index) const {
    std::lock_guard lck(m_);
    checkBounds(index, 4);
    return decode_double(registers_, index);
}

void HoldingRegister::checkBounds(size_t index, size_t count) const {
    if (index + count > registers_.size()) {
        throw std::out_of_range("Register index out of bounds");
    }
}
