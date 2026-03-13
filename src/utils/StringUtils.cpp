#include "StringUtils.h"

#include <windows.h>

namespace ctg {
namespace strutils {
    namespace
    {
        bool IsWhitespace(wchar_t ch)
        {
            switch (ch)
            {
            case L' ':
            case L'\t':
            case L'\r':
            case L'\n':
            case L'\v':
            case L'\f':
                return true;
            default:
                return false;
            }
        }
    }

    std::wstring Trim(const std::wstring& text)
    {
        if (text.empty())
            return {};

        size_t start = 0;
        size_t end = text.size();

        while (start < end && IsWhitespace(text[start]))
        {
            ++start;
        }

        while (end > start && IsWhitespace(text[end - 1]))
        {
            --end;
        }

        return text.substr(start, end - start);
    }

    std::wstring Utf8ToUtf16(const std::string& utf8)
    {
        if (utf8.empty())
            return {};

        int needed = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                         static_cast<int>(utf8.size()),
                                         nullptr, 0);
        if (needed <= 0)
            return {};

        std::wstring result(static_cast<size_t>(needed), L'\0');
        int written = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                          static_cast<int>(utf8.size()),
                                          result.data(), needed);
        if (written != needed)
            return {};

        return result;
    }

    std::string Utf16ToUtf8(const std::wstring& utf16)
    {
        if (utf16.empty())
            return {};

        int needed = WideCharToMultiByte(CP_UTF8, 0, utf16.data(),
                                         static_cast<int>(utf16.size()),
                                         nullptr, 0, nullptr, nullptr);
        if (needed <= 0)
            return {};

        std::string result(static_cast<size_t>(needed), '\0');
        int written = WideCharToMultiByte(CP_UTF8, 0, utf16.data(),
                                          static_cast<int>(utf16.size()),
                                          result.data(), needed,
                                          nullptr, nullptr);
        if (written != needed)
            return {};

        return result;
    }
} // namespace strutils
} // namespace ctg

