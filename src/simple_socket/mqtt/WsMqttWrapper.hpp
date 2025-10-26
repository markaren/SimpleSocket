
#ifndef SIMPLE_SOCKET_WSMQTTWRAPPER_HPP
#define SIMPLE_SOCKET_WSMQTTWRAPPER_HPP


#include "simple_socket/SimpleConnection.hpp"

#include "simple_socket/ws/WebSocket.hpp"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>

namespace simple_socket {
    class WebSocketConnection;

    struct WsWrapper: SimpleConnection {

        explicit WsWrapper(WebSocketConnection* c): connection(c) {}

        int read(uint8_t* buffer, size_t size) override {
            std::unique_lock lock(m_);
            cv_.wait(lock, [&] { return closed_ || !queue_.empty(); });
            if (queue_.empty()) return -1;// closed and no data

            std::string msg = std::move(queue_.front());
            queue_.pop_front();

            size_t toCopy = std::min(size, msg.size());
            std::memcpy(buffer, msg.data(), toCopy);

            if (toCopy < msg.size()) {
                // put the remainder back to the front so next read continues it
                queue_.push_front(msg.substr(toCopy));
            }

            return static_cast<int>(toCopy);
        }

        bool write(const uint8_t* data, size_t size) override {
            if (closed_) return false;
            return connection->send(data, size);
        }

        void close() override {
            {
                std::lock_guard lock(m_);
                closed_ = true;
            }
            cv_.notify_all();
        }

        void push_back(const std::string& msg) {
            {
                std::lock_guard lock(m_);
                queue_.push_back(msg);
            }
            cv_.notify_one();
        }

        [[nodiscard]] bool closed() const {
            return closed_;
        }

    private:
        std::atomic_bool closed_{false};
        std::deque<std::string> queue_;
        std::mutex m_;
        std::condition_variable cv_;
        WebSocketConnection* connection;
    };

}// namespace simple_socket


#endif//SIMPLE_SOCKET_WSMQTTWRAPPER_HPP
