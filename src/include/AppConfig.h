#pragma once

#include <string>

namespace ctg {
namespace config {

    // Minutes within a day [0, 24*60).
    using MinutesOfDay = int;

    struct DailyTimeRange
    {
        MinutesOfDay startMinutes{ 0 };
        MinutesOfDay endMinutes{ 0 };
    };

    struct AppConfig
    {
        DailyTimeRange usableTime;        // Allowed time range
        int warnMinutes{ 0 };            // Countdown warning minutes (M)
        int maxContinuousMinutes{ 0 };   // Max continuous usage minutes (N)
        int restMinutes{ 0 };            // Rest lock screen minutes (X)

        // Parent PIN in plain text (UTF-8), used only at runtime for
        // UI display and verification. Configuration file stores only
        // a Base64-encoded version of this value.
        std::string parentPinPlainUi;

        bool autoStart{ false };         // Auto-start with user login

        // Language code, e.g. "zh-CN", "en-US", "auto".
        // Empty string means "auto by system language".
        std::string languageCode;
    };

    // Make an AppConfig instance filled with default values.
    AppConfig MakeDefaultConfig();

} // namespace config
} // namespace ctg

