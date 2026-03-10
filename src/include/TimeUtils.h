#pragma once

#include <string>

// Time and duration related helper utilities.
namespace ctg {
namespace timeutils {

    // Minutes within a day [0, 24*60).
    using MinutesOfDay = int;

    // Parse "HH:MM" into MinutesOfDay; return -1 on failure.
    MinutesOfDay ParseTimeHM(const std::wstring& text);

    // Format MinutesOfDay as "HH:MM"; return empty string on invalid input.
    std::wstring FormatTimeHM(MinutesOfDay minutesOfDay);

    // Format seconds as "MM:SS" for countdown display.
    std::wstring FormatSecondsToMMSS(int totalSeconds);

    // Get current local time as MinutesOfDay.
    MinutesOfDay GetNowMinutesOfDay();

} // namespace timeutils
} // namespace ctg

