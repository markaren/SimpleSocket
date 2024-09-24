
#ifndef SIMPLE_SOCKET_PIPE_HPP
#define SIMPLE_SOCKET_PIPE_HPP

#include <memory>
#include <string>
#include <vector>

namespace simple_socket {

    class NamedPipe {

        struct PassKey {
            friend class NamedPipe;
        };

    public:
        explicit NamedPipe(PassKey passKey);

        NamedPipe(NamedPipe&& other) noexcept;

        NamedPipe(NamedPipe& other) = delete;
        NamedPipe& operator=(NamedPipe& other) = delete;

        bool send(const std::string& message);

        bool send(const std::vector<unsigned char>& data);

        int receive(std::vector<unsigned char>& buffer);

        static std::unique_ptr<NamedPipe> listen(const std::string& name);

        static std::unique_ptr<NamedPipe> connect(const std::string& name, long timeOut = 5000);

        ~NamedPipe();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_PIPE_HPP
