#pragma once

#include <string>

#include "AppConfig.h"

namespace ctg {
namespace config {

    // Public entry point for configuration access and management.
    class ConfigManager
    {
    public:
        ConfigManager();

        // Reload configuration from disk (if present); always start from defaults.
        // Returns true if reading from disk succeeded (otherwise defaults remain).
        bool Load();

        // Persist current configuration to disk.
        bool Save() const;

        // Get current configuration (read-only).
        const AppConfig& Get() const noexcept { return m_config; }

        // Replace current configuration (typically from UI); does not auto-save.
        void Set(const AppConfig& cfg) { m_config = cfg; }

        // Get the full path of the configuration file (%AppData%\ChildTimeGuard\config.json).
        static std::wstring GetConfigFilePath();

        // Get the full path of the language file (%AppData%\ChildTimeGuard\language.json).
        static std::wstring GetLanguageFilePath();

    private:
        AppConfig m_config;
    };

} // namespace config
} // namespace ctg

