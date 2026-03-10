#include "ControlDialog.h"

#include "resource.h"
#include "StringUtils.h"
#include "ResourceManager.h"

#include <string>

namespace ctg {
namespace ui {

    namespace {

        // Guard to prevent opening multiple control dialogs at the same time.
        static bool g_ctrlDialogOpen = false;

        struct CtrlDialogContext
        {
            HINSTANCE hInstance{ nullptr };
            ctg::config::ConfigManager* configManager{ nullptr };
            ctg::core::TimeController* timeController{ nullptr };
            ControlDialogResult* outResult{ nullptr };
            HICON hIcon{ nullptr };
            int failedCount{ 0 };
        };

        // Verify entered password against the configured parent PIN.
        // When no PIN is configured, the default PIN is "2026".
        bool VerifyPassword(HWND hDlg, const std::string& configuredPinUtf8)
        {
            wchar_t buf[64]{};
            if (::GetDlgItemTextW(hDlg, IDC_EDIT_PASSWORD, buf, 64) <= 0)
                return false;

            std::wstring wstr(buf);
            std::string enteredUtf8 = ctg::strutils::Utf16ToUtf8(wstr);

            std::string effectivePin = configuredPinUtf8;
            if (effectivePin.empty())
            {
                effectivePin = "2026";
            }

            return !enteredUtf8.empty() && enteredUtf8 == effectivePin;
        }

        INT_PTR CALLBACK CtrlDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            CtrlDialogContext* ctx = nullptr;
            if (msg == WM_INITDIALOG)
            {
                ctx = reinterpret_cast<CtrlDialogContext*>(lParam);
                ::SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(ctx));
            }
            else
            {
                ctx = reinterpret_cast<CtrlDialogContext*>(::GetWindowLongPtrW(hDlg, DWLP_USER));
            }

            switch (msg)
            {
            case WM_INITDIALOG:
                {
                    if (!ctx || !ctx->outResult)
                        return TRUE;

                    ctx->outResult->action = ControlDialogAction::Cancel;
                    ctx->outResult->tempDisableMinutes = 0;

                    // Make dialog top-most and bring to front so it's not covered.
                    ::SetWindowPos(
                        hDlg,
                        HWND_TOPMOST,
                        0,
                        0,
                        0,
                        0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
                    ::SetForegroundWindow(hDlg);

                    ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
                    if (rm)
                    {
                        ::SetWindowTextW(hDlg, rm->GetString(L"Child Time Guard").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_RADIO_CONFIG, rm->GetString(L"Modify config").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_RADIO_SUSPEND, rm->GetString(L"Suspend (min):").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_RADIO_EXIT, rm->GetString(L"Exit app").c_str());
                        ::SetDlgItemTextW(hDlg, IDC_CTRL_PWD_LABEL, rm->GetString(L"Parent PIN:").c_str());
                        ::SetDlgItemTextW(hDlg, IDCANCEL, rm->GetString(L"Cancel").c_str());
                        ::SetDlgItemTextW(hDlg, IDOK, rm->GetString(L"OK").c_str());
                    }

                    // Load app icon from embedded resources and set as dialog icon.
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

                    bool tempActive = ctx->timeController && ctx->timeController->IsTemporarySuspendActive();

                    if (tempActive)
                    {
                        ::CheckRadioButton(hDlg, IDC_RADIO_CONFIG, IDC_RADIO_EXIT, IDC_RADIO_SUSPEND);
                        std::uint32_t remSec = ctx->timeController->GetTemporarySuspendRemainingSeconds();
                        int remMin = static_cast<int>((remSec + 29) / 60);
                        if (remMin < 0)
                            remMin = 0;
                        wchar_t buf[16]{};
                        swprintf_s(buf, L"%d", remMin);
                        ::SetDlgItemTextW(hDlg, IDC_EDIT_Y, buf);
                    }
                    else
                    {
                        ::CheckRadioButton(hDlg, IDC_RADIO_CONFIG, IDC_RADIO_EXIT, IDC_RADIO_CONFIG);
                        ::SetDlgItemTextW(hDlg, IDC_EDIT_Y, L"120");
                    }

                    // Configure Y-edit: max 3 digits, always editable.
                    HWND hEditY = ::GetDlgItem(hDlg, IDC_EDIT_Y);
                    if (hEditY)
                    {
                        ::SendMessageW(hEditY, EM_SETLIMITTEXT, 3, 0);
                        ::SendMessageW(hEditY, EM_SETREADONLY, FALSE, 0);
                    }

                    // Clear and hide password error text initially.
                    HWND hErr = ::GetDlgItem(hDlg, IDC_STATIC_PWD_ERROR);
                    if (hErr)
                    {
                        ::SetWindowTextW(hErr, L"");
                        ::ShowWindow(hErr, SW_HIDE);
                    }
                }
                return TRUE;

            case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                case IDC_RADIO_CONFIG:
                case IDC_RADIO_SUSPEND:
                case IDC_RADIO_EXIT:
                    // Ensure the three radio buttons are always mutually exclusive.
                    ::CheckRadioButton(
                        hDlg,
                        IDC_RADIO_CONFIG,
                        IDC_RADIO_EXIT,
                        LOWORD(wParam));
                    return TRUE;

                default:
                    // Hide error message when user starts typing again in the password box.
                    if (LOWORD(wParam) == IDC_EDIT_PASSWORD && HIWORD(wParam) == EN_CHANGE)
                    {
                        HWND hErr = ::GetDlgItem(hDlg, IDC_STATIC_PWD_ERROR);
                        if (hErr)
                        {
                            ::SetDlgItemTextW(hDlg, IDC_STATIC_PWD_ERROR, L"");
                            ::ShowWindow(hErr, SW_HIDE);
                        }
                    }
                    break;

                case IDOK:
                    {
                        if (!ctx || !ctx->outResult)
                        {
                            ::EndDialog(hDlg, IDCANCEL);
                            return TRUE;
                        }

                        const std::string& configuredPin = ctx->configManager
                            ? ctx->configManager->Get().parentPinPlainUi
                            : std::string();
                        if (!VerifyPassword(hDlg, configuredPin))
                        {
                            // Password error: clear input, show inline red error text, and
                            // after 3 consecutive failures close the dialog.
                            ::SetDlgItemTextW(hDlg, IDC_EDIT_PASSWORD, L"");
                            HWND hErr = ::GetDlgItem(hDlg, IDC_STATIC_PWD_ERROR);
                            if (hErr)
                            {
                                ctg::i18n::ResourceManager* errRm = ctg::i18n::GetResourceManager();
                                std::wstring errMsg = errRm ? errRm->GetString(L"Wrong password, Please try again.") : L"\u5bc6\u7801\u9519\u8bef\uff0c\u8bf7\u91cd\u65b0\u8f93\u5165\u3002";
                                ::SetDlgItemTextW(hDlg, IDC_STATIC_PWD_ERROR, errMsg.c_str());
                                ::ShowWindow(hErr, SW_SHOWNORMAL);
                            }

                            ++ctx->failedCount;
                            if (ctx->failedCount >= 3)
                            {
                                ::EndDialog(hDlg, IDCANCEL);
                            }
                            return TRUE;
                        }

                        int action = 0;
                        if (::IsDlgButtonChecked(hDlg, IDC_RADIO_CONFIG) == BST_CHECKED)
                            action = 1;
                        else if (::IsDlgButtonChecked(hDlg, IDC_RADIO_SUSPEND) == BST_CHECKED)
                            action = 2;
                        else if (::IsDlgButtonChecked(hDlg, IDC_RADIO_EXIT) == BST_CHECKED)
                            action = 3;

                        int minutesY = 120;
                        if (action == 2)
                        {
                            BOOL ok = FALSE;
                            minutesY = ::GetDlgItemInt(hDlg, IDC_EDIT_Y, &ok, FALSE);
                            if (!ok || minutesY <= 0)
                                minutesY = 1;
                            if (minutesY > 999)
                                minutesY = 999;
                        }

                        ctx->outResult->tempDisableMinutes = (action == 2) ? minutesY : 0;
                        if (action == 1)
                            ctx->outResult->action = ControlDialogAction::OpenConfig;
                        else if (action == 2)
                            ctx->outResult->action = ControlDialogAction::TempDisable;
                        else if (action == 3)
                            ctx->outResult->action = ControlDialogAction::ExitApp;
                        else
                            ctx->outResult->action = ControlDialogAction::OpenConfig;

                        ::EndDialog(hDlg, IDOK);
                    }
                    return TRUE;

                case IDCANCEL:
                    if (ctx && ctx->outResult)
                        ctx->outResult->action = ControlDialogAction::Cancel;
                    ::EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }
                break;

            default:
                if (msg == WM_CTLCOLORSTATIC)
                {
                    HDC hdc = reinterpret_cast<HDC>(wParam);
                    HWND hCtrl = reinterpret_cast<HWND>(lParam);
                    HWND hErr = ::GetDlgItem(hDlg, IDC_STATIC_PWD_ERROR);
                    if (hErr && hCtrl == hErr)
                    {
                        ::SetTextColor(hdc, RGB(200, 0, 0));
                        ::SetBkMode(hdc, TRANSPARENT);
                        return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_3DFACE));
                    }
                }
                if (msg == WM_DESTROY)
                {
                    if (ctx && ctx->hIcon)
                    {
                        ::DestroyIcon(ctx->hIcon);
                        ctx->hIcon = nullptr;
                    }
                }
                break;
            }

            return FALSE;
        }

    } // namespace

    bool ControlDialog::ShowModal(
        HINSTANCE hInstance,
        HWND parent,
        ctg::config::ConfigManager* configManager,
        ctg::core::TimeController* timeController,
        ControlDialogResult* outResult)
    {
        // Prevent re-entrancy: if dialog is already open, ignore this request.
        if (g_ctrlDialogOpen)
        {
            return false;
        }
        g_ctrlDialogOpen = true;

        ControlDialogResult localResult;
        if (!outResult)
            outResult = &localResult;

        outResult->action = ControlDialogAction::Cancel;
        outResult->tempDisableMinutes = 0;

        CtrlDialogContext ctx{};
        ctx.hInstance = hInstance;
        ctx.configManager = configManager;
        ctx.timeController = timeController;
        ctx.outResult = outResult;

        INT_PTR ret = ::DialogBoxParamW(
            hInstance,
            MAKEINTRESOURCEW(IDD_CTRL_DIALOG),
            parent,
            &CtrlDlgProc,
            reinterpret_cast<LPARAM>(&ctx));

        g_ctrlDialogOpen = false;

        return (ret == IDOK && outResult->action != ControlDialogAction::Cancel);
    }

} // namespace ui
} // namespace ctg
