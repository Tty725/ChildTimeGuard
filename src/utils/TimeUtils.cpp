#include "TimeUtils.h"

#include <windows.h>

namespace ctg {
namespace timeutils {
    MinutesOfDay ParseTimeHM(const std::wstring& text)
    {
        if (text.size() != 5 || text[2] != L':')
            return -1;

        int hh = (text[0] - L'0') * 10 + (text[1] - L'0');
        int mm = (text[3] - L'0') * 10 + (text[4] - L'0');

        if (hh < 0 || hh > 23 || mm < 0 || mm > 59)
            return -1;

        return hh * 60 + mm;
    }

    std::wstring FormatTimeHM(MinutesOfDay minutesOfDay)
    {
        if (minutesOfDay < 0 || minutesOfDay >= 24 * 60)
            return {};

        int hh = minutesOfDay / 60;
        int mm = minutesOfDay % 60;

        wchar_t buf[6]{};
        swprintf_s(buf, L"%02d:%02d", hh, mm);
        return buf;
    }

    std::wstring FormatSecondsToMMSS(int totalSeconds)
    {
        if (totalSeconds < 0)
            totalSeconds = 0;

        int mm = totalSeconds / 60;
        int ss = totalSeconds % 60;

        wchar_t buf[6]{};
        swprintf_s(buf, L"%02d:%02d", mm, ss);
        return buf;
    }

    MinutesOfDay GetNowMinutesOfDay()
    {
        SYSTEMTIME st{};
        GetLocalTime(&st);
        return static_cast<MinutesOfDay>(st.wHour * 60 + st.wMinute);
    }
} // namespace timeutils
} // namespace ctg

