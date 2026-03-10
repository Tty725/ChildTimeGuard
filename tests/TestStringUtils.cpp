#include "TestFramework.h"

#include "StringUtils.h"

using namespace ctg::strutils;

CTG_TEST(StringUtils_Trim_Basic)
{
    CTG_EXPECT_EQ(Trim(L""), std::wstring());
    CTG_EXPECT_EQ(Trim(L"  abc  "), std::wstring(L"abc"));
    CTG_EXPECT_EQ(Trim(L"\t\r\nabc\r\n"), std::wstring(L"abc"));
}

CTG_TEST(StringUtils_Utf8_Utf16_Roundtrip)
{
    std::string utf8 = u8"儿童时间守护";
    auto w = Utf8ToUtf16(utf8);
    auto back = Utf16ToUtf8(w);
    CTG_EXPECT_EQ(back, utf8);
}

