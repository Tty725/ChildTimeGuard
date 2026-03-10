#pragma once

#include <string>

#include "AppConfig.h"

namespace ctg {
namespace config {

    // Responsible for reading/writing AppConfig to a JSON configuration file.
    class JsonConfigStore
    {
    public:
        // Load configuration from the given path.
        // On failure, keep cfg unchanged and return false.
        static bool LoadFromFile(const std::wstring& path, AppConfig& cfg);

        // Save configuration to the given path.
        // Return false on failure.
        static bool SaveToFile(const std::wstring& path, const AppConfig& cfg);
    };

} // namespace config
} // namespace ctg

