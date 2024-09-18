
#ifndef SIMPLE_SOCKET_TCPSOCKET_HPP
#define SIMPLE_SOCKET_TCPSOCKET_HPP

#include <memory>
#include <string>
#include <vector>

// This Socket interface is not production grade

namespace simple_socket {

    class TCPConnection {
    public:
        virtual int read(std::vector<unsigned char>& buffer) = 0;

        virtual int read(unsigned char* buffer, size_t size) = 0;

        virtual bool readExact(std::vector<unsigned char>& buffer) = 0;

        virtual bool readExact(unsigned char* buffer, size_t size) = 0;

        virtual bool write(const std::string& message) = 0;

        virtual bool write(const std::vector<unsigned char>& data) = 0;

        virtual void close() = 0;

        virtual ~TCPConnection() = default;
    };

    class TCPSocket: public TCPConnection {
    public:
        TCPSocket();

        int read(std::vector<unsigned char>& buffer) override;

        int read(unsigned char* buffer, size_t size) override;

        bool readExact(std::vector<unsigned char>& buffer) override;

        bool readExact(unsigned char* buffer, size_t size) override;

        bool write(const std::string& message) override;

        bool write(const std::vector<unsigned char>& data) override;

        void close() override;

        ~TCPSocket() override;

    protected:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

    class TCPClient: public TCPSocket {
    public:
        bool connect(const std::string& ip, int port);
    };

    class TCPServer: public TCPSocket {
    public:
        explicit TCPServer(int port, int backlog = 1);

        std::unique_ptr<TCPConnection> accept();
    };

    class UnixDomainClient: public TCPSocket {
    public:
        bool connect(const std::string& domain);
    };

    class UnixDomainServer: public TCPSocket {
    public:
        explicit UnixDomainServer(const std::string& domain, int backlog = 1);

        std::unique_ptr<TCPConnection> accept();
    };

}// namespace simple_socket

#endif// SIMPLE_SOCKET_TCPSOCKET_HPP
