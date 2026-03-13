#include "TestFramework.h"

#include "AppConfig.h"

using namespace ctg::config;

CTG_TEST(AppConfig_DefaultValues)
{
    AppConfig cfg = MakeDefaultConfig();

    CTG_EXPECT_EQ(cfg.usableTime.startMinutes, 8 * 60);
    CTG_EXPECT_EQ(cfg.usableTime.endMinutes, 21 * 60);
    CTG_EXPECT_EQ(cfg.warnMinutes, 2);
    CTG_EXPECT_EQ(cfg.maxContinuousMinutes, 30);
    CTG_EXPECT_EQ(cfg.restMinutes, 20);
    CTG_EXPECT_TRUE(cfg.autoStart);
}

