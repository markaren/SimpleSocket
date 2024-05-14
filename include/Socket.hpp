
#ifndef SPHEROSIM_SOCKET_HPP
#define SPHEROSIM_SOCKET_HPP

#include <memory>
#include <string>
#include <vector>

// This Socket interface is not production grade

class Connection {
public:
    virtual bool read(std::vector<unsigned char>& buffer, size_t* bytesRead = nullptr) = 0;

    virtual bool read(unsigned char* buffer, size_t size, size_t* bytesRead = nullptr) = 0;

    virtual bool readExact(std::vector<unsigned char>& buffer) = 0;

    virtual bool readExact(unsigned char* buffer, size_t size) = 0;

    virtual bool write(const std::string& message) = 0;

    virtual bool write(const std::vector<unsigned char>& data) = 0;

    virtual ~Connection() = default;
};

class Socket: public Connection {
public:
    Socket();

    bool read(std::vector<unsigned char>& buffer, size_t* bytesRead) override;

    bool read(unsigned char* buffer, size_t size, size_t* bytesRead) override;

    bool readExact(std::vector<unsigned char>& buffer) override;

    bool readExact(unsigned char* buffer, size_t size) override;

    bool write(const std::string& message) override;

    bool write(const std::vector<unsigned char>& data) override;

    void close();

    ~Socket() override;

protected:
    void bind(int port);

    void listen(int backlog = 1);

    std::unique_ptr<Connection> accept();

    bool connect(const std::string& ip, int port);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

class ClientSocket: public Socket {
public:
    bool connect(const std::string& ip, int port);
};

class ServerSocket: public Socket {
public:
    void bind(int port);

    void listen(int backlog = 1);

    std::unique_ptr<Connection> accept();
};

#endif// SPHEROSIM_SOCKET_HPP
