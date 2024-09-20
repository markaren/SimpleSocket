
#ifndef SIMPLE_SOCKET_PIPE_HPP
#define SIMPLE_SOCKET_PIPE_HPP

#include <memory>
#include <string>
#include <vector>

class NamedPipe {
public:
    NamedPipe();

    NamedPipe(NamedPipe&& other) noexcept;

    NamedPipe(NamedPipe& other) = delete;
    NamedPipe& operator=(NamedPipe& other) = delete;

    bool send(const std::string& message);

    size_t receive(std::vector<unsigned char>& buffer);

    static std::unique_ptr<NamedPipe> listen(const std::string& name);

    static std::unique_ptr<NamedPipe> connect(const std::string& name, long timeOut = 5000);

    ~NamedPipe();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

#endif//SIMPLE_SOCKET_PIPE_HPP
