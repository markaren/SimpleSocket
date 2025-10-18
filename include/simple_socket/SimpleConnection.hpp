
#ifndef SIMPLE_SOCKET_SIMPLE_CONNECTION_HPP
#define SIMPLE_SOCKET_SIMPLE_CONNECTION_HPP

#include <cstdint>
#include <ranges>

namespace simple_socket {

    class SimpleConnection {
    public:
        virtual int read(uint8_t* buffer, size_t size) = 0;
        virtual bool write(const uint8_t* data, size_t size) = 0;


        bool readExact(uint8_t* buffer, size_t size) {

            size_t totalBytesReceived = 0;
            while (totalBytesReceived < size) {
                const auto remainingBytes = size - totalBytesReceived;
                const auto bytesRead = read(buffer + totalBytesReceived, remainingBytes);
                if (bytesRead <= 0) {
                    return false;// Error or connection closed
                }
                totalBytesReceived += bytesRead;
            }
            return totalBytesReceived == size;
        }


        template<class Container>
            requires std::ranges::contiguous_range<Container>
        size_t read(Container& buffer) {
            return read(buffer.data(), buffer.size());
        }

        template<class Container>
            requires std::ranges::contiguous_range<Container>
        bool readExact(Container& buffer) {
            return readExact(buffer.data(), buffer.size());
        }

        template<class Container>
            requires std::ranges::contiguous_range<Container>
        bool write(const Container& data) {
            return write(data.data(), data.size());
        }

        bool write(const char* data, size_t size) {
            return write(reinterpret_cast<const uint8_t*>(data), size);
        }

        template<size_t N>
        bool write(const char (&data)[N]) {
            static_assert(N > 0, "Array size must be greater than 0");
            const size_t length = (data[N - 1] == '\0') ? N - 1 : N;
            return write(reinterpret_cast<const uint8_t*>(data), length);
        }

        virtual void close() = 0;

        virtual ~SimpleConnection() = default;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_SIMPLE_CONNECTION_HPP
