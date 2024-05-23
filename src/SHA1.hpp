
#ifndef SIMPLE_SOCKET_SHA1_HPP
#define SIMPLE_SOCKET_SHA1_HPP

#include <string>
#include <istream>

// SHA1 hashing function
class SHA1 {
public:
    SHA1() {
        reset();
    }

    void update(const std::string& s) {
        for (char c : s) {
            update_(c);
        }
    }

    void update(std::istream& is) {
        char sbuf[STREAM_BUFFER_SIZE];
        while (is) {
            is.read(sbuf, STREAM_BUFFER_SIZE);
            update_(reinterpret_cast<uint8_t*>(sbuf), is.gcount());
        }
    }

    std::string final() {
        uint8_t digest[HASH_SIZE];
        finalize(digest);

        char buf[HASH_SIZE*2 + 1];
        buf[HASH_SIZE*2] = 0;
        for (int i = 0; i < HASH_SIZE; ++i)
            sprintf(buf + i*2, "%02x", digest[i]);

        return {buf};
    }

private:
    static const unsigned int HASH_SIZE = 20;
    static const unsigned int BLOCK_INTS = 16;
    static const unsigned int BLOCK_BYTES = BLOCK_INTS * 4;
    static const unsigned int STREAM_BUFFER_SIZE = 8192;

    uint32_t digest[5];
    std::string buffer;
    uint64_t transforms;

    void reset() {
        digest[0] = 0x67452301;
        digest[1] = 0xEFCDAB89;
        digest[2] = 0x98BADCFE;
        digest[3] = 0x10325476;
        digest[4] = 0xC3D2E1F0;

        buffer = "";
        transforms = 0;
    }

    void update_(uint8_t block[BLOCK_BYTES], std::size_t n) {
        buffer += std::string((const char*) block, n);
        while (buffer.size() >= BLOCK_BYTES) {
            transform();
            buffer.erase(0, BLOCK_BYTES);
        }
    }

    void update_(const std::string &s) {
        for (char c : s) {
            update_((uint8_t)c);
        }
    }

    void update_(uint8_t byte) {
        buffer += byte;
        if (buffer.size() == BLOCK_BYTES) {
            transform();
            buffer.clear();
        }
    }

    void transform() {
        uint32_t block[BLOCK_INTS];
        decode(block, buffer.data(), BLOCK_BYTES);

        uint32_t a = digest[0];
        uint32_t b = digest[1];
        uint32_t c = digest[2];
        uint32_t d = digest[3];
        uint32_t e = digest[4];

// 4 rounds of 20 operations each. Loop unrolled.
#define SHA1_ROUND(f, k) \
            { uint32_t temp = ROTATE_LEFT(a, 5) + f(b, c, d) + e + k + block[i]; e = d; d = c; c = ROTATE_LEFT(b, 30); b = a; a = temp; }
        for (unsigned int i = 0; i < 16; ++i) {
            SHA1_ROUND(F1, 0x5A827999)
        }
        for (unsigned int i = 16; i < 20; ++i) {
            SHA1_ROUND(F2, 0x6ED9EBA1)
        }
        for (unsigned int i = 20; i < 40; ++i) {
            SHA1_ROUND(F3, 0x8F1BBCDC)
        }
        for (unsigned int i = 40; i < 60; ++i) {
            SHA1_ROUND(F4, 0xCA62C1D6)
        }
        for (unsigned int i = 60; i < 80; ++i) {
            SHA1_ROUND(F4, 0xCA62C1D6)
        }
#undef SHA1_ROUND

        digest[0] += a;
        digest[1] += b;
        digest[2] += c;
        digest[3] += d;
        digest[4] += e;

        ++transforms;
    }

    void finalize(uint8_t digest[HASH_SIZE]) {
        uint64_t total_bits = (transforms * BLOCK_BYTES + buffer.size()) * 8;
        buffer += (char)0x80;
        unsigned int orig_size = buffer.size();
        while (buffer.size() < BLOCK_BYTES - 8) {
            buffer += (char)0x00;
        }

        uint32_t last_block[BLOCK_INTS];
        decode(last_block, buffer.data(), BLOCK_BYTES);
        last_block[BLOCK_INTS-1] = total_bits;
        last_block[BLOCK_INTS-2] = (total_bits >> 32);

        transform();
        encode(digest, this->digest, HASH_SIZE);
    }

    static inline uint32_t F1(uint32_t b, uint32_t c, uint32_t d) {
        return d ^ (b & (c ^ d));
    }

    static inline uint32_t F2(uint32_t b, uint32_t c, uint32_t d) {
        return b ^ c ^ d;
    }

    static inline uint32_t F3(uint32_t b, uint32_t c, uint32_t d) {
        return (b & c) | (d & (b | c));
    }

    static inline uint32_t F4(uint32_t b, uint32_t c, uint32_t d) {
        return b ^ c ^ d;
    }

    static inline uint32_t ROTATE_LEFT(uint32_t x, unsigned int n) {
        return (x << n) | (x >> (32 - n));
    }

    static void decode(uint32_t output[], const char* input, unsigned int len) {
        for (unsigned int i = 0, j = 0; j < len; ++i, j += 4) {
            output[i] = ((uint32_t)(uint8_t)input[j] << 24)
                        | ((uint32_t)(uint8_t)input[j + 1] << 16)
                        | ((uint32_t)(uint8_t)input[j + 2] << 8)
                        | ((uint32_t)(uint8_t)input[j + 3]);
        }
    }

    static void encode(uint8_t output[], const uint32_t input[], unsigned int len) {
        for (unsigned int i = 0, j = 0; j < len; ++i, j += 4) {
            output[j]     = (input[i] >> 24) & 0xff;
            output[j + 1] = (input[i] >> 16) & 0xff;
            output[j + 2] = (input[i] >> 8) & 0xff;
            output[j + 3] = input[i] & 0xff;
        }
    }
};

#endif//SIMPLE_SOCKET_SHA1_HPP
