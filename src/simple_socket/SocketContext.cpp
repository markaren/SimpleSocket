
#include "simple_socket/SocketContext.hpp"

#include "WSASession.hpp"

using namespace simple_socket;

struct SocketContext::Impl {

#ifdef _WIN32
    WSASession session{};
#endif
};

SocketContext::SocketContext()
    : pimpl_(std::make_unique<Impl>()) {}

SocketContext::~SocketContext() = default;
