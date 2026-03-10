#include "TestFramework.h"

#include "TimeUtils.h"

using namespace ctg::timeutils;

CTG_TEST(TimeUtils_ParseTimeHM_Valid)
{
    CTG_EXPECT_EQ(ParseTimeHM(L"00:00"), 0);
    CTG_EXPECT_EQ(ParseTimeHM(L"08:00"), 8 * 60);
    CTG_EXPECT_EQ(ParseTimeHM(L"23:59"), 23 * 60 + 59);
}

CTG_TEST(TimeUtils_ParseTimeHM_Invalid)
{
    CTG_EXPECT_EQ(ParseTimeHM(L"24:00"), -1);
    CTG_EXPECT_EQ(ParseTimeHM(L"ab:cd"), -1);
    CTG_EXPECT_EQ(ParseTimeHM(L"8:00"), -1);
}

CTG_TEST(TimeUtils_FormatTimeHM_Basic)
{
    CTG_EXPECT_EQ(FormatTimeHM(0), std::wstring(L"00:00"));
    CTG_EXPECT_EQ(FormatTimeHM(8 * 60), std::wstring(L"08:00"));
    CTG_EXPECT_EQ(FormatTimeHM(23 * 60 + 59), std::wstring(L"23:59"));
}

CTG_TEST(TimeUtils_FormatSecondsToMMSS)
{
    CTG_EXPECT_EQ(FormatSecondsToMMSS(0), std::wstring(L"00:00"));
    CTG_EXPECT_EQ(FormatSecondsToMMSS(59), std::wstring(L"00:59"));
    CTG_EXPECT_EQ(FormatSecondsToMMSS(60), std::wstring(L"01:00"));
    CTG_EXPECT_EQ(FormatSecondsToMMSS(125), std::wstring(L"02:05"));
}

