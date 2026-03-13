#include "Base64Util.h"

namespace ctg {
namespace base64 {

    namespace {

        const char kTable[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        inline bool IsBase64Char(unsigned char c)
        {
            return (c >= 'A' && c <= 'Z') ||
                   (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9') ||
                   c == '+' || c == '/';
        }
    } // namespace

    std::string Encode(const std::string& utf8)
    {
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(utf8.data());
        const std::size_t len = utf8.size();

        std::string out;
        out.reserve(((len + 2) / 3) * 4);

        std::size_t i = 0;
        while (i < len)
        {
            unsigned char b0 = bytes[i++];
            unsigned char b1 = (i < len) ? bytes[i++] : 0;
            unsigned char b2 = (i < len) ? bytes[i++] : 0;

            unsigned int triple = (b0 << 16) | (b1 << 8) | b2;

            out.push_back(kTable[(triple >> 18) & 0x3F]);
            out.push_back(kTable[(triple >> 12) & 0x3F]);

            if (i - 1 <= len)
            {
                out.push_back((i - 1 > len) ? '=' : kTable[(triple >> 6) & 0x3F]);
            }

            if (i <= len)
            {
                out.push_back((i > len) ? '=' : kTable[triple & 0x3F]);
            }
        }

        // Fix padding explicitly to avoid subtle off-by-one issues.
        std::size_t mod = len % 3;
        if (mod == 1)
        {
            out[out.size() - 1] = '=';
            out[out.size() - 2] = '=';
        }
        else if (mod == 2)
        {
            out[out.size() - 1] = '=';
        }

        return out;
    }

    bool Decode(const std::string& b64, std::string& utf8Out)
    {
        std::string clean;
        clean.reserve(b64.size());
        for (unsigned char c : b64)
        {
            if (IsBase64Char(c) || c == '=')
                clean.push_back(static_cast<char>(c));
        }

        std::size_t len = clean.size();
        if (len == 0 || len % 4 != 0)
            return false;

        auto DecodeChar = [](unsigned char c) -> int {
            if (c >= 'A' && c <= 'Z') return c - 'A';
            if (c >= 'a' && c <= 'z') return c - 'a' + 26;
            if (c >= '0' && c <= '9') return c - '0' + 52;
            if (c == '+') return 62;
            if (c == '/') return 63;
            return -1;
        };

        std::string out;
        out.reserve((len / 4) * 3);

        for (std::size_t i = 0; i < len; i += 4)
        {
            unsigned char c0 = clean[i];
            unsigned char c1 = clean[i + 1];
            unsigned char c2 = clean[i + 2];
            unsigned char c3 = clean[i + 3];

            int d0 = DecodeChar(c0);
            int d1 = DecodeChar(c1);
            int d2 = (c2 == '=') ? -1 : DecodeChar(c2);
            int d3 = (c3 == '=') ? -1 : DecodeChar(c3);

            if (d0 < 0 || d1 < 0 || (d2 < 0 && c2 != '=') || (d3 < 0 && c3 != '='))
                return false;

            unsigned int triple = (d0 << 18) | (d1 << 12) |
                                  ((d2 < 0 ? 0 : d2) << 6) |
                                  (d3 < 0 ? 0 : d3);

            out.push_back(static_cast<char>((triple >> 16) & 0xFF));
            if (c2 != '=')
                out.push_back(static_cast<char>((triple >> 8) & 0xFF));
            if (c3 != '=')
                out.push_back(static_cast<char>(triple & 0xFF));
        }

        utf8Out = out;
        return true;
    }

} // namespace base64
} // namespace ctg

