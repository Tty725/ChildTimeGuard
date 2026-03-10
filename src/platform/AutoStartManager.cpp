#include "AutoStartManager.h"

#include <string>

namespace ctg {
namespace platform {

    namespace {

        constexpr const wchar_t* kRunKeyPath =
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

        bool OpenRunKey(HKEY& hKey)
        {
            LONG result = ::RegCreateKeyExW(
                HKEY_CURRENT_USER,
                kRunKeyPath,
                0,
                nullptr,
                REG_OPTION_NON_VOLATILE,
                KEY_READ | KEY_WRITE,
                nullptr,
                &hKey,
                nullptr);
            return (result == ERROR_SUCCESS);
        }

        std::wstring NormalizeExePath(const wchar_t* exePath)
        {
            if (!exePath || *exePath == L'\0')
                return {};

            std::wstring path(exePath);

            // 如果没有被引号包裹，则添加一对引号，避免路径中有空格。
            if (!(path.size() >= 2 && path.front() == L'\"' && path.back() == L'\"'))
            {
                path.insert(path.begin(), L'\"');
                path.push_back(L'\"');
            }

            return path;
        }
    } // namespace

    bool AutoStartManager::EnableAutoStart(const wchar_t* appName, const wchar_t* exePath)
    {
        if (!appName || !exePath)
            return false;

        HKEY hKey = nullptr;
        if (!OpenRunKey(hKey))
            return false;

        std::wstring value = NormalizeExePath(exePath);
        if (value.empty())
        {
            ::RegCloseKey(hKey);
            return false;
        }

        const DWORD bytes =
            static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));

        LONG result = ::RegSetValueExW(
            hKey,
            appName,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            bytes);

        ::RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
    }

    bool AutoStartManager::DisableAutoStart(const wchar_t* appName)
    {
        if (!appName)
            return false;

        HKEY hKey = nullptr;
        if (!OpenRunKey(hKey))
            return false;

        LONG result = ::RegDeleteValueW(hKey, appName);
        ::RegCloseKey(hKey);

        // 如果键不存在也视作“已禁用”。
        return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
    }

    bool AutoStartManager::IsAutoStartEnabled(const wchar_t* appName)
    {
        if (!appName)
            return false;

        HKEY hKey = nullptr;
        if (!OpenRunKey(hKey))
            return false;

        LONG result = ::RegQueryValueExW(
            hKey,
            appName,
            nullptr,
            nullptr,
            nullptr,
            nullptr);

        ::RegCloseKey(hKey);

        return (result == ERROR_SUCCESS);
    }

} // namespace platform
} // namespace ctg

