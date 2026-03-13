#include "ConfigManager.h"

#include <windows.h>
#include <shlobj.h>

#include "JsonConfigStore.h"

namespace ctg {
namespace config {

    namespace {

        std::wstring BuildConfigPath()
        {
            wchar_t path[MAX_PATH]{};
            if (SUCCEEDED(SHGetFolderPathW(
                    nullptr,
                    CSIDL_APPDATA,
                    nullptr,
                    SHGFP_TYPE_CURRENT,
                    path)))
            {
                std::wstring result(path);
                if (!result.empty() && result.back() != L'\\')
                {
                    result.push_back(L'\\');
                }
                result += L"ChildTimeGuard\\config.json";
                return result;
            }

            // Fallback to current directory.
            return L"config.json";
        }

        std::wstring BuildLanguagePath()
        {
            wchar_t path[MAX_PATH]{};
            if (SUCCEEDED(SHGetFolderPathW(
                    nullptr,
                    CSIDL_APPDATA,
                    nullptr,
                    SHGFP_TYPE_CURRENT,
                    path)))
            {
                std::wstring result(path);
                if (!result.empty() && result.back() != L'\\')
                {
                    result.push_back(L'\\');
                }
                result += L"ChildTimeGuard\\language.json";
                return result;
            }
            return L"language.json";
        }
    } // namespace

    ConfigManager::ConfigManager()
        : m_config(MakeDefaultConfig())
    {
    }

    bool ConfigManager::Load()
    {
        m_config = MakeDefaultConfig();
        const std::wstring path = GetConfigFilePath();
        return JsonConfigStore::LoadFromFile(path, m_config);
    }

    bool ConfigManager::Save() const
    {
        const std::wstring path = GetConfigFilePath();
        return JsonConfigStore::SaveToFile(path, m_config);
    }

    std::wstring ConfigManager::GetConfigFilePath()
    {
        static const std::wstring s_path = BuildConfigPath();
        return s_path;
    }

    std::wstring ConfigManager::GetLanguageFilePath()
    {
        static const std::wstring s_path = BuildLanguagePath();
        return s_path;
    }

} // namespace config
} // namespace ctg

