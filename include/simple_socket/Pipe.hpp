
#ifndef SIMPLE_SOCKET_PIPE_HPP
#define SIMPLE_SOCKET_PIPE_HPP

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

class PipeServer {
public:
    PipeServer(const std::string& name) {
        hPipe_ = CreateNamedPipe(
                name.c_str(),                                         // Pipe name
                PIPE_ACCESS_DUPLEX,                                   // Read/Write access
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,// Message type pipe
                1,                                                    // Max. instances
                512,                                                  // Output buffer size
                512,                                                  // Input buffer size
                0,                                                    // Client time-out
                nullptr);                                             // Default security attribute

        if (hPipe_ == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to create pipe. Error: " << GetLastError() << std::endl;
            exit(EXIT_FAILURE);
        }
        std::cout << "Pipe created successfully!" << std::endl;
    }

    void accept() {
        std::cout << "Waiting for client connection..." << std::endl;
        if (!ConnectNamedPipe(hPipe_, nullptr)) {
            std::cerr << "Failed to connect pipe. Error: " << GetLastError() << std::endl;
            exit(EXIT_FAILURE);
        }
        std::cout << "Client connected!" << std::endl;
    }

    void send(const std::string& message) {
        DWORD bytesWritten;
        if (!WriteFile(hPipe_, message.c_str(), message.size(), &bytesWritten, nullptr)) {
            std::cerr << "Failed to write to pipe. Error: " << GetLastError() << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    size_t receive(std::vector<unsigned char>& buffer) {

        DWORD bytesRead;
        if (!ReadFile(hPipe_, buffer.data(), buffer.size() - 1, &bytesRead, nullptr)) {
            std::cerr << "Failed to read from pipe. Error: " << GetLastError() << std::endl;
            exit(EXIT_FAILURE);
        }
        buffer[bytesRead] = '\0';
        return bytesRead;
    }

    ~PipeServer() {
        CloseHandle(hPipe_);
    }


private:
    HANDLE hPipe_;
};

class PipeClient {
public:
    PipeClient() {}

    bool connect(const std::string& name, long timeOut) {

        close();

        const auto start = std::chrono::steady_clock::now();
        while (true) {

            hPipe_ = CreateFile(
                    name.c_str(),                // Pipe name
                    GENERIC_READ | GENERIC_WRITE,// Read/Write access
                    0,                           // No sharing
                    nullptr,                     // Default security attributes
                    OPEN_EXISTING,               // Opens existing pipe
                    0,                           // Default attributes
                    nullptr);                    // No template file

            if (hPipe_ != INVALID_HANDLE_VALUE) {
                break;
            }

            if (GetLastError() != ERROR_PIPE_BUSY) {
                std::cerr << "Could not open pipe. Error: " << GetLastError() << std::endl;
                exit(EXIT_FAILURE);
            }

            // Wait for server to become available.
            if (!WaitNamedPipe(R"(\\.\pipe\PingPongPipe)", 5000)) {
                std::cerr << "Pipe is busy, retrying..." << std::endl;
            }
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

            // Check if the timeout has been exceeded
            if (elapsed >= timeOut) {
                std::cerr << "Connection attempt timed out after " << timeOut << " ms." << std::endl;
                return false;
            }

            // Optional: Sleep for a short while to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(500));// Wait 500 ms before retrying
        }

        return true;
    }

    void send(const std::string& message) {
        DWORD bytesWritten;
        if (!WriteFile(hPipe_, message.c_str(), message.size(), &bytesWritten, nullptr)) {
            std::cerr << "Failed to write to pipe. Error: " << GetLastError() << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    size_t receive(std::vector<unsigned char>& buffer) {

        DWORD bytesRead;
        if (!ReadFile(hPipe_, buffer.data(), buffer.size() - 1, &bytesRead, nullptr)) {
            std::cerr << "Failed to read from pipe. Error: " << GetLastError() << std::endl;
            exit(EXIT_FAILURE);
        }
        buffer[bytesRead] = '\0';
        return bytesRead;
    }

    void close() {
        CloseHandle(hPipe_);
    }

    ~PipeClient() {
        close();
    }

private:
    HANDLE hPipe_;
};

#endif//SIMPLE_SOCKET_PIPE_HPP
