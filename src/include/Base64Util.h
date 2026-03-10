#pragma once

#include <string>

// Simple Base64 encoding/decoding helpers for UTF-8 strings.
namespace ctg {
namespace base64 {

    // Encode arbitrary binary data (provided as UTF-8 string) into Base64.
    std::string Encode(const std::string& utf8);

    // Decode Base64 string into original bytes (returned as UTF-8 string).
    // Returns true on success, false on invalid input.
    bool Decode(const std::string& b64, std::string& utf8Out);

} // namespace base64
} // namespace ctg

