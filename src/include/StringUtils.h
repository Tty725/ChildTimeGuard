#pragma once

#include <string>

// String and encoding helper utilities.
namespace ctg {
namespace strutils {

    // Trim leading and trailing whitespace characters.
    std::wstring Trim(const std::wstring& text);

    // UTF-8 (std::string) -> UTF-16 (std::wstring); return empty string on failure.
    std::wstring Utf8ToUtf16(const std::string& utf8);

    // UTF-16 (std::wstring) -> UTF-8 (std::string); return empty string on failure.
    std::string Utf16ToUtf8(const std::wstring& utf16);

} // namespace strutils
} // namespace ctg

