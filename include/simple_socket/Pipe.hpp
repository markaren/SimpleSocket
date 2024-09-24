
#ifndef SIMPLE_SOCKET_PIPE_HPP
#define SIMPLE_SOCKET_PIPE_HPP

#include "SimpleConnection.hpp"

#include <memory>
#include <string>

namespace simple_socket {

    class NamedPipe: SimpleConnection {

        struct PassKey {
            friend class NamedPipe;
        };

    public:
        explicit NamedPipe(PassKey passKey);

        NamedPipe(NamedPipe&& other) noexcept;

        NamedPipe(NamedPipe& other) = delete;
        NamedPipe& operator=(NamedPipe& other) = delete;

        int read(std::vector<unsigned char>& buffer) override;
        int read(unsigned char* buffer, size_t size) override;
        bool readExact(std::vector<unsigned char>& buffer) override;
        bool readExact(unsigned char* buffer, size_t size) override;
        bool write(const std::string& message) override;
        bool write(const std::vector<unsigned char>& data) override;
        void close() override;

        static std::unique_ptr<NamedPipe> listen(const std::string& name);

        static std::unique_ptr<NamedPipe> connect(const std::string& name, long timeOut = 5000);

        ~NamedPipe() override;

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_PIPE_HPP
