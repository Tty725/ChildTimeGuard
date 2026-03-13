#include "TestFramework.h"

#include <cstring>
#include <fstream>

#include <windows.h>

#include "ResourceManager.h"

using namespace ctg::i18n;

CTG_TEST(ResourceManager_DefaultLanguage_IsEnUS)
{
    ResourceManager rm;
    CTG_EXPECT_EQ(rm.GetLanguage(), std::wstring(L"en-US"));
    // With no file loaded, GetString returns key as fallback.
    CTG_EXPECT_EQ(rm.GetString(L"Child Time Guard"), std::wstring(L"Child Time Guard"));
}

CTG_TEST(ResourceManager_LoadFromFile_AndSwitchToZhCN)
{
    ResourceManager rm;
    // Minimal valid language.json in UTF-8 (one key per lang for test).
    const char* json = u8"{\"zh-CN\":{\"_display\":\"\u7b80\u4f53\u4e2d\u6587\",\"OK\":\"\u786e\u5b9d\"},\"en-US\":{\"_display\":\"English\",\"OK\":\"OK\"}}";
    wchar_t path[MAX_PATH];
    if (::GetTempPathW(MAX_PATH, path) == 0)
        return;
    std::wstring tmpPath(path);
    tmpPath += L"ctg_test_language.json";
    std::ofstream f(tmpPath, std::ios::binary);
    if (!f)
        return;
    f.write(json, std::strlen(json));
    f.close();
    CTG_EXPECT_TRUE(rm.LoadFromFile(tmpPath));
    ::DeleteFileW(tmpPath.c_str());

    rm.SetLanguage(L"zh-CN");
    CTG_EXPECT_EQ(rm.GetLanguage(), std::wstring(L"zh-CN"));
    CTG_EXPECT_EQ(rm.GetString(L"OK"), std::wstring(L"\u786e\u5b9d"));  // 确定
    CTG_EXPECT_EQ(rm.GetAvailableLanguageCodes().size(), 2u);
    CTG_EXPECT_EQ(rm.GetLanguageDisplayName(L"zh-CN"), std::wstring(L"\u7b80\u4f53\u4e2d\u6587"));
}
