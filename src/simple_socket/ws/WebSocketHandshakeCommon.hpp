
#ifndef SIMPLE_SOCKET_WEBSOCKETHANDSHAKECOMMON_HPP
#define SIMPLE_SOCKET_WEBSOCKETHANDSHAKECOMMON_HPP

#include "simple_socket/SimpleConnection.hpp"
#include "simple_socket/util/string_utils.hpp"
#include "simple_socket/socket_common.hpp"

#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>

namespace simple_socket {

    struct HttpHeaders {
        std::string startLine;
        std::unordered_map<std::string, std::string> headers; // lower-cased names, trimmed values

        const std::string* get(const std::string& name) const {
            const auto it = headers.find(toLower(name));
            return it != headers.end() ? &it->second : nullptr;
        }
    };

    // Read raw HTTP header block (including trailing "\r\n\r\n").
    // Throws on read failure or when headers exceed maxBytes.
    inline std::string readHttpHeaderBlock(SimpleConnection& conn, size_t maxBytes = 16 * 1024) {
        std::string raw;
        std::vector<unsigned char> buffer(1024);
        constexpr size_t defaultMax = 16 * 1024;
        if (maxBytes == 0) maxBytes = defaultMax;

        while (raw.find("\r\n\r\n") == std::string::npos) {
            const int n = conn.read(buffer);
            if (n <= 0) throwSocketError("Failed to read HTTP headers.");
            raw.append(reinterpret_cast<const char*>(buffer.data()), static_cast<size_t>(n));
            if (raw.size() > maxBytes) throwSocketError("HTTP headers too large.");
        }
        return raw;
    }

    // Parse a raw HTTP header block into start line + headers (lower-cased names, trimmed values).
    // Caller may pass the entire HTTP payload; parsing stops at the first "\r\n\r\n".
    inline HttpHeaders parseHttpHeaders(const std::string& raw) {
        HttpHeaders out;
        const auto hdrEnd = raw.find("\r\n\r\n");
        const auto headerBlock = (hdrEnd == std::string::npos) ? raw : raw.substr(0, hdrEnd);
        std::istringstream iss(headerBlock);
        std::string line;

        if (!std::getline(iss, line)) throwSocketError("Bad HTTP header block.");
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out.startLine = line;

        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            const auto pos = line.find(':');
            if (pos == std::string::npos) continue;
            const std::string name = toLower(trim(line.substr(0, pos)));
            const std::string value = trim(line.substr(pos + 1));
            out.headers[name] = value;
        }
        return out;
    }

    inline void validateCommonHandshakeRequest(const HttpHeaders& http) {

        const std::string* hUpgrade = http.get("upgrade");
        if (!hUpgrade || *hUpgrade != "websocket")
            throwSocketError("Handshake failed: missing/invalid Upgrade: websocket.");

        const std::string* hConn = http.get("connection");
        if (!hConn || toLower(*hConn).find("upgrade") == std::string::npos)
            throwSocketError("Handshake failed: missing/invalid Connection: Upgrade.");
    }

    // Validate server-side request headers and return the trimmed Sec-WebSocket-Key.
    inline std::string validateServerHandshakeRequest(const HttpHeaders& http) {
        validateCommonHandshakeRequest(http);

        const std::string* hVersion = http.get("sec-websocket-version");
        if (!hVersion || trim(*hVersion) != "13")
            throwSocketError("Handshake failed: unsupported Sec-WebSocket-Version.");

        const std::string* hKey = http.get("sec-websocket-key");
        if (!hKey || trim(*hKey).empty())
            throwSocketError("Handshake failed: missing Sec-WebSocket-Key.");

        return trim(*hKey);
    }

    // Validate client-side response headers against expected Accept value.
    inline void validateClientHandshakeResponse(const HttpHeaders& http, const std::string& expectedAccept) {
        // check HTTP status 101
        const auto& start = http.startLine;
        const bool is101 = (start.find(" 101 ") != std::string::npos) ||
                           (start.size() >= 3 && start.rfind(" 101", start.size() - 3) != std::string::npos);
        if (!is101) throwSocketError("Handshake failed: expected HTTP 101");

       validateCommonHandshakeRequest(http);

        auto itAcc = http.get("sec-websocket-accept");
        if (!itAcc) throwSocketError("Handshake failed: missing Sec-WebSocket-Accept");

        if (trim(*itAcc) != expectedAccept)
            throwSocketError("Handshake failed: Sec-WebSocket-Accept mismatch");
    }

} // namespace simple_socket

#endif//SIMPLE_SOCKET_WEBSOCKETHANDSHAKECOMMON_HPP
