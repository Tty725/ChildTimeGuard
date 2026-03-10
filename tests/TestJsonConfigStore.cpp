#include "TestFramework.h"

#include <filesystem>

#include "JsonConfigStore.h"

using namespace ctg::config;

CTG_TEST(JsonConfigStore_SaveAndLoad_Roundtrip)
{
    namespace fs = std::filesystem;

    AppConfig cfg = MakeDefaultConfig();
    cfg.warnMinutes = 5;
    cfg.maxContinuousMinutes = 45;
    cfg.restMinutes = 15;
    cfg.parentPinPlainUi = "4321";
    cfg.autoStart = false;
    cfg.languageCode = "en-US";

    fs::path temp = fs::temp_directory_path() / "ctg_config_test.json";

    bool saveOk = JsonConfigStore::SaveToFile(temp.wstring(), cfg);
    CTG_EXPECT_TRUE(saveOk);

    AppConfig loaded = MakeDefaultConfig();
    bool loadOk = JsonConfigStore::LoadFromFile(temp.wstring(), loaded);
    CTG_EXPECT_TRUE(loadOk);

    CTG_EXPECT_EQ(loaded.warnMinutes, cfg.warnMinutes);
    CTG_EXPECT_EQ(loaded.maxContinuousMinutes, cfg.maxContinuousMinutes);
    CTG_EXPECT_EQ(loaded.restMinutes, cfg.restMinutes);
    CTG_EXPECT_EQ(loaded.parentPinPlainUi, cfg.parentPinPlainUi);
    CTG_EXPECT_EQ(loaded.autoStart, cfg.autoStart);
    CTG_EXPECT_EQ(loaded.languageCode, cfg.languageCode);
}

