
#include "simple_socket/URLFetcher.hpp"

#include "simple_socket/TCPSocket.hpp"

#include <sstream>

using namespace simple_socket;

namespace {
    void parseURL(const std::string& url, std::string& host, std::string& path) {
        // Very basic URL parsing. This can be extended to support more complex URLs.
        size_t protocolPos = url.find("://");
        size_t hostStart = (protocolPos == std::string::npos) ? 0 : protocolPos + 3;
        size_t pathStart = url.find('/', hostStart);

        host = url.substr(hostStart, pathStart - hostStart);
        path = (pathStart == std::string::npos) ? "/" : url.substr(pathStart);
    }

    void sendHttpRequest(SimpleConnection& conn, const std::string& host, const std::string& path) {
        std::string request = "GET " + path + " HTTP/1.1\r\n"
                                              "Host: " +
                              host + "\r\n"
                                     "Connection: close\r\n\r\n";
        conn.write(reinterpret_cast<const uint8_t*>(request.c_str()), request.size());
    }

    void processHeaders(const std::string& headers) {
        // Here you can process HTTP headers (e.g., check for status code, content-length, etc.)
        // For now, we’ll assume everything is fine and proceed with downloading.
        // But in production, you should parse and handle errors, redirects, etc.
    }

    void receiveHttpResponse(SimpleConnection& conn, std::stringstream& outputFile) {
        const size_t bufferSize = 4096;
        uint8_t buffer[bufferSize];
        std::string headers;
        bool headersParsed = false;
        size_t contentStart = 0;

        while (true) {
            int bytesRead = conn.read(buffer, bufferSize);
            if (bytesRead <= 0) {
                break;// Connection closed
            }

            if (!headersParsed) {
                headers.append(reinterpret_cast<const char*>(buffer), bytesRead);
                size_t headerEnd = headers.find("\r\n\r\n");

                if (headerEnd != std::string::npos) {
                    // Headers end at "\r\n\r\n". Anything after that is the body.
                    headersParsed = true;
                    contentStart = headerEnd + 4;
                    processHeaders(headers.substr(0, headerEnd));
                    outputFile.write(reinterpret_cast<const char*>(buffer + contentStart), bytesRead - contentStart);
                }
            } else {
                outputFile.write(reinterpret_cast<const char*>(buffer), bytesRead);
            }
        }
    }

}// namespace

struct URLFetcher::Impl {

    std::string fetch(const std::string& url) {
        std::string host, path;
        parseURL(url, host, path);

        std::unique_ptr<SimpleConnection> conn = ctx_.connect(host, 80);

        // Send the HTTP GET request
        sendHttpRequest(*conn, host, path);

        // Receive and process the response
        std::stringstream ss;

        receiveHttpResponse(*conn, ss);

        return ss.str();
    }

private:
    TCPClientContext ctx_;
};

std::string URLFetcher::fetch(const std::string& url) {

    return pimpl_->fetch(url);
}
URLFetcher::URLFetcher()
    : pimpl_(std::make_unique<Impl>()) {}

URLFetcher::~URLFetcher() = default;
