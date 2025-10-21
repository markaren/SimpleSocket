
#ifndef SIMPLE_SOCKET_STRING_UTILS_HPP
#define SIMPLE_SOCKET_STRING_UTILS_HPP

#include <algorithm>
#include <string>

namespace simple_socket {

    inline std::string toLower(std::string s) {
        std::ranges::transform(s, s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    inline std::string ltrim(std::string s) {
        s.erase(s.begin(), std::ranges::find_if(s, [](unsigned char c) { return !std::isspace(c); }));
        return s;
    }
    inline std::string rtrim(std::string s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
        return s;
    }

    inline std::string trim(std::string s) { return rtrim(ltrim(std::move(s))); }

}// namespace simple_socket

#endif//SIMPLE_SOCKET_STRING_UTILS_HPP
