
#include "simple_socket/SimpleHttpFetcher.hpp"
#include "simple_socket/TCPSocket.hpp"

#include <stdexcept>
#include <vector>
#include <cstdint>

using namespace simple_socket;

namespace {
    std::tuple<std::string, uint16_t, bool> parseHostPort(const std::string& input) {
       std::string s = input;

        // Optional scheme (default to http)
        bool tls = false;
        if (s.rfind("https://", 0) == 0) { tls = true; s.erase(0, 8); }
        else if (s.rfind("http://", 0) == 0) { s.erase(0, 7); }

        // Strip any path/query after the authority
        const size_t slash = s.find('/');
        const std::string authority = (slash == std::string::npos) ? s : s.substr(0, slash);

        std::string host;
        uint16_t port = 0;

        if (!authority.empty() && authority.front() == '[') {
            // [ipv6]:port?
            const size_t rb = authority.find(']');
            if (rb == std::string::npos) throw std::invalid_argument("Invalid IPv6 host.");
            host = authority.substr(1, rb - 1); // strip brackets for connect()
            if (rb + 1 < authority.size() && authority[rb + 1] == ':') {
                const std::string portStr = authority.substr(rb + 2);
                try {
                    unsigned long p = std::stoul(portStr);
                    if (p > 65535) throw std::out_of_range("port");
                    port = static_cast<uint16_t>(p);
                } catch (...) {
                    throw std::invalid_argument("Invalid port number.");
                }
            }
        } else {
            // Distinguish host:port vs bare IPv6 literal (must be bracketed to carry a port)
            size_t colonCount = 0;
            for (char c : authority) if (c == ':') ++colonCount;

            if (colonCount == 0) {
                host = authority; // no port
            } else if (colonCount == 1) {
                const size_t pos = authority.rfind(':');
                host = authority.substr(0, pos);
                const std::string portStr = authority.substr(pos + 1);
                try {
                    unsigned long p = std::stoul(portStr);
                    if (p > 65535) throw std::out_of_range("port");
                    port = static_cast<uint16_t>(p);
                } catch (...) {
                    throw std::invalid_argument("Invalid port number.");
                }
            } else {
                // Unbracketed IPv6 literal with no port
                host = authority;
            }
        }

        if (host.empty()) throw std::invalid_argument("Missing host.");
        if (port == 0) port = tls ? 443 : 80; // default when no explicit port

        return { host, port, tls };
    }

    bool isDefaultPort(bool tls, uint16_t port) {
        return (!tls && port == 80) || (tls && port == 443);
    }

    bool looksLikeIpv6(const std::string& h) {
        // `parseHostPort` returns unbracketed IPv6 literals (contain ':')
        return (!h.empty() && h.front() == '[' && h.back() == ']') || (h.find(':') != std::string::npos);
    }

    std::string bracketIfIpv6(const std::string& host) {
        if (host.size() >= 2 && host.front() == '[' && host.back() == ']') return host;
        return looksLikeIpv6(host) ? ("[" + host + "]") : host;
    }

    std::string extractPath(const std::string& url) {
        const auto schemePos = url.find("://");
        const size_t start = (schemePos == std::string::npos) ? 0 : schemePos + 3;
        const auto slashPos = url.find('/', start);
        if (slashPos == std::string::npos) return "/";
        return url.substr(slashPos);
    }
    
}

std::optional<std::string> SimpleHttpFetcher::fetch(const std::string& url) {

    auto [host, port, tls] = parseHostPort(url);
    const std::string path = extractPath(url);

    auto connection = context.connect(host, port, tls);

    // Proper Host header (bracket IPv6, include port only when non-default)
    std::string hostHeader = bracketIfIpv6(host);
    if (!isDefaultPort(tls, port)) hostHeader += ":" + std::to_string(port);

    
    // Add a User-Agent to avoid 403, and send the correct path
    const std::string httpRequest =
        "GET " + path + " HTTP/1.1\r\n"
        "Host: " + hostHeader + "\r\n"
        "User-Agent: SimpleSocket/1.0\r\n"
        "Accept: */*\r\n"
        "Accept-Encoding: identity\r\n"
        "Connection: close\r\n\r\n";
    if (!connection->write(httpRequest.c_str(), httpRequest.size())) {
        return std::nullopt;
    }

    std::string response;
    constexpr size_t bufferSize = 4096;
    std::vector<uint8_t> buffer(bufferSize);

    int bytesRead;
    while ((bytesRead = connection->read(buffer.data(), bufferSize)) > 0) {
        response.append(reinterpret_cast<char*>(buffer.data()), bytesRead);
    }

    // Remove headers: split at first CRLFCRLF
    const size_t sep = response.find("\r\n\r\n");
    if (sep == std::string::npos) return std::nullopt; // malformed HTTP

    // Return only body so files like 'cmake-build-debug/tests/integration/Placeholder_view_vector.svg' are pure XML
    return response.substr(sep + 4);
}
