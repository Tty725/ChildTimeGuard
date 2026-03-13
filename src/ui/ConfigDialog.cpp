#include "ConfigDialog.h"

#include <cwchar>
#include <string>

#include <windows.h>

#include "resource.h"
#include "TimeUtils.h"
#include "StringUtils.h"
#include "AutoStartManager.h"
#include "ResourceManager.h"

namespace ctg {
namespace ui {

    using ctg::timeutils::FormatTimeHM;
    using ctg::timeutils::ParseTimeHM;

    namespace {

        struct ConfigDialogContext
        {
            HINSTANCE hInstance{ nullptr };
            config::AppConfig* config{ nullptr };
            HICON hIcon{ nullptr };
            bool firstRun{ false };
        };

        void SetTopMostAndIcon(HWND hDlg, ConfigDialogContext* ctx)
        {
            if (!ctx)
                return;

            ::SetWindowPos(
                hDlg,
                HWND_TOPMOST,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
            ::SetForegroundWindow(hDlg);

            HICON hIcon = reinterpret_cast<HICON>(::LoadImageW(
                ctx->hInstance,
                MAKEINTRESOURCEW(IDI_APP_ICON),
                IMAGE_ICON,
                0,
                0,
                LR_DEFAULTSIZE));
            if (hIcon)
            {
                ctx->hIcon = hIcon;
                ::SendMessageW(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
                ::SendMessageW(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
            }
        }

        void InitLanguageCombo(HWND hDlg, const config::AppConfig& cfg)
        {
            HWND hCombo = ::GetDlgItem(hDlg, IDC_COMBO_LANGUAGE);
            if (!hCombo)
                return;

            ::SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);

            ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
            std::wstring sAuto = rm ? rm->GetString(L"Auto") : L"Auto";
            ::SendMessageW(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(sAuto.c_str()));

            if (rm)
            {
                std::vector<std::wstring> codes = rm->GetAvailableLanguageCodes();
                for (const auto& code : codes)
                {
                    std::wstring display = rm->GetLanguageDisplayName(code);
                    ::SendMessageW(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(display.c_str()));
                }
            }

            int sel = 0;
            if (cfg.languageCode != "auto" && !cfg.languageCode.empty())
            {
                std::wstring want = ctg::strutils::Utf8ToUtf16(cfg.languageCode);
                if (rm)
                {
                    const std::vector<std::wstring> codes = rm->GetAvailableLanguageCodes();
                    for (size_t i = 0; i < codes.size(); ++i)
                    {
                        if (codes[i] == want)
                        {
                            sel = static_cast<int>(i + 1);
                            break;
                        }
                    }
                }
            }
            ::SendMessageW(hCombo, CB_SETCURSEL, static_cast<WPARAM>(sel), 0);
        }

        bool ReadTimeRange(HWND hDlg, config::DailyTimeRange& outRange)
        {
            wchar_t buf[16]{};
            ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
            auto getStr = [rm](const wchar_t* key) -> std::wstring {
                return rm ? rm->GetString(key) : std::wstring(key);
            };

            // Start time.
            if (::GetDlgItemTextW(hDlg, IDC_EDIT_START_TIME, buf, static_cast<int>(std::size(buf))) <= 0)
            {
                ::MessageBoxW(hDlg, getStr(L"Please enter a start time in HH:MM format.").c_str(), getStr(L"Invalid time").c_str(), MB_ICONWARNING | MB_OK);
                ::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_START_TIME));
                return false;
            }
            std::wstring startText = ctg::strutils::Trim(std::wstring(buf));
            auto startMinutes = ParseTimeHM(startText);
            if (startMinutes < 0)
            {
                ::MessageBoxW(hDlg, getStr(L"Start time must be in HH:MM (24-hour) format.").c_str(), getStr(L"Invalid time").c_str(), MB_ICONWARNING | MB_OK);
                ::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_START_TIME));
                return false;
            }

            // End time.
            std::wmemset(buf, 0, std::size(buf));
            if (::GetDlgItemTextW(hDlg, IDC_EDIT_END_TIME, buf, static_cast<int>(std::size(buf))) <= 0)
            {
                ::MessageBoxW(hDlg, getStr(L"Please enter an end time in HH:MM format.").c_str(), getStr(L"Invalid time").c_str(), MB_ICONWARNING | MB_OK);
                ::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_END_TIME));
                return false;
            }
            std::wstring endText = ctg::strutils::Trim(std::wstring(buf));
            auto endMinutes = ParseTimeHM(endText);
            if (endMinutes < 0)
            {
                ::MessageBoxW(hDlg, getStr(L"End time must be in HH:MM (24-hour) format.").c_str(), getStr(L"Invalid time").c_str(), MB_ICONWARNING | MB_OK);
                ::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_END_TIME));
                return false;
            }

            if (endMinutes <= startMinutes)
            {
                ::MessageBoxW(hDlg, getStr(L"End time must be later than start time.").c_str(), getStr(L"Invalid time range").c_str(), MB_ICONWARNING | MB_OK);
                ::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_END_TIME));
                return false;
            }

            outRange.startMinutes = startMinutes;
            outRange.endMinutes = endMinutes;
            return true;
        }

        bool ReadPositiveInt(HWND hDlg, int controlId, const wchar_t* messageKey, int& outValue)
        {
            BOOL ok = FALSE;
            UINT value = ::GetDlgItemInt(hDlg, controlId, &ok, FALSE);
            if (!ok || value == 0)
            {
                ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
                std::wstring msg = rm ? rm->GetString(messageKey) : std::wstring(messageKey);
                std::wstring title = rm ? rm->GetString(L"Invalid value") : L"Invalid value";
                ::MessageBoxW(hDlg, msg.c_str(), title.c_str(), MB_ICONWARNING | MB_OK);
                ::SetFocus(::GetDlgItem(hDlg, controlId));
                return false;
            }

            outValue = static_cast<int>(value);
            return true;
        }

        bool UpdateParentPin(HWND hDlg,
                             const config::AppConfig& currentCfg,
                             std::string& outPlainUtf8)
        {
            wchar_t buf[64]{};
            ::GetDlgItemTextW(hDlg, IDC_EDIT_PARENT_PIN, buf, static_cast<int>(std::size(buf)));

            std::wstring pinText = ctg::strutils::Trim(std::wstring(buf));

            if (pinText.empty())
            {
                if (!currentCfg.parentPinPlainUi.empty())
                {
                    // Keep existing PIN when user leaves the field empty.
                    outPlainUtf8 = currentCfg.parentPinPlainUi;
                    return true;
                }

                // No existing PIN: use default PIN "2026".
                pinText = L"2026";
            }

            std::string utf8 = ctg::strutils::Utf16ToUtf8(pinText);
            if (utf8.empty())
                return false;

            outPlainUtf8 = utf8;
            return true;
        }

        void ApplyAutoStartSetting(HWND hDlg, bool wantAutoStart, bool& outFinalAutoStart, HINSTANCE hInstance)
        {
            using ctg::platform::AutoStartManager;

            outFinalAutoStart = wantAutoStart;

            if (wantAutoStart)
            {
                wchar_t exePath[MAX_PATH]{};
                DWORD len = ::GetModuleFileNameW(hInstance, exePath, MAX_PATH);
                if (len == 0 || len >= MAX_PATH)
                {
                    outFinalAutoStart = false;
                }
                else
                {
                    if (!AutoStartManager::EnableAutoStart(L"ChildTimeGuard", exePath))
                    {
                        outFinalAutoStart = false;
                    }
                }

                if (!outFinalAutoStart)
                {
                    // Requirement: on failure, silently uncheck the box.
                    ::CheckDlgButton(hDlg, IDC_CHECK_AUTOSTART, BST_UNCHECKED);
                }
            }
            else
            {
                // Best-effort disable; ignore failure.
                AutoStartManager::DisableAutoStart(L"ChildTimeGuard");
            }
        }

        std::string GetLanguageCodeFromCombo(HWND hDlg)
        {
            HWND hCombo = ::GetDlgItem(hDlg, IDC_COMBO_LANGUAGE);
            if (!hCombo)
                return "auto";

            LRESULT sel = ::SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
            if (sel <= 0)
                return "auto";

            ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
            if (!rm)
                return "auto";
            std::vector<std::wstring> codes = rm->GetAvailableLanguageCodes();
            const size_t idx = static_cast<size_t>(sel - 1);
            if (idx < codes.size())
            {
                std::string s = ctg::strutils::Utf16ToUtf8(codes[idx]);
                return s.empty() ? "auto" : s;
            }
            return "auto";
        }

        INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            ConfigDialogContext* ctx = nullptr;
            if (msg == WM_INITDIALOG)
            {
                ctx = reinterpret_cast<ConfigDialogContext*>(lParam);
                ::SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(ctx));
            }
            else
            {
                ctx = reinterpret_cast<ConfigDialogContext*>(::GetWindowLongPtrW(hDlg, DWLP_USER));
            }

            switch (msg)
            {
            case WM_INITDIALOG:
                {
                    if (!ctx || !ctx->config)
                        return TRUE;

                    SetTopMostAndIcon(hDlg, ctx);

                    ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
                    if (rm)
                    {
                        ::SetWindowTextW(hDlg, rm->GetString(L"Child Time Guard - Config").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_STATIC_USABLE_TIME, rm->GetString(L"Usable time (24h):").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_STATIC_TILDE, rm->GetString(L"~").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_STATIC_MAX_CONT, rm->GetString(L"Continuous use (min):").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_STATIC_WARN, rm->GetString(L"Countdown Reminder (min):").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_STATIC_REST, rm->GetString(L"Rest Screen Lock (min):").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_STATIC_PARENT_PIN, rm->GetString(L"Parent PIN:").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_STATIC_AUTOSTART, rm->GetString(L"Auto start:").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_STATIC_LANGUAGE, rm->GetString(L"Language:").c_str());
                        ::SetDlgItemTextW(hDlg, IDCANCEL, rm->GetString(L"Cancel").c_str());
                        ::SetDlgItemTextW(hDlg, IDOK, rm->GetString(L"OK").c_str());
                    }

                    const config::AppConfig& cfg = *ctx->config;

                    // Time range.
                    std::wstring start = FormatTimeHM(cfg.usableTime.startMinutes);
                    std::wstring end = FormatTimeHM(cfg.usableTime.endMinutes);
                    if (start.empty())
                        start = L"08:00";
                    if (end.empty())
                        end = L"21:00";
                    ::SetDlgItemTextW(hDlg, IDC_EDIT_START_TIME, start.c_str());
                    ::SetDlgItemTextW(hDlg, IDC_EDIT_END_TIME, end.c_str());

                    // Numeric fields.
                    ::SetDlgItemInt(hDlg, IDC_EDIT_MAX_CONT, static_cast<UINT>(cfg.maxContinuousMinutes), FALSE);
                    ::SetDlgItemInt(hDlg, IDC_EDIT_WARN_MIN, static_cast<UINT>(cfg.warnMinutes), FALSE);
                    ::SetDlgItemInt(hDlg, IDC_EDIT_REST_MIN, static_cast<UINT>(cfg.restMinutes), FALSE);

                    // Parent PIN: show plain text from config if valid; otherwise fall back to default "2026".
                    std::wstring pinUi;
                    if (!cfg.parentPinPlainUi.empty())
                    {
                        std::wstring w = ctg::strutils::Utf8ToUtf16(cfg.parentPinPlainUi);
                        bool allDigits = !w.empty();
                        for (wchar_t ch : w)
                        {
                            if (ch < L'0' || ch > L'9')
                            {
                                allDigits = false;
                                break;
                            }
                        }
                        if (allDigits)
                        {
                            pinUi = w;
                        }
                    }
                    if (pinUi.empty())
                    {
                        pinUi = L"2026";
                    }
                    ::SetDlgItemTextW(hDlg, IDC_EDIT_PARENT_PIN, pinUi.c_str());

                    // Language combo and auto-start checkbox.
                    InitLanguageCombo(hDlg, cfg);
                    ::CheckDlgButton(
                        hDlg,
                        IDC_CHECK_AUTOSTART,
                        cfg.autoStart ? BST_CHECKED : BST_UNCHECKED);

                    // First-run mode: disable Cancel and remove title-bar Close (X).
                    if (ctx->firstRun)
                    {
                        ::EnableWindow(::GetDlgItem(hDlg, IDCANCEL), FALSE);
                        HMENU hMenu = ::GetSystemMenu(hDlg, FALSE);
                        if (hMenu)
                            ::RemoveMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
                    }
                }
                return TRUE;

            case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                case IDOK:
                    {
                        if (!ctx || !ctx->config)
                        {
                            ::EndDialog(hDlg, IDCANCEL);
                            return TRUE;
                        }

                        config::AppConfig newCfg = *ctx->config;

                        // Time range.
                        if (!ReadTimeRange(hDlg, newCfg.usableTime))
                            return TRUE;

                        // Numeric fields.
                        if (!ReadPositiveInt(hDlg, IDC_EDIT_MAX_CONT, L"Continuous minutes must be a positive integer.", newCfg.maxContinuousMinutes))
                            return TRUE;
                        if (!ReadPositiveInt(hDlg, IDC_EDIT_WARN_MIN, L"Countdown minutes must be a positive integer.", newCfg.warnMinutes))
                            return TRUE;
                        if (!ReadPositiveInt(hDlg, IDC_EDIT_REST_MIN, L"Rest screen lock minutes must be a positive integer.", newCfg.restMinutes))
                            return TRUE;

                        if (newCfg.warnMinutes >= newCfg.maxContinuousMinutes)
                        {
                            ctg::i18n::ResourceManager* rmVal = ctg::i18n::GetResourceManager();
                            std::wstring msg = rmVal ? rmVal->GetString(L"The countdown value must be less than the continuous value.") : L"The countdown value must be less than the continuous value.";
                            std::wstring titleVal = rmVal ? rmVal->GetString(L"Invalid value") : L"Invalid value";
                            ::MessageBoxW(hDlg, msg.c_str(), titleVal.c_str(), MB_ICONWARNING | MB_OK);
                            ::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_WARN_MIN));
                            return TRUE;
                        }

                        // Parent PIN: plain text for UI.
                        std::string pinPlainUtf8;
                        if (!UpdateParentPin(hDlg, *ctx->config, pinPlainUtf8))
                        {
                            ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
                            std::wstring msg = rm ? rm->GetString(L"Failed to update parent PIN, Please try again.") : L"Failed to update parent PIN, Please try again.";
                            std::wstring title = rm ? rm->GetString(L"Error") : L"Error";
                            ::MessageBoxW(hDlg, msg.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
                            ::SetFocus(::GetDlgItem(hDlg, IDC_EDIT_PARENT_PIN));
                            return TRUE;
                        }
                        newCfg.parentPinPlainUi = pinPlainUtf8;

                        // Language.
                        newCfg.languageCode = GetLanguageCodeFromCombo(hDlg);

                        // Auto-start: apply to registry and update config flag.
                        BOOL checked = (::IsDlgButtonChecked(hDlg, IDC_CHECK_AUTOSTART) == BST_CHECKED);
                        bool finalAutoStart = false;
                        ApplyAutoStartSetting(hDlg, checked != FALSE, finalAutoStart, ctx->hInstance);
                        newCfg.autoStart = finalAutoStart;

                        *ctx->config = newCfg;

                        ::EndDialog(hDlg, IDOK);
                    }
                    return TRUE;

                case IDCANCEL:
                    ::EndDialog(hDlg, IDCANCEL);
                    return TRUE;

                case IDC_BTN_AVAILABLE_TIME_HELP:
                    {
                        ctg::i18n::ResourceManager* rmTip = ctg::i18n::GetResourceManager();
                        std::wstring tip = rmTip ? rmTip->GetString(
                            L"During unavailable times, a forced screen lock countdown will occur; before the countdown ends, the application can be manually exited, and after the countdown ends, the system will automatically shut down.")
                            : L"During unavailable times, a forced screen lock countdown will occur; before the countdown ends, the application can be manually exited, and after the countdown ends, the system will automatically shut down.";
                        std::wstring title = rmTip ? rmTip->GetString(L"Child Time Guard") : L"Child Time Guard";
                        ::MessageBoxW(hDlg, tip.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
                    }
                    return TRUE;
                }
                break;

            case WM_CTLCOLORSTATIC:
                {
                    HDC hdc = reinterpret_cast<HDC>(wParam);
                    UINT id = static_cast<UINT>(::GetDlgCtrlID(reinterpret_cast<HWND>(lParam)));
                    ::SetBkMode(hdc, TRANSPARENT);
                    if (id == IDC_STATIC_USABLE_TIME)
                    {
                        ::SetTextColor(hdc, RGB(255, 0, 0));
                        return reinterpret_cast<INT_PTR>(::GetStockObject(NULL_BRUSH));
                    }
                    if (id == IDC_STATIC_MAX_CONT || id == IDC_STATIC_REST)
                    {
                        ::SetTextColor(hdc, RGB(0, 0, 255));
                        return reinterpret_cast<INT_PTR>(::GetStockObject(NULL_BRUSH));
                    }
                }
                break;

            case WM_DESTROY:
                if (ctx && ctx->hIcon)
                {
                    ::DestroyIcon(ctx->hIcon);
                    ctx->hIcon = nullptr;
                }
                break;
            }

            return FALSE;
        }

    } // namespace

    bool ConfigDialog::ShowModal(HINSTANCE hInstance,
                                 HWND parent,
                                 config::AppConfig& inOutConfig,
                                 bool firstRun)
    {
        ConfigDialogContext ctx{};
        ctx.hInstance = hInstance;
        ctx.config = &inOutConfig;
        ctx.firstRun = firstRun;

        INT_PTR ret = ::DialogBoxParamW(
            hInstance,
            MAKEINTRESOURCEW(IDD_CONFIG_DIALOG),
            parent,
            &ConfigDlgProc,
            reinterpret_cast<LPARAM>(&ctx));

        return (ret == IDOK);
    }

} // namespace ui
} // namespace ctg

