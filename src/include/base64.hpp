#ifndef BASE64_HPP
#define BASE64_HPP

#include <string>
#include <vector>
#include <stdexcept>

// A simple base64 encoding and decoding implementation
namespace {

static const char* BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

inline bool isBase64Char(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

} // anonymous namespace


// -------------------------------------------------------------------------
// base64Encode
//    Takes a pointer to bytes and their length, produces a base64 string
// -------------------------------------------------------------------------
static inline std::string base64Encode(const uint8_t* data, size_t len)
{
    std::string encoded;
    encoded.reserve(((len + 2) / 3) * 4);

    uint32_t temp{};
    for (size_t i = 0; i < len; i += 3) {
        temp = data[i] << 16;

        if ((i + 1) < len) {
            temp |= data[i + 1] << 8;
        }
        if ((i + 2) < len) {
            temp |= data[i + 2];
        }

        // Extract 4 x 6-bit groups
        encoded.push_back(BASE64_CHARS[(temp >> 18) & 0x3F]);
        encoded.push_back(BASE64_CHARS[(temp >> 12) & 0x3F]);
        if ((i + 1) < len)
            encoded.push_back(BASE64_CHARS[(temp >> 6) & 0x3F]);
        else
            encoded.push_back('=');

        if ((i + 2) < len)
            encoded.push_back(BASE64_CHARS[temp & 0x3F]);
        else
            encoded.push_back('=');
    }

    return encoded;
}


// -------------------------------------------------------------------------
// base64Decode
//    Takes a base64-encoded string, returns a vector of bytes
// -------------------------------------------------------------------------
static inline std::vector<uint8_t> base64Decode(const std::string &encoded)
{
    // Remove any padding or whitespace
    // (Optional: some implementations do or do not expect whitespace.)
    std::string clean;
    clean.reserve(encoded.size());
    for (char c : encoded) {
        if (isBase64Char(static_cast<unsigned char>(c)) || c == '=')
            clean.push_back(c);
    }

    if (clean.size() % 4 != 0) {
        throw std::runtime_error("Invalid base64 input length.");
    }

    size_t padding = 0;
    if (clean.size() >= 2) {
        if (clean[clean.size() - 1] == '=') padding++;
        if (clean[clean.size() - 2] == '=') padding++;
    }

    // Each 4-character group decodes to 3 bytes (except for padding)
    std::vector<uint8_t> decoded;
    decoded.reserve(((clean.size() / 4) * 3) - padding);

    uint32_t temp = 0;
    for (size_t i = 0; i < clean.size(); i += 4) {
        // Build 24-bit number from 4 base64 chars
        // (Use a LUT or pointer arithmetic to speed it up in production)
        uint32_t val = 0;
        for (int j = 0; j < 4; j++) {
            val <<= 6;
            if (clean[i + j] == '=') {
                // do nothing, zero bits
            } else {
                const char* p = std::strchr(BASE64_CHARS, clean[i + j]);
                if (!p) {
                    throw std::runtime_error("Invalid base64 character encountered.");
                }
                val |= static_cast<uint32_t>(p - BASE64_CHARS);
            }
        }

        decoded.push_back((val >> 16) & 0xFF);
        if (clean[i + 2] != '=') {
            decoded.push_back((val >> 8) & 0xFF);
        }
        if (clean[i + 3] != '=') {
            decoded.push_back(val & 0xFF);
        }
    }

    return decoded;
}

#endif // BASE64_HPP
