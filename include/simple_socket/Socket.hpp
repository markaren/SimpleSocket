
#ifndef SIMPLE_SOCKET_SOCKET_HPP
#define SIMPLE_SOCKET_SOCKET_HPP

#include <memory>
#include <string>
#include <vector>

namespace simple_socket {

    class SocketConnection {
    public:
        virtual int read(std::vector<unsigned char>& buffer) = 0;

        virtual int read(unsigned char* buffer, size_t size) = 0;

        virtual bool readExact(std::vector<unsigned char>& buffer) = 0;

        virtual bool readExact(unsigned char* buffer, size_t size) = 0;

        virtual bool write(const std::string& message) = 0;

        virtual bool write(const std::vector<unsigned char>& data) = 0;

        virtual void close() = 0;

        virtual ~SocketConnection() = default;
    };

    class Socket: public SocketConnection {
    public:
        Socket();

        int read(std::vector<unsigned char>& buffer) override;

        int read(unsigned char* buffer, size_t size) override;

        bool readExact(std::vector<unsigned char>& buffer) override;

        bool readExact(unsigned char* buffer, size_t size) override;

        bool write(const std::string& message) override;

        bool write(const std::vector<unsigned char>& data) override;

        void close() override;

        ~Socket() override;

    protected:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_SOCKET_HPP
