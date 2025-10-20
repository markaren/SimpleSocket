#include "simple_socket/SharedMemoryConnection.hpp"

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

struct BufferControl {
    size_t size;
    uint8_t data[1];// Flexible array member
};

struct SharedMemoryConnection::Impl {
    std::string name_;
    size_t bufferSize_;
    bool isServer_;
    uint8_t* shm_ = nullptr;

    BufferControl* myBuffer_ = nullptr;
    BufferControl* peerBuffer_ = nullptr;

#ifdef _WIN32
    HANDLE hMapFile_ = nullptr;
    HANDLE myWriteSem_ = nullptr;
    HANDLE myReadSem_ = nullptr;
    HANDLE peerWriteSem_ = nullptr;
    HANDLE peerReadSem_ = nullptr;
#else
    int fd_ = -1;
    sem_t* myWriteSem_ = nullptr;
    sem_t* myReadSem_ = nullptr;
    sem_t* peerWriteSem_ = nullptr;
    sem_t* peerReadSem_ = nullptr;
#endif

    Impl(const std::string& name, size_t size, bool isServer)
        : name_(name), bufferSize_(size), isServer_(isServer) {

        // Shared memory layout: [BufferA][BufferB]
        size_t totalSize = 2 * (sizeof(BufferControl) - 1 + size);

#ifdef _WIN32
        if (isServer) {
            hMapFile_ = CreateFileMapping(
                    INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                    static_cast<DWORD>(totalSize), name.c_str());
        } else {
            hMapFile_ = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
        }
        if (hMapFile_) {
            shm_ = static_cast<uint8_t*>(MapViewOfFile(
                    hMapFile_, FILE_MAP_ALL_ACCESS, 0, 0, totalSize));
        }
#else
        if (isServer) {
            fd_ = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
            ftruncate(fd_, totalSize);
        } else {
            fd_ = shm_open(name.c_str(), O_RDWR, 0666);
        }
        if (fd_ != -1) {
            shm_ = static_cast<uint8_t*>(mmap(
                    nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
        }
#endif
        // Buffer assignment
        myBuffer_ = reinterpret_cast<BufferControl*>(shm_ + (isServer ? 0 : (sizeof(BufferControl) - 1 + size)));
        peerBuffer_ = reinterpret_cast<BufferControl*>(shm_ + (isServer ? (sizeof(BufferControl) - 1 + size) : 0));

        // Semaphore names
        std::string semBase = (isServer ? "A" : "B");
        std::string peerSemBase = (isServer ? "B" : "A");

#ifdef _WIN32
        myWriteSem_ = CreateSemaphore(nullptr, 1, 1, (name + "_w" + semBase).c_str());
        myReadSem_ = CreateSemaphore(nullptr, 0, 1, (name + "_r" + semBase).c_str());
        peerWriteSem_ = CreateSemaphore(nullptr, 1, 1, (name + "_w" + peerSemBase).c_str());
        peerReadSem_ = CreateSemaphore(nullptr, 0, 1, (name + "_r" + peerSemBase).c_str());
#else
        myWriteSem_ = sem_open(("/" + name + "_w" + semBase).c_str(), O_CREAT, 0666, 1);
        myReadSem_ = sem_open(("/" + name + "_r" + semBase).c_str(), O_CREAT, 0666, 0);
        peerWriteSem_ = sem_open(("/" + name + "_w" + peerSemBase).c_str(), O_CREAT, 0666, 1);
        peerReadSem_ = sem_open(("/" + name + "_r" + peerSemBase).c_str(), O_CREAT, 0666, 0);
#endif
    }

    ~Impl() { close(); }

    void close() {
#ifdef _WIN32
        if (shm_) UnmapViewOfFile(shm_);
        if (hMapFile_) CloseHandle(hMapFile_);
        if (myWriteSem_) CloseHandle(myWriteSem_);
        if (myReadSem_) CloseHandle(myReadSem_);
        if (peerWriteSem_) CloseHandle(peerWriteSem_);
        if (peerReadSem_) CloseHandle(peerReadSem_);
#else
        if (shm_) munmap(shm_, 2 * (sizeof(BufferControl) - 1 + bufferSize_));
        if (fd_ != -1) ::close(fd_);
        if (myWriteSem_) sem_close(myWriteSem_);
        if (myReadSem_) sem_close(myReadSem_);
        if (peerWriteSem_) sem_close(peerWriteSem_);
        if (peerReadSem_) sem_close(peerReadSem_);
        if (isServer_) {
            shm_unlink(name_.c_str());
            sem_unlink(("/" + name_ + "_wA").c_str());
            sem_unlink(("/" + name_ + "_rA").c_str());
            sem_unlink(("/" + name_ + "_wB").c_str());
            sem_unlink(("/" + name_ + "_rB").c_str());
        }
#endif
        shm_ = nullptr;
        myWriteSem_ = myReadSem_ = peerWriteSem_ = peerReadSem_ = nullptr;
    }
};

SharedMemoryConnection::SharedMemoryConnection(const std::string& name, size_t size, bool isServer)
    : pimpl_(std::make_unique<Impl>(name, size, isServer)) {}

int SharedMemoryConnection::read(uint8_t* buffer, size_t size) {
    if (!buffer || size == 0) return -1;
#ifdef _WIN32
    WaitForSingleObject(pimpl_->peerReadSem_, INFINITE);
#else
    sem_wait(pimpl_->peerReadSem_);
#endif
    std::atomic_thread_fence(std::memory_order_acquire);

    size_t dataSize = pimpl_->peerBuffer_->size;
    if (dataSize > size) {
#ifdef _WIN32
        ReleaseSemaphore(pimpl_->peerWriteSem_, 1, nullptr);
#else
        sem_post(pimpl_->peerWriteSem_);
#endif
        return -1;
    }
    std::memcpy(buffer, pimpl_->peerBuffer_->data, dataSize);
    pimpl_->peerBuffer_->size = 0;
    std::atomic_thread_fence(std::memory_order_release);

#ifdef _WIN32
    ReleaseSemaphore(pimpl_->peerWriteSem_, 1, nullptr);
#else
    sem_post(pimpl_->peerWriteSem_);
#endif
    return static_cast<int>(dataSize);
}

bool SharedMemoryConnection::write(const uint8_t* data, size_t size) {
    if (!data || size == 0 || size > pimpl_->bufferSize_) return false;
#ifdef _WIN32
    WaitForSingleObject(pimpl_->myWriteSem_, INFINITE);
#else
    sem_wait(pimpl_->myWriteSem_);
#endif
    std::atomic_thread_fence(std::memory_order_release);
    pimpl_->myBuffer_->size = size;
    std::memcpy(pimpl_->myBuffer_->data, data, size);
    std::atomic_thread_fence(std::memory_order_release);

#ifdef _WIN32
    ReleaseSemaphore(pimpl_->myReadSem_, 1, nullptr);
#else
    sem_post(pimpl_->myReadSem_);
#endif
    return true;
}

void SharedMemoryConnection::close() {
    if (pimpl_) pimpl_->close();
}

SharedMemoryConnection::~SharedMemoryConnection() = default;
