#include "JsonConfigStore.h"

#include <fstream>
#include <sstream>

#include <windows.h>

#include "StringUtils.h"
#include "TimeUtils.h"
#include "Base64Util.h"

namespace ctg {
namespace config {

    namespace {

        bool ExtractString(const std::string& json,
                           const std::string& key,
                           std::string& valueOut)
        {
            const std::string pattern = "\"" + key + "\"";
            auto pos = json.find(pattern);
            if (pos == std::string::npos)
                return false;

            pos = json.find(':', pos);
            if (pos == std::string::npos)
                return false;

            pos = json.find('"', pos);
            if (pos == std::string::npos)
                return false;

            auto end = json.find('"', pos + 1);
            if (end == std::string::npos)
                return false;

            valueOut.assign(json.begin() + static_cast<std::ptrdiff_t>(pos + 1),
                            json.begin() + static_cast<std::ptrdiff_t>(end));
            return true;
        }

        bool ExtractInt(const std::string& json,
                        const std::string& key,
                        int& valueOut)
        {
            const std::string pattern = "\"" + key + "\"";
            auto pos = json.find(pattern);
            if (pos == std::string::npos)
                return false;

            pos = json.find(':', pos);
            if (pos == std::string::npos)
                return false;

            ++pos;
            while (pos < json.size() && isspace(static_cast<unsigned char>(json[pos])))
                ++pos;

            auto end = pos;
            while (end < json.size() &&
                   (isdigit(static_cast<unsigned char>(json[end])) || json[end] == '-' ))
            {
                ++end;
            }

            if (end == pos)
                return false;

            try
            {
                valueOut = std::stoi(json.substr(pos, end - pos));
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        bool ExtractBool(const std::string& json,
                         const std::string& key,
                         bool& valueOut)
        {
            const std::string pattern = "\"" + key + "\"";
            auto pos = json.find(pattern);
            if (pos == std::string::npos)
                return false;

            pos = json.find(':', pos);
            if (pos == std::string::npos)
                return false;

            ++pos;
            while (pos < json.size() && isspace(static_cast<unsigned char>(json[pos])))
                ++pos;

            if (json.compare(pos, 4, "true") == 0)
            {
                valueOut = true;
                return true;
            }

            if (json.compare(pos, 5, "false") == 0)
            {
                valueOut = false;
                return true;
            }

            return false;
        }

        void EnsureDirectoryForPath(const std::wstring& path)
        {
            std::wstring dir = path;
            const auto pos = dir.find_last_of(L"\\/");
            if (pos != std::wstring::npos)
            {
                dir.resize(pos);
                if (!dir.empty())
                {
                    CreateDirectoryW(dir.c_str(), nullptr);
                }
            }
        }
    } // namespace

    bool JsonConfigStore::LoadFromFile(const std::wstring& path, AppConfig& cfg)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            return false;
        }

        std::ostringstream oss;
        oss << file.rdbuf();
        std::string json = oss.str();

        // If parsing fails, keep cfg as is (usually defaults).
        std::string start;
        std::string end;

        if (ExtractString(json, "start", start))
        {
            std::wstring wStart = ctg::strutils::Utf8ToUtf16(start);
            auto minutes = ctg::timeutils::ParseTimeHM(wStart);
            if (minutes >= 0)
            {
                cfg.usableTime.startMinutes = minutes;
            }
        }

        if (ExtractString(json, "end", end))
        {
            std::wstring wEnd = ctg::strutils::Utf8ToUtf16(end);
            auto minutes = ctg::timeutils::ParseTimeHM(wEnd);
            if (minutes >= 0)
            {
                cfg.usableTime.endMinutes = minutes;
            }
        }

        ExtractInt(json, "warn_minutes", cfg.warnMinutes);
        ExtractInt(json, "max_continuous_minutes", cfg.maxContinuousMinutes);
        ExtractInt(json, "rest_minutes", cfg.restMinutes);

        // Parent PIN: configuration file stores only Base64-encoded value.
        // If missing or invalid, keep current (usually default "2026").
        std::string pinB64;
        if (ExtractString(json, "parent_pin_b64", pinB64))
        {
            std::string pinUtf8;
            if (ctg::base64::Decode(pinB64, pinUtf8) && !pinUtf8.empty())
            {
                cfg.parentPinPlainUi = pinUtf8;
            }
        }

        ExtractBool(json, "auto_start", cfg.autoStart);

        std::string language;
        if (ExtractString(json, "language", language))
        {
            cfg.languageCode = language;
        }

        return true;
    }

    bool JsonConfigStore::SaveToFile(const std::wstring& path, const AppConfig& cfg)
    {
        // Ensure directory exists.
        EnsureDirectoryForPath(path);

        std::ofstream file(path, std::ios::trunc);
        if (!file.is_open())
        {
            return false;
        }

        std::wstring startHM = ctg::timeutils::FormatTimeHM(cfg.usableTime.startMinutes);
        std::wstring endHM = ctg::timeutils::FormatTimeHM(cfg.usableTime.endMinutes);

        std::string startUtf8 = ctg::strutils::Utf16ToUtf8(startHM);
        std::string endUtf8 = ctg::strutils::Utf16ToUtf8(endHM);

        if (startUtf8.empty())
        {
            startUtf8 = "08:00";
        }
        if (endUtf8.empty())
        {
            endUtf8 = "21:00";
        }

        file << "{\n";
        file << "  \"usable_time\": { \"start\": \"" << startUtf8
             << "\", \"end\": \"" << endUtf8 << "\" },\n";
        file << "  \"warn_minutes\": " << cfg.warnMinutes << ",\n";
        file << "  \"max_continuous_minutes\": " << cfg.maxContinuousMinutes << ",\n";
        file << "  \"rest_minutes\": " << cfg.restMinutes << ",\n";

        std::string pinUtf8 = cfg.parentPinPlainUi.empty()
            ? std::string("2026")
            : cfg.parentPinPlainUi;
        std::string pinB64 = ctg::base64::Encode(pinUtf8);

        file << "  \"parent_pin_b64\": \"" << pinB64 << "\",\n";
        file << "  \"auto_start\": " << (cfg.autoStart ? "true" : "false") << ",\n";
        file << "  \"language\": \"" << cfg.languageCode << "\"\n";
        file << "}\n";

        return true;
    }

} // namespace config
} // namespace ctg

