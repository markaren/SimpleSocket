#ifndef SIMPLE_SOCKET_SHARED_MEMORY_CONNECTION_HPP
#define SIMPLE_SOCKET_SHARED_MEMORY_CONNECTION_HPP

#include "simple_socket/SimpleConnection.hpp"

#include <memory>
#include <string>

namespace simple_socket {

    class SharedMemoryConnection: public SimpleConnection {
    public:
        SharedMemoryConnection(const std::string& name, size_t size, bool isServer);
        ~SharedMemoryConnection() override;

        SharedMemoryConnection(const SharedMemoryConnection&) = delete;
        SharedMemoryConnection& operator=(const SharedMemoryConnection&) = delete;

        using SimpleConnection::read;
        using SimpleConnection::write;

        int read(uint8_t* buffer, size_t size) override;
        bool write(const uint8_t* data, size_t size) override;
        void close() override;

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif// SIMPLE_SOCKET_SHARED_MEMORY_CONNECTION_HPP
