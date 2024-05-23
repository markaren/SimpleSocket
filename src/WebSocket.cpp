
#include "WebSocket.hpp"
#include "TCPSocket.hpp"
#include "SHA1.hpp"
#include "SocketIncludes.hpp"

#include <atomic>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace {


    const std::string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";


    class SHA1 {
    public:
        SHA1();
        void update(const std::string &s);
        std::string final();
        std::vector<uint8_t> finalRaw();

    private:
        static const size_t BlockSize = 64;
        static const size_t HashSize = 20;

        static const uint32_t H0 = 0x67452301;
        static const uint32_t H1 = 0xEFCDAB89;
        static const uint32_t H2 = 0x98BADCFE;
        static const uint32_t H3 = 0x10325476;
        static const uint32_t H4 = 0xC3D2E1F0;

        uint32_t h0, h1, h2, h3, h4;
        std::vector<uint8_t> buffer;
        uint64_t bitCount;

        void processBlock(const uint8_t *block);
        void processBuffer();
        static uint64_t swapByteOrder(uint64_t value);
    };

    SHA1::SHA1()
        : h0(H0), h1(H1), h2(H2), h3(H3), h4(H4), bitCount(0) {
    }

    void SHA1::update(const std::string &s) {
        buffer.insert(buffer.end(), s.begin(), s.end());
        bitCount += s.size() * 8;

        while (buffer.size() >= BlockSize) {
            processBlock(buffer.data());
            buffer.erase(buffer.begin(), buffer.begin() + BlockSize);
        }
    }

    std::string SHA1::final() {
        buffer.push_back(0x80);

        while (buffer.size() < 56) {
            buffer.push_back(0x00);
        }

        uint64_t bitCountBigEndian = swapByteOrder(bitCount);
        for (int i = 56; i < 64; ++i) {
            buffer.push_back((bitCountBigEndian >> (56 - 8 * (i - 56))) & 0xFF);
        }

        processBlock(buffer.data());

        std::ostringstream result;
        result << std::hex << std::setfill('0');
        result << std::setw(8) << h0;
        result << std::setw(8) << h1;
        result << std::setw(8) << h2;
        result << std::setw(8) << h3;
        result << std::setw(8) << h4;

        return result.str();
    }

    std::vector<uint8_t> SHA1::finalRaw() {
        buffer.push_back(0x80);

        while (buffer.size() < 56) {
            buffer.push_back(0x00);
        }

        uint64_t bitCountBigEndian = swapByteOrder(bitCount);
        for (int i = 56; i < 64; ++i) {
            buffer.push_back((bitCountBigEndian >> (56 - 8 * (i - 56))) & 0xFF);
        }

        processBlock(buffer.data());

        std::vector<uint8_t> hash(HashSize);
        for (int i = 0; i < 4; ++i) {
            hash[i] = (h0 >> (24 - i * 8)) & 0xFF;
            hash[i + 4] = (h1 >> (24 - i * 8)) & 0xFF;
            hash[i + 8] = (h2 >> (24 - i * 8)) & 0xFF;
            hash[i + 12] = (h3 >> (24 - i * 8)) & 0xFF;
            hash[i + 16] = (h4 >> (24 - i * 8)) & 0xFF;
        }

        return hash;
    }

    uint64_t SHA1::swapByteOrder(uint64_t value) {
        return ((value >> 56) & 0x00000000000000FF) |
               ((value >> 40) & 0x000000000000FF00) |
               ((value >> 24) & 0x0000000000FF0000) |
               ((value >> 8)  & 0x00000000FF000000) |
               ((value << 8)  & 0x000000FF00000000) |
               ((value << 24) & 0x0000FF0000000000) |
               ((value << 40) & 0x00FF000000000000) |
               ((value << 56) & 0xFF00000000000000);
    }

    void SHA1::processBlock(const uint8_t *block) {
        uint32_t w[80];

        for (int i = 0; i < 16; ++i) {
            w[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) |
                   (block[i * 4 + 2] << 8) | (block[i * 4 + 3]);
        }

        for (int i = 16; i < 80; ++i) {
            w[i] = (w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]);
            w[i] = (w[i] << 1) | (w[i] >> 31);
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;

        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    void SHA1::processBuffer() {
        uint8_t block[BlockSize] = {0};
        size_t i = 0;
        for (; i < buffer.size(); ++i) {
            block[i] = buffer[i];
        }
        block[i] = 0x80;
        if (buffer.size() > 55) {
            processBlock(block);
            memset(block, 0, BlockSize);
        }
        uint64_t bitCountBigEndian = swapByteOrder(bitCount);
        memcpy(block + 56, &bitCountBigEndian, 8);
        processBlock(block);
    }

    std::string sha1(const std::string &input) {
        SHA1 sha1;
        sha1.update(input);
        return sha1.final();
    }

    std::vector<uint8_t> sha1Raw(const std::string &input) {
        SHA1 sha1;
        sha1.update(input);
        return sha1.finalRaw();
    }


     std::string base64Encode(const std::string &input) {

         std::string encoded;
         int i = 0;
         int j = 0;
         unsigned char char_array_3[3];
         unsigned char char_array_4[4];
         int in_len = input.size();
         int pos = 0;

         while (in_len--) {
             char_array_3[i++] = input[pos++];
             if (i == 3) {
                 char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                 char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                 char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                 char_array_4[3] = char_array_3[2] & 0x3f;

                 for (i = 0; (i < 4); i++)
                     encoded += base64_chars[char_array_4[i]];
                 i = 0;
             }
         }

         if (i) {
             for (j = i; j < 3; j++)
                 char_array_3[j] = '\0';

             char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
             char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
             char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
             char_array_4[3] = char_array_3[2] & 0x3f;

             for (j = 0; (j < i + 1); j++)
                 encoded += base64_chars[char_array_4[j]];

             while ((i++ < 3))
                 encoded += '=';
         }

         return encoded;
     }

     std::string base64_decode(const std::string &encoded_string) {

         auto is_base64 = [](unsigned char c) {
             return (isalnum(c) || (c == '+') || (c == '/'));
         };

         int in_len = encoded_string.size();
         int i = 0;
         int j = 0;
         int in_ = 0;
         unsigned char char_array_4[4], char_array_3[3];
         std::string ret;

         while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
             char_array_4[i++] = encoded_string[in_]; in_++;
             if (i ==4) {
                 for (i = 0; i <4; i++)
                     char_array_4[i] = base64_chars.find(char_array_4[i]);

                 char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                 char_array_3[1] = ((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                 char_array_3[2] = ((char_array_4[2] & 0x03) << 6) + char_array_4[3];

                 for (i = 0; (i < 3); i++)
                     ret += char_array_3[i];
                 i = 0;
             }
         }

         if (i) {
             for (j = i; j <4; j++)
                 char_array_4[j] = 0;

             for (j = 0; j <4; j++)
                 char_array_4[j] = base64_chars.find(char_array_4[j]);

             char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
             char_array_3[1] = ((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2);

             for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
         }

         return ret;
     }

    std::string websocketAcceptKey(const std::string &key) {
        static const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        std::string concatenated = key + GUID;
        sha1::SHA1 s;
        s.processBytes(concatenated.c_str(), concatenated.size());
        uint32_t digest[5];
        s.getDigest(digest);
        char tmp[48];
        snprintf(tmp, 45, "%08x %08x %08x %08x %08x", digest[0], digest[1], digest[2], digest[3], digest[4]);
        std::string hash = tmp;
        // remove spaces from hash
        hash.erase(std::remove(hash.begin(), hash.end(), ' '), hash.end());
        auto base64 = base64Encode(hash);
        return base64;
    }

    std::vector<uint8_t> createFrame(const std::string& message) {
        std::vector<uint8_t> frame;
        frame.push_back(0x81); // FIN, text frame

        if (message.size() <= 125) {
            frame.push_back(static_cast<uint8_t>(message.size()));
        } else if (message.size() <= 65535) {
            frame.push_back(126);
            frame.push_back((message.size() >> 8) & 0xFF);
            frame.push_back(message.size() & 0xFF);
        } else {
            frame.push_back(127);
            for (int i = 7; i >= 0; --i) {
                frame.push_back((message.size() >> (i * 8)) & 0xFF);
            }
        }

        frame.insert(frame.end(), message.begin(), message.end());
        return frame;
    }
}

struct WebSocketConnection::Impl {

    Impl(WebSocket* socket, std::unique_ptr<TCPConnection> conn)
        : conn(std::move(conn)) {

        handshake();

        thread = std::thread([this] {
            listen();
        });
    }

    void handshake() {
        std::vector<unsigned char> buffer(1024);
        auto bytesReceived = conn->read(buffer);
        if (bytesReceived == -1) {
            throwSocketError("Failed to read handshake request from client.");
        }

        std::string request(buffer.begin(), buffer.begin() + bytesReceived);
        std::string::size_type keyPos = request.find("Sec-WebSocket-Key: ");
        if (keyPos == std::string::npos) {
            throwSocketError("Client handshake request is invalid.");
        }
        keyPos += 19;
        std::string::size_type keyEnd = request.find("\r\n", keyPos);
        std::string clientKey = request.substr(keyPos, keyEnd - keyPos);

        std::string acceptKey = websocketAcceptKey(clientKey);

        std::ostringstream response;
        response << "HTTP/1.1 101 Switching Protocols\r\n"
                 << "Upgrade: websocket\r\n"
                 << "Connection: Upgrade\r\n"
                 << "Sec-WebSocket-Accept: " << acceptKey << "\r\n\r\n";

        std::string responseStr = response.str();
        send(responseStr);
        if (!conn->write(responseStr)) {
            throwSocketError("Failed to send handshake response");
        }
    }

    void send(const std::string& message) {
        auto frame = createFrame(message);
        conn->write(frame);
    }

    void close() {
        std::vector<uint8_t> closeFrame = {0x88}; // FIN, Close frame
        closeFrame.push_back(0);
        conn->write(closeFrame);
        conn->close();
    }

    void listen() {
        std::vector<unsigned char> buffer(1024);
        while (true) {

            int recv = conn->read(buffer);
            if (recv == -1) {
                break;
            }

            std::vector<uint8_t> frame{buffer.begin(), buffer.begin() + recv};

            if (frame.size() < 2) return;

            uint8_t opcode = frame[0] & 0x0F;
            bool isFinal = (frame[0] & 0x80) != 0;
            bool isMasked = (frame[1] & 0x80) != 0;
            uint64_t payloadLen = frame[1] & 0x7F;

            size_t pos = 2;
            if (payloadLen == 126) {
                payloadLen = (frame[2] << 8) | frame[3];
                pos += 2;
            } else if (payloadLen == 127) {
                payloadLen = 0;
                for (int i = 0; i < 8; ++i) {
                    payloadLen = (payloadLen << 8) | frame[2 + i];
                }
                pos += 8;
            }

            std::vector<uint8_t> mask(4);
            if (isMasked) {
                for (int i = 0; i < 4; ++i) {
                    mask[i] = frame[pos++];
                }
            }

            std::vector<uint8_t> payload(frame.begin() + pos, frame.begin() + pos + payloadLen);
            if (isMasked) {
                for (size_t i = 0; i < payload.size(); ++i) {
                    payload[i] ^= mask[i % 4];
                }
            }

            switch (opcode) {
                case 0x1: // Text frame
                {
                    std::string message(payload.begin(), payload.end());
                    std::cout << "Received message: " << message << std::endl;

                    auto responseFrame = createFrame(message);
                    conn->write(responseFrame);
                }
                break;
                case 0x8: // Close frame
                    std::cout << "Received close frame" << std::endl;
                    conn->close();
                    break;
                case 0x9: // Ping frame
                    std::cout << "Received ping frame" << std::endl;
                    {
                        std::vector<uint8_t> pongFrame = {0x8A}; // FIN, Pong frame
                        pongFrame.push_back(static_cast<uint8_t>(payload.size()));
                        pongFrame.insert(pongFrame.end(), payload.begin(), payload.end());
                        conn->write(pongFrame);
                    }
                    break;
                case 0xA: // Pong frame
                    std::cout << "Received pong frame" << std::endl;
                    break;
                default:
                    std::cerr << "Unsupported opcode: " << static_cast<int>(opcode) << std::endl;
                    break;
            }
        }
    }

    ~Impl() {
        close();
        if (thread.joinable()) {
            thread.join();
        }
    }

    WebSocket* socket;
    std::unique_ptr<TCPConnection> conn;
    std::thread thread;
};

WebSocketConnection::WebSocketConnection(WebSocket* socket, std::unique_ptr<TCPConnection> conn)
    : pimpl_(std::make_unique<Impl>(socket, std::move(conn))) {}

void WebSocketConnection::send(const std::string& message) {
    pimpl_->send(message);
}

WebSocketConnection::~WebSocketConnection() = default;


struct WebSocket::Impl {

public:
    explicit Impl(WebSocket* scope, uint16_t port)
        : scope(scope), socket(port) {

        thread = std::thread([this] {
            run();
        });
    }

    void run() {

        std::vector<std::unique_ptr<WebSocketConnection>> connections;

        while (!stop) {

            try {
                auto ws = std::make_unique<WebSocketConnection>(scope, socket.accept());
                scope->onOpen(ws.get());
                connections.emplace_back(std::move(ws));
            } catch (std::exception&) {
                continue;
            }
        }
    }

    ~Impl() {
        stop = true;
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::atomic_bool stop = false;
    TCPServer socket;
    std::thread thread;
    WebSocket* scope;
};


WebSocket::WebSocket(uint16_t port)
    : pimpl_(std::make_unique<Impl>(this, port)) {}

void WebSocket::stop() {
    pimpl_->stop = true;
    pimpl_->socket.close();
}

WebSocket::~WebSocket() = default;
