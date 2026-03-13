#include "ResourceManager.h"

#include <fstream>
#include <sstream>
#include <unordered_set>

#include <windows.h>

#include "StringUtils.h"
#include "resource.h"

namespace ctg {
namespace i18n {

    namespace {

        static ResourceManager* g_instance = nullptr;

        void SkipWhitespace(const std::string& s, size_t& i)
        {
            while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n'))
                ++i;
        }

        // Parse JSON string at i (expect opening "); return UTF-8 string, i advanced past closing ".
        bool ParseJsonString(const std::string& s, size_t& i, std::string& out)
        {
            if (i >= s.size() || s[i] != '"')
                return false;
            ++i;
            out.clear();
            while (i < s.size())
            {
                if (s[i] == '"')
                {
                    ++i;
                    return true;
                }
                if (s[i] == '\\' && i + 1 < s.size())
                {
                    if (s[i + 1] == '"') { out += '"'; i += 2; continue; }
                    if (s[i + 1] == '\\') { out += '\\'; i += 2; continue; }
                }
                out += s[i++];
            }
            return false;
        }

        // Parse inner object { "k": "v", ... }, fill table (keys/values converted to wstring via Utf8ToUtf16).
        bool ParseInnerObject(const std::string& s, size_t& i,
                              std::unordered_map<std::wstring, std::wstring>& table)
        {
            SkipWhitespace(s, i);
            if (i >= s.size() || s[i] != '{')
                return false;
            ++i;
            for (;;)
            {
                SkipWhitespace(s, i);
                if (i < s.size() && s[i] == '}')
                {
                    ++i;
                    return true;
                }
                std::string keyUtf8, valUtf8;
                if (!ParseJsonString(s, i, keyUtf8))
                    return false;
                SkipWhitespace(s, i);
                if (i >= s.size() || s[i] != ':')
                    return false;
                ++i;
                SkipWhitespace(s, i);
                if (!ParseJsonString(s, i, valUtf8))
                    return false;
                std::wstring kw = ctg::strutils::Utf8ToUtf16(keyUtf8);
                std::wstring vw = ctg::strutils::Utf8ToUtf16(valUtf8);
                table[kw] = std::move(vw);
                SkipWhitespace(s, i);
                if (i < s.size() && s[i] == ',')
                {
                    ++i;
                    continue;
                }
            }
        }

        // Parse root object { "zh-CN": { ... }, "en-US": { ... } }, fill tables and order.
        bool ParseRootObject(const std::string& s, size_t& i,
                             std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>>& tables,
                             std::vector<std::wstring>& order)
        {
            SkipWhitespace(s, i);
            if (i >= s.size() || s[i] != '{')
                return false;
            ++i;
            for (;;)
            {
                SkipWhitespace(s, i);
                if (i < s.size() && s[i] == '}')
                {
                    ++i;
                    return true;
                }
                std::string keyUtf8;
                if (!ParseJsonString(s, i, keyUtf8))
                    return false;
                std::wstring langCode = ctg::strutils::Utf8ToUtf16(keyUtf8);
                SkipWhitespace(s, i);
                if (i >= s.size() || s[i] != ':')
                    return false;
                ++i;
                SkipWhitespace(s, i);
                std::unordered_map<std::wstring, std::wstring> inner;
                if (!ParseInnerObject(s, i, inner))
                    return false;
                tables[langCode] = std::move(inner);
                order.push_back(langCode);
                SkipWhitespace(s, i);
                if (i < s.size() && s[i] == ',')
                {
                    ++i;
                    continue;
                }
            }
        }

        bool ParseLanguageJson(const std::string& utf8,
                               std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>>& tables,
                               std::vector<std::wstring>& order)
        {
            size_t i = 0;
            return ParseRootObject(utf8, i, tables, order);
        }

        std::unordered_set<std::wstring> GetKeySetExcludingDisplay(const std::unordered_map<std::wstring, std::wstring>& table)
        {
            std::unordered_set<std::wstring> keys;
            for (const auto& p : table)
            {
                if (p.first != L"_display")
                    keys.insert(p.first);
            }
            return keys;
        }

    } // namespace

    std::string ReadEmbeddedLanguageJson()
    {
        HMODULE hMod = ::GetModuleHandleW(nullptr);
        if (!hMod)
            return {};
        HRSRC hRes = ::FindResourceW(hMod, MAKEINTRESOURCEW(IDR_LANGUAGE_JSON),
                                     reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(10)));
        if (!hRes)
            return {};
        HGLOBAL hLoaded = ::LoadResource(hMod, hRes);
        if (!hLoaded)
            return {};
        const void* pData = ::LockResource(hLoaded);
        if (!pData)
            return {};
        DWORD size = ::SizeofResource(hMod, hRes);
        if (size == 0)
            return {};
        const char* p = static_cast<const char*>(pData);
        return std::string(p, p + size);
    }

    void ResourceManager::SetLanguage(const std::wstring& languageCode)
    {
        if (!languageCode.empty())
        {
            m_languageCode = languageCode;
        }
    }

    std::wstring ResourceManager::GetLanguage() const
    {
        return m_languageCode;
    }

    std::wstring ResourceManager::GetString(const std::wstring& key) const
    {
        auto it = m_tables.find(m_languageCode);
        if (it != m_tables.end())
        {
            auto it2 = it->second.find(key);
            if (it2 != it->second.end())
                return it2->second;
        }
        it = m_tables.find(L"en-US");
        if (it != m_tables.end())
        {
            auto it2 = it->second.find(key);
            if (it2 != it->second.end())
                return it2->second;
        }
        return key;
    }

    bool ResourceManager::LoadFromFile(const std::wstring& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return false;
        std::stringstream ss;
        ss << file.rdbuf();
        std::string utf8 = ss.str();
        if (utf8.empty())
            return false;
        std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>> tables;
        std::vector<std::wstring> order;
        if (!ParseLanguageJson(utf8, tables, order))
            return false;
        m_tables = std::move(tables);
        m_languageCodesOrder = std::move(order);
        return true;
    }

    bool ResourceManager::ValidateFileFormat(const std::wstring& path) const
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return false;
        std::stringstream ss;
        ss << file.rdbuf();
        std::string utf8 = ss.str();
        if (utf8.empty())
            return false;
        std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>> tables;
        std::vector<std::wstring> order;
        if (!ParseLanguageJson(utf8, tables, order))
            return false;

        std::string builtin = ReadEmbeddedLanguageJson();
        if (builtin.empty())
            return false;
        std::unordered_map<std::wstring, std::unordered_map<std::wstring, std::wstring>> builtinTables;
        std::vector<std::wstring> builtinOrder;
        if (!ParseLanguageJson(builtin, builtinTables, builtinOrder))
            return false;
        auto canonical = GetKeySetExcludingDisplay(builtinTables[L"en-US"]);
        if (canonical.empty())
            canonical = GetKeySetExcludingDisplay(builtinTables[L"zh-CN"]);

        for (const auto& p : tables)
        {
            auto fileKeys = GetKeySetExcludingDisplay(p.second);
            if (fileKeys != canonical)
                return false;
        }
        return true;
    }

    std::vector<std::wstring> ResourceManager::GetAvailableLanguageCodes() const
    {
        return m_languageCodesOrder;
    }

    std::wstring ResourceManager::GetLanguageDisplayName(const std::wstring& code) const
    {
        auto it = m_tables.find(code);
        if (it != m_tables.end())
        {
            auto it2 = it->second.find(L"_display");
            if (it2 != it->second.end())
                return it2->second;
        }
        return code;
    }

    ResourceManager* GetResourceManager()
    {
        return g_instance;
    }

    void SetResourceManager(ResourceManager* instance)
    {
        g_instance = instance;
    }

} // namespace i18n
} // namespace ctg
