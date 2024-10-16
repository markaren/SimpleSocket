

#ifndef SIMPLE_SOCKET_URLFETCHER_HPP
#define SIMPLE_SOCKET_URLFETCHER_HPP

#include <memory>
#include <string>

namespace simple_socket {

    class URLFetcher {
    public:
        URLFetcher();

        std::string fetch(const std::string& url);

        ~URLFetcher();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

}// namespace simple_socket

#endif//SIMPLE_SOCKET_URLFETCHER_HPP
