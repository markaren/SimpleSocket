
#ifndef SIMPLE_SOCKET_SOCKET_HPP
#define SIMPLE_SOCKET_SOCKET_HPP

#include <cstddef>

namespace simple_socket {

    class SimpleConnection {
    public:
        virtual int read(unsigned char* buffer, size_t size) = 0;
        virtual bool readExact(unsigned char* buffer, size_t size) = 0;
        virtual bool write(const unsigned char* data, size_t size) = 0;

        template<class Container>
        int read(Container& buffer) {
            return read(buffer.data(), buffer.size());
        }

        template<class Container>
        bool readExact(Container& buffer) {
            return readExact(buffer.data(), buffer.size());
        }

        template<class Container>
        bool write(const Container& data) {
            return write(data.data(), data.size());
        }

        bool write(const char* data, size_t size) {
            return write(reinterpret_cast<const unsigned char*>(data), size);
        }

        template<size_t N>
        bool write(const char (&data)[N]) {
            return write(reinterpret_cast<const unsigned char*>(data), N - 1);// N - 1 to exclude null terminator if it's a C-string
        }

        virtual void close() = 0;

        virtual ~SimpleConnection() = default;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_SOCKET_HPP
