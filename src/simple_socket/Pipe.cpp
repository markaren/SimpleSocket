
#include "simple_socket/Pipe.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

using namespace simple_socket;

struct PipeConnection: SimpleConnection {

    explicit PipeConnection(HANDLE hPipe)
        : hPipe_(hPipe) {}

    int read(unsigned char* buffer, size_t size) override {
        DWORD bytesRead;
        if (!ReadFile(hPipe_, buffer, size - 1, &bytesRead, nullptr)) {
            std::cerr << "Failed to read from pipe. Error: " << GetLastError() << std::endl;
            return -1;
        }
        buffer[bytesRead] = '\0';
        return static_cast<int>(bytesRead);
    }

    bool readExact(unsigned char* buffer, size_t size) override {
        throw std::runtime_error("PipeConnection::readExact: Unsupported operation");
    }

    bool write(const unsigned char* data, size_t size) override {
        DWORD bytesWritten;
        if (!WriteFile(hPipe_, data, size, &bytesWritten, nullptr)) {
            std::cerr << "Failed to write to pipe. Error: " << GetLastError() << std::endl;
            return false;
        }
        return true;
    }

    void close() override {
        if (hPipe_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe_);
            hPipe_ = INVALID_HANDLE_VALUE;
        }
    }

    ~PipeConnection() override {

        PipeConnection::close();
    }

private:
    HANDLE hPipe_;
};


std::unique_ptr<SimpleConnection> NamedPipe::listen(const std::string& name) {
    const auto pipeName = R"(\\.\pipe\)" + name;
    HANDLE hPipe = CreateNamedPipe(
            pipeName.c_str(),                                     // Pipe name
            PIPE_ACCESS_DUPLEX,                                   // Read/Write access
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,// Message type pipe
            1,                                                    // Max. instances
            512,                                                  // Output buffer size
            512,                                                  // Input buffer size
            0,                                                    // Client time-out
            nullptr);                                             // Default security attribute

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create pipe. Error: " << GetLastError() << std::endl;
        return nullptr;
    }

    if (!ConnectNamedPipe(hPipe, nullptr)) {
        std::cerr << "Failed to connect pipe. Error: " << GetLastError() << std::endl;
        return nullptr;
    }

    return std::make_unique<PipeConnection>(hPipe);
}

std::unique_ptr<SimpleConnection> NamedPipe::connect(const std::string& name, long timeOut) {
    const auto pipeName = R"(\\.\pipe\)" + name;
    const auto start = std::chrono::steady_clock::now();
    while (true) {

        HANDLE hPipe = CreateFile(
                pipeName.c_str(),            // Pipe name
                GENERIC_READ | GENERIC_WRITE,// Read/Write access
                0,                           // No sharing
                nullptr,                     // Default security attributes
                OPEN_EXISTING,               // Opens existing pipe
                0,                           // Default attributes
                nullptr);                    // No template file

        if (hPipe != INVALID_HANDLE_VALUE) {
            return std::make_unique<PipeConnection>(hPipe);
        }

        if (GetLastError() != ERROR_PIPE_BUSY) {
            std::cerr << "Could not open pipe. Error: " << GetLastError() << std::endl;
            return nullptr;
        }

        // Wait for server to become available.
        if (!WaitNamedPipe(pipeName.c_str(), 5000)) {
            std::cerr << "Pipe is busy, retrying..." << std::endl;
        }
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

        // Check if the timeout has been exceeded
        if (elapsed >= timeOut) {
            std::cerr << "Connection attempt timed out after " << timeOut << " ms." << std::endl;
            return nullptr;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
