#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// Simple multi-language string resource manager.
// Data is loaded from language.json (file or embedded resource).
namespace ctg {
namespace i18n {

    // Supported language codes examples: L"zh-CN", L"en-US".
    class ResourceManager
    {
    public:
        ResourceManager() = default;

        // Set current language code; empty string means "auto" (decide outside).
        void SetLanguage(const std::wstring& languageCode);

        // Get current language code.
        std::wstring GetLanguage() const;

        // Get localized string by key; fall back to en-US then key itself.
        std::wstring GetString(const std::wstring& key) const;

        // Load language data from JSON file at path (UTF-8). Returns true on success.
        bool LoadFromFile(const std::wstring& path);

        // Validate that file at path has correct format (all language nodes have same key set as canonical).
        // Canonical key set is taken from embedded language.json. Returns true if valid.
        bool ValidateFileFormat(const std::wstring& path) const;

        // Return language codes (top-level keys) from currently loaded data. Order preserved as in file.
        std::vector<std::wstring> GetAvailableLanguageCodes() const;

        // Return display name for language code (_display value, or code if missing).
        std::wstring GetLanguageDisplayName(const std::wstring& code) const;

    private:
        using StringTable = std::unordered_map<std::wstring, std::wstring>;
        std::unordered_map<std::wstring, StringTable> m_tables;
        std::vector<std::wstring> m_languageCodesOrder;
        std::wstring m_languageCode{ L"en-US" };
    };

    // Global access for UI: set by App after Load config and SetLanguage.
    ResourceManager* GetResourceManager();
    void SetResourceManager(ResourceManager* instance);

    // Read embedded language.json from exe resource. Returns UTF-8 content or empty on failure.
    std::string ReadEmbeddedLanguageJson();

} // namespace i18n
} // namespace ctg
