
#ifndef SIMPLE_SOCKET_SOCKET_HPP
#define SIMPLE_SOCKET_SOCKET_HPP

#include <string>
#include <vector>

namespace simple_socket {

    class SimpleConnection {
    public:
        virtual int read(std::vector<unsigned char>& buffer) = 0;

        virtual int read(unsigned char* buffer, size_t size) = 0;

        virtual bool readExact(std::vector<unsigned char>& buffer) = 0;

        virtual bool readExact(unsigned char* buffer, size_t size) = 0;

        virtual bool write(const std::string& message) = 0;

        virtual bool write(const std::vector<unsigned char>& data) = 0;

        virtual void close() = 0;

        virtual ~SimpleConnection() = default;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_SOCKET_HPP
