#include "simple_socket/SharedMemoryConnection.hpp"

#include <chrono>
#include <cstring>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

using namespace simple_socket;

struct SharedMemoryConnection::Impl {
    std::string name_;
    std::string semName_;
    size_t bufferSize_;
    bool isServer_;
    uint8_t* buffer_ = nullptr;

#ifdef _WIN32
    HANDLE hMapFile_ = nullptr;
    HANDLE writeSem_ = nullptr;
    HANDLE readSem_ = nullptr;
#else
    int fd_ = -1;
    sem_t* writeSem_ = nullptr;
    sem_t* readSem_ = nullptr;
#endif

    Impl(const std::string& name, size_t size, bool isServer)
        : name_(name), bufferSize_(size), isServer_(isServer) {

#ifdef _WIN32
        if (isServer) {
            hMapFile_ = CreateFileMapping(
                    INVALID_HANDLE_VALUE,
                    nullptr,
                    PAGE_READWRITE,
                    0,
                    static_cast<DWORD>(size + sizeof(size_t)),
                    name.c_str());
        } else {
            hMapFile_ = OpenFileMapping(
                    FILE_MAP_ALL_ACCESS,
                    FALSE,
                    name.c_str());
        }

        if (hMapFile_) {
            buffer_ = static_cast<uint8_t*>(MapViewOfFile(
                    hMapFile_,
                    FILE_MAP_ALL_ACCESS,
                    0,
                    0,
                    size + sizeof(size_t)));
        }

        semName_ = "sem_" + name;
        writeSem_ = CreateSemaphore(nullptr, 1, 1, semName_.c_str());
        readSem_ = CreateSemaphore(nullptr, 0, 1, (semName_ + "_read").c_str());
#else
        if (isServer) {
            fd_ = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
            ftruncate(fd_, size + sizeof(size_t));
        } else {
            fd_ = shm_open(name.c_str(), O_RDWR, 0666);
        }

        if (fd_ != -1) {
            buffer_ = static_cast<uint8_t*>(mmap(
                    nullptr,
                    size + sizeof(size_t),
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd_,
                    0));
        }

        semName_ = "/sem_" + name;
        if (isServer) {
            writeSem_ = sem_open(semName_.c_str(), O_CREAT, 0666, 1);
            readSem_ = sem_open((semName_ + "_read").c_str(), O_CREAT, 0666, 0);
        } else {
            writeSem_ = sem_open(semName_.c_str(), 0);
            readSem_ = sem_open((semName_ + "_read").c_str(), 0);
        }
#endif
    }

    ~Impl() {
        close();
    }

    void close() {
#ifdef _WIN32
        if (buffer_) UnmapViewOfFile(buffer_);
        if (hMapFile_) CloseHandle(hMapFile_);
        if (writeSem_) CloseHandle(writeSem_);
        if (readSem_) CloseHandle(readSem_);
#else
        if (buffer_) munmap(buffer_, bufferSize_ + sizeof(size_t));
        if (fd_ != -1) ::close(fd_);

        if (writeSem_) sem_close(writeSem_);
        if (readSem_) sem_close(readSem_);

        if (isServer_) {
            shm_unlink(name_.c_str());
            sem_unlink(semName_.c_str());
            sem_unlink((semName_ + "_read").c_str());
        }
#endif
        buffer_ = nullptr;
    }
};

SharedMemoryConnection::SharedMemoryConnection(const std::string& name, size_t size, bool isServer)
    : pimpl_(std::make_unique<Impl>(name, size, isServer)) {}

SharedMemoryConnection::~SharedMemoryConnection() = default;

int SharedMemoryConnection::read(uint8_t* buffer, size_t size) {
    if (!buffer || size == 0) return -1;

#ifdef _WIN32
    WaitForSingleObject(pimpl_->readSem_, INFINITE);
#else
    sem_wait(pimpl_->readSem_);
#endif

    std::atomic_thread_fence(std::memory_order_acquire);

    size_t dataSize;
    std::memcpy(&dataSize, pimpl_->buffer_, sizeof(size_t));
    std::atomic_thread_fence(std::memory_order_acquire);

    if (dataSize > size) {
#ifdef _WIN32
        ReleaseSemaphore(pimpl_->writeSem_, 1, nullptr);
#else
        sem_post(pimpl_->writeSem_);
#endif
        return -1;
    }

    std::memcpy(buffer, pimpl_->buffer_ + sizeof(size_t), dataSize);

    size_t zero = 0;
    std::memcpy(pimpl_->buffer_, &zero, sizeof(size_t));
    std::atomic_thread_fence(std::memory_order_release);


#ifdef _WIN32
    ReleaseSemaphore(pimpl_->writeSem_, 1, nullptr);
#else
    sem_post(pimpl_->writeSem_);
#endif


    return static_cast<int>(dataSize);
}

bool SharedMemoryConnection::readExact(uint8_t* buffer, size_t size) {
    return read(buffer, size) == static_cast<int>(size);
}

bool SharedMemoryConnection::write(const uint8_t* data, size_t size) {
    if (!data || size == 0 || size > pimpl_->bufferSize_) return false;

#ifdef _WIN32
    WaitForSingleObject(pimpl_->writeSem_, INFINITE);
#else
    sem_wait(pimpl_->writeSem_);
#endif

    std::atomic_thread_fence(std::memory_order_release);
    std::memcpy(pimpl_->buffer_, &size, sizeof(size_t));
    std::atomic_thread_fence(std::memory_order_release);

    std::memcpy(pimpl_->buffer_ + sizeof(size_t), data, size);

    std::atomic_thread_fence(std::memory_order_release);

#ifdef _WIN32
    ReleaseSemaphore(pimpl_->readSem_, 1, nullptr);
#else
    sem_post(pimpl_->readSem_);
    std::this_thread::sleep_for(std::chrono::nanoseconds(10));
#endif

    return true;
}

void SharedMemoryConnection::close() {
    if (pimpl_) {
        pimpl_->close();
    }
}
