
#include "simple_socket/SimpleHttpFetcher.hpp"
#include "simple_socket/TCPSocket.hpp"
#include "simple_socket/util/string_utils.hpp"

#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

using namespace simple_socket;

namespace {
    std::tuple<std::string, uint16_t, bool> parseHostPort(const std::string& input) {
        std::string s = input;

        // Optional scheme (default to http)
        bool tls = false;
        if (s.rfind("https://", 0) == 0) {
            tls = true;
            s.erase(0, 8);
        } else if (s.rfind("http://", 0) == 0) {
            s.erase(0, 7);
        }

        // Strip any path/query after the authority
        const size_t slash = s.find('/');
        const std::string authority = (slash == std::string::npos) ? s : s.substr(0, slash);

        std::string host;
        uint16_t port = 0;

        if (!authority.empty() && authority.front() == '[') {
            // [ipv6]:port?
            const size_t rb = authority.find(']');
            if (rb == std::string::npos) throw std::invalid_argument("Invalid IPv6 host.");
            host = authority.substr(1, rb - 1);// strip brackets for connect()
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
            for (char c : authority)
                if (c == ':') ++colonCount;

            if (colonCount == 0) {
                host = authority;// no port
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
        if (port == 0) port = tls ? 443 : 80;// default when no explicit port

        return {host, port, tls};
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

    std::unordered_map<std::string, std::string> parseHeaders(const std::string& headerBlock) {
        std::unordered_map<std::string, std::string> h;
        size_t pos = 0;
        // skip status line
        const size_t eol = headerBlock.find("\r\n", pos);
        pos = (eol == std::string::npos) ? headerBlock.size() : (eol + 2);
        while (pos < headerBlock.size()) {
            const size_t nl = headerBlock.find("\r\n", pos);
            const std::string line = (nl == std::string::npos) ? headerBlock.substr(pos) : headerBlock.substr(pos, nl - pos);
            if (line.empty()) break;
            const size_t colon = line.find(':');
            if (colon != std::string::npos) {
                const std::string name = toLower(trim(line.substr(0, colon)));
                const std::string value = trim(line.substr(colon + 1));
                h[name] = value;
            }
            if (nl == std::string::npos) break;
            pos = nl + 2;
        }
        return h;
    }

    bool readMore(auto& connection, std::vector<uint8_t>& stash) {
        uint8_t tmp[4096];
        const int n = connection->read(tmp, sizeof(tmp));
        if (n <= 0) return false;
        stash.insert(stash.end(), tmp, tmp + n);
        return true;
    }

    bool readLineCRLF(auto& connection, std::vector<uint8_t>& stash, size_t& off, std::string& line) {
        for (;;) {
            constexpr char delimiter[] = "\r\n";
            auto it = std::search(stash.begin() + off, stash.end(), std::begin(delimiter), std::end(delimiter) - 1);
            if (it != stash.end()) {
                const size_t idx = static_cast<size_t>(it - stash.begin());
                line.assign(reinterpret_cast<const char*>(&stash[off]), idx - off);
                off = idx + 2;
                return true;
            }
            if (!readMore(connection, stash)) return false;
        }
    }

    bool ensureN(auto& connection, std::vector<uint8_t>& stash, size_t need) {
        while (stash.size() < need) {
            if (!readMore(connection, stash)) return false;
        }
        return true;
    }

    bool decodeChunked(auto& connection, std::vector<uint8_t>& stash, size_t& off, std::vector<uint8_t>& out) {
        for (;;) {
            std::string sizeLine;
            if (!readLineCRLF(connection, stash, off, sizeLine)) return false;
            // strip chunk extensions
            const size_t sc = sizeLine.find(';');
            const std::string hex = (sc == std::string::npos) ? sizeLine : sizeLine.substr(0, sc);
            size_t chunkSize = 0;
            try {
                chunkSize = std::stoul(hex, nullptr, 16);
            } catch (...) { return false; }
            if (chunkSize == 0) {
                // consume trailer headers until empty line
                std::string trailer;
                do {
                    if (!readLineCRLF(connection, stash, off, trailer)) return false;
                } while (!trailer.empty());
                return true;
            }
            // ensure chunk bytes + CRLF are present
            if (!ensureN(connection, stash, off + chunkSize + 2)) return false;
            out.insert(out.end(), stash.begin() + off, stash.begin() + off + chunkSize);
            off += chunkSize;
            // consume CRLF after chunk
            if (!(stash[off] == '\r' && stash[off + 1] == '\n')) return false;
            off += 2;
        }
    }

    std::string makeHttpRequest(std::string path, std::string hostHeader) {
        return "GET " + path + " HTTP/1.1\r\n"
                               "Host: " +
               hostHeader + "\r\n"
                            "User-Agent: SimpleSocket/1.0\r\n"
                            "Accept: */*\r\n"
                            "Accept-Encoding: identity\r\n"
                            "Connection: close\r\n\r\n";
    }

}// namespace

std::optional<std::string> SimpleHttpFetcher::fetch(const std::string& url) {

    auto bytes = fetchBytes(url);
    if (!bytes) return std::nullopt;

    // Construct from pointer + size to preserve any embedded '\0' bytes.
    return std::string(reinterpret_cast<const char*>(bytes->data()),
                       bytes->size());
}

std::optional<std::vector<uint8_t>> SimpleHttpFetcher::fetchBytes(const std::string& url) {

    auto [host, port, tls] = parseHostPort(url);
    const std::string path = extractPath(url);

    auto connection = context.connect(host, port, tls);

    std::string hostHeader = bracketIfIpv6(host);
    if (!isDefaultPort(tls, port)) hostHeader += ":" + std::to_string(port);

    const std::string request = makeHttpRequest(path, hostHeader);

    if (!connection->write(request.c_str(), request.size())) return std::nullopt;

    // Read headers
    std::string headerAccum;
    std::vector<uint8_t> stash;
    stash.reserve(8192);
    for (;;) {
        // Try to find CRLFCRLF in headerAccum
        const size_t sep = headerAccum.find("\r\n\r\n");
        if (sep != std::string::npos) {
            // Copy any body bytes already received after headers
            std::vector<uint8_t> body;
            const size_t rem = headerAccum.size() - (sep + 4);
            if (rem) body.insert(body.end(),
                                 reinterpret_cast<const uint8_t*>(headerAccum.data() + sep + 4),
                                 reinterpret_cast<const uint8_t*>(headerAccum.data() + sep + 4) + rem);
            // Parse headers
            const auto headers = parseHeaders(headerAccum.substr(0, sep));
            const auto itTE = headers.find("transfer-encoding");
            const auto itCL = headers.find("content-length");

            // If we already have more unread raw bytes in stash (from mixing string/bytes), append them
            if (!stash.empty()) body.insert(body.end(), stash.begin(), stash.end());

            // Handle chunked
            if (itTE != headers.end() && toLower(itTE->second).find("chunked") != std::string::npos) {
                // Move body into stash for unified parsing
                stash = std::move(body);
                size_t off = 0;
                std::vector<uint8_t> out;
                if (!decodeChunked(connection, stash, off, out)) return std::nullopt;
                return out;
            }

            // Handle Content-Length
            if (itCL != headers.end()) {
                size_t want = 0;
                try {
                    want = std::stoull(itCL->second);
                } catch (...) { return std::nullopt; }
                if (body.size() < want) {
                    std::vector<uint8_t> buf(4096);
                    while (body.size() < want) {
                        const size_t need = want - body.size();
                        const int n = connection->read(buf.data(), static_cast<int>(std::min(buf.size(), need)));
                        if (n <= 0) return std::nullopt;
                        body.insert(body.end(), buf.begin(), buf.begin() + n);
                    }
                } else if (body.size() > want) {
                    body.resize(want);
                }
                return body;
            }

            // Fallback: read until EOF (Connection: close)
            std::vector<uint8_t> buf(4096);
            int n;
            while ((n = connection->read(buf.data(), static_cast<int>(buf.size()))) > 0) {
                body.insert(body.end(), buf.begin(), buf.begin() + n);
            }
            return body;
        }

        // Need more header bytes
        uint8_t buf[4096];
        const int n = connection->read(buf, sizeof(buf));
        if (n <= 0) return std::nullopt;
        headerAccum.append(reinterpret_cast<const char*>(buf), n);
    }
}
