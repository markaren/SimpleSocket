
#include "UDPSocket.hpp"

#include "common.hpp"

namespace simple_socket {

    struct UDPSocket::Impl {

        explicit Impl(int localPort)
            : sockfd_(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {

            if (sockfd_ == INVALID_SOCKET) {

                throwSocketError("Failed to create socket");
            }

            sockaddr_in local{};
            local.sin_family = AF_INET;
            local.sin_addr.s_addr = INADDR_ANY;
            local.sin_port = htons(localPort);

            if (::bind(sockfd_, (sockaddr*) &local, sizeof(local)) == SOCKET_ERROR) {

                throwSocketError("Bind failed");
            }
        }

        bool sendTo(const std::string& address, uint16_t port, const std::string& data) const {
            sockaddr_in to{};
            to.sin_family = AF_INET;
            to.sin_port = htons(port);
            if (!inet_pton(AF_INET, address.c_str(), &to.sin_addr)) {

                return false;
            }

            return sendto(sockfd_, data.c_str(), data.size(), 0, reinterpret_cast<sockaddr*>(&to), sizeof(to)) != SOCKET_ERROR;
        }

        bool sendTo(const std::string& address, uint16_t port, const std::vector<unsigned char>& data) const {
            sockaddr_in to{};
            to.sin_family = AF_INET;
            to.sin_port = htons(port);
            if (!inet_pton(AF_INET, address.c_str(), &to.sin_addr)) {

                return false;
            }

            return sendto(sockfd_, reinterpret_cast<const char*>(data.data()), data.size(), 0, reinterpret_cast<sockaddr*>(&to), sizeof(to)) != SOCKET_ERROR;
        }

        int recvFrom(const std::string& address, uint16_t port, std::vector<unsigned char>& buffer) const {

            sockaddr_in from{};
            from.sin_family = AF_INET;
            from.sin_port = htons(port);
            if (!inet_pton(AF_INET, address.c_str(), &from.sin_addr)) {

                return -1;
            }
            socklen_t fromLength = sizeof(from);

            const auto receive = recvfrom(sockfd_, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<sockaddr*>(&from), &fromLength);
            if (receive == SOCKET_ERROR) {
                return -1;
            }

            return receive;
        }

        [[nodiscard]] std::string recvFrom(const std::string& address, uint16_t port) const {

            sockaddr_in from{};
            from.sin_family = AF_INET;
            from.sin_port = htons(port);
            if (!inet_pton(AF_INET, address.c_str(), &from.sin_addr)) {

                return "";
            }
            socklen_t fromLength = sizeof(from);

            thread_local std::vector<unsigned char> buffer(MAX_UDP_PACKET_SIZE);

            const auto receive = recvfrom(sockfd_, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0, reinterpret_cast<sockaddr*>(&from), &fromLength);
            if (receive == SOCKET_ERROR) {

                return "";
            }

            return {buffer.begin(), buffer.begin() + receive};
        }

        void close() const {

            closeSocket(sockfd_);
        }

        ~Impl() {

            close();
        }

    private:
#ifdef _WIN32
        WSASession session_;
#endif
        SOCKET sockfd_;
    };


    UDPSocket::UDPSocket(int localPort)
        : pimpl_(std::make_unique<Impl>(localPort)) {}

    bool UDPSocket::sendTo(const std::string& address, uint16_t remotePort, const std::string& data) {

        if (data.empty() || data.size() > MAX_UDP_PACKET_SIZE) {
            return false;
        }

        return pimpl_->sendTo(address, remotePort, data);
    }

    bool UDPSocket::sendTo(const std::string& address, uint16_t remotePort, const std::vector<unsigned char>& data) {

        if (data.empty() || data.size() > MAX_UDP_PACKET_SIZE) {
            return false;
        }

        return pimpl_->sendTo(address, remotePort, data);
    }

    int UDPSocket::recvFrom(const std::string& address, uint16_t remotePort, std::vector<unsigned char>& buffer) {

        return pimpl_->recvFrom(address, remotePort, buffer);
    }

    void UDPSocket::close() {

        pimpl_->close();
    }

    std::string UDPSocket::recvFrom(const std::string& address, uint16_t remotePort) {

        return pimpl_->recvFrom(address, remotePort);
    }

    UDPSocket::~UDPSocket() = default;

}// namespace simple_socket
