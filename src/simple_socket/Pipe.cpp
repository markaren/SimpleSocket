
#include "simple_socket/Pipe.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>


struct NamedPipe::Impl {

    HANDLE hPipe_{INVALID_HANDLE_VALUE};

    ~Impl() {
        if (hPipe_ != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe_);
            hPipe_ = INVALID_HANDLE_VALUE;
        }
    }

    static std::unique_ptr<NamedPipe> createPipe(HANDLE hPipe) {
        auto pipe = std::make_unique<NamedPipe>(PassKey());
        pipe->pimpl_->hPipe_ = hPipe;
        return pipe;
    }
};

NamedPipe::NamedPipe(PassKey)
    : pimpl_(std::make_unique<Impl>()) {}

NamedPipe::NamedPipe(NamedPipe&& other) noexcept: pimpl_(std::make_unique<Impl>()) {
    other.pimpl_->hPipe_ = INVALID_HANDLE_VALUE;
}

bool NamedPipe::send(const std::string& message) {
    DWORD bytesWritten;
    if (!WriteFile(pimpl_->hPipe_, message.c_str(), message.size(), &bytesWritten, nullptr)) {
        std::cerr << "Failed to write to pipe. Error: " << GetLastError() << std::endl;
        return false;
    }
    return true;
}

int NamedPipe::receive(std::vector<unsigned char>& buffer) {

    DWORD bytesRead;
    if (!ReadFile(pimpl_->hPipe_, buffer.data(), buffer.size() - 1, &bytesRead, nullptr)) {
        std::cerr << "Failed to read from pipe. Error: " << GetLastError() << std::endl;
        return -1;
    }
    buffer[bytesRead] = '\0';
    return static_cast<int>(bytesRead);
}

std::unique_ptr<NamedPipe> NamedPipe::listen(const std::string& name) {
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

    return Impl::createPipe(hPipe);
}

std::unique_ptr<NamedPipe> NamedPipe::connect(const std::string& name, long timeOut) {
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
            return Impl::createPipe(hPipe);
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

NamedPipe::~NamedPipe() = default;
