#include "ReminderDialog.h"

#include <cwchar>
#include <string>

#include "resource.h"
#include "TimeUtils.h"
#include "ResourceManager.h"

namespace ctg {
namespace ui {

    namespace {

        constexpr UINT_PTR kTimerIdCountdown = 1;
        constexpr UINT kTimerIntervalMs = 1000;

        struct ReminderState
        {
            HINSTANCE hInstance{ nullptr };
            HICON hIcon{ nullptr };
            int usedMinutes{ 0 };
            int totalSeconds{ 0 };
            int remainingSeconds{ 0 };
        };

        void UpdateCountdownText(HWND hDlg, int remainingSeconds)
        {
            std::wstring text = ctg::timeutils::FormatSecondsToMMSS(remainingSeconds);
            ::SetDlgItemTextW(hDlg, IDC_REMINDER_COUNTDOWN, text.c_str());
        }

        INT_PTR CALLBACK ReminderDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            ReminderState* state = reinterpret_cast<ReminderState*>(::GetWindowLongPtrW(hDlg, DWLP_USER));

            switch (msg)
            {
            case WM_INITDIALOG:
                {
                    state = reinterpret_cast<ReminderState*>(lParam);
                    if (!state)
                        return TRUE;
                    ::SetWindowLongPtrW(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(state));

                    ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
                    std::wstring formatStr = rm ? rm->GetString(L"Children, the break starts in %d minutes.") : L"Children, the break starts in %d minutes.";
                    wchar_t buf[256]{};
                    int warnMinutes = state->totalSeconds / 60;
                    swprintf_s(buf, 256, formatStr.c_str(), warnMinutes);
                    ::SetDlgItemTextW(hDlg, IDC_REMINDER_MESSAGE, buf);

                    if (rm)
                    {
                        ::SetWindowTextW(hDlg, rm->GetString(L"Child Time Guard").c_str());
                        ::SetDlgItemTextW(hDlg, IDOK, rm->GetString(L"I got it").c_str());
                    }

                    state->remainingSeconds = state->totalSeconds;
                    UpdateCountdownText(hDlg, state->remainingSeconds);

                    HICON hIcon = reinterpret_cast<HICON>(::LoadImageW(
                        state->hInstance,
                        MAKEINTRESOURCEW(IDI_APP_ICON),
                        IMAGE_ICON,
                        0, 0,
                        LR_DEFAULTCOLOR));
                    if (hIcon)
                    {
                        state->hIcon = hIcon;
                        ::SendMessageW(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
                        ::SendMessageW(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
                    }

                    ::SetTimer(hDlg, kTimerIdCountdown, kTimerIntervalMs, nullptr);
                    ::SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
                    ::SetForegroundWindow(hDlg);
                }
                return TRUE;

            case WM_TIMER:
                if (wParam == kTimerIdCountdown && state)
                {
                    state->remainingSeconds--;
                    if (state->remainingSeconds < 0)
                        state->remainingSeconds = 0;
                    UpdateCountdownText(hDlg, state->remainingSeconds);
                }
                return TRUE;

            case WM_DESTROY:
                ::KillTimer(hDlg, kTimerIdCountdown);
                if (state && state->hIcon)
                {
                    ::DestroyIcon(state->hIcon);
                    state->hIcon = nullptr;
                }
                return TRUE;

            case WM_COMMAND:
                if (LOWORD(wParam) == IDOK)
                {
                    ::KillTimer(hDlg, kTimerIdCountdown);
                    ::EndDialog(hDlg, IDOK);
                    return TRUE;
                }
                break;
            }

            return FALSE;
        }

    } // namespace

    void ReminderDialog::ShowOnce(HINSTANCE hInstance, HWND parent, int usedMinutes, int totalSeconds)
    {
        ReminderState state;
        state.hInstance = hInstance;
        state.usedMinutes = usedMinutes;
        state.totalSeconds = totalSeconds;

        ::DialogBoxParamW(
            hInstance,
            MAKEINTRESOURCEW(IDD_REMINDER_DIALOG),
            parent,
            ReminderDlgProc,
            reinterpret_cast<LPARAM>(&state));
    }

} // namespace ui
} // namespace ctg
