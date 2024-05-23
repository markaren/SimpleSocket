
#ifndef SPHEROSIM_SOCKET_HPP
#define SPHEROSIM_SOCKET_HPP

#include <memory>
#include <string>
#include <vector>

// This Socket interface is not production grade

class TCPConnection {
public:
    virtual int read(std::vector<unsigned char>& buffer) = 0;

    virtual int read(unsigned char* buffer, size_t size) = 0;

    virtual bool readExact(std::vector<unsigned char>& buffer) = 0;

    virtual bool readExact(unsigned char* buffer, size_t size) = 0;

    virtual bool write(const std::string& message) = 0;

    virtual bool write(const std::vector<unsigned char>& data) = 0;

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

    void close();

    ~TCPSocket() override;

private:
    friend class TCPClient;
    friend class TCPServer;

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

#endif// SPHEROSIM_SOCKET_HPP
