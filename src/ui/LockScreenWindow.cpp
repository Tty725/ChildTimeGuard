#include "LockScreenWindow.h"

#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include "AppMessages.h"
#include "ShutdownController.h"
#include "TimeUtils.h"
#include "resource.h"
#include "ResourceManager.h"

namespace ctg {
namespace ui {

    namespace {

        struct RestLockSession
        {
            int restMinutes{ 0 };
            int usedMinutesForLine1{ 0 };
            int warnMinutesForLine1{ 0 };
            int remainingSeconds{ 0 };
            HWND parentToNotify{ nullptr };
            std::vector<HWND> windows;
            HWND primaryHwnd{ nullptr };
            HICON hIcon128{ nullptr };

            static constexpr int kBubbleCount = 12;
            float bubbleX[kBubbleCount]{};
            float bubbleY[kBubbleCount]{};
            float bubbleVX[kBubbleCount]{};
            float bubbleVY[kBubbleCount]{};
            COLORREF bubbleColor[kBubbleCount]{};
            bool bubblesInitialized{ false };
        };

        constexpr UINT_PTR kTimerIdRest = 1;
        constexpr UINT_PTR kTimerIdAnim = 2;
        constexpr UINT kTimerIntervalMs = 1000;
        constexpr UINT kAnimIntervalMs = 120;

        RestLockSession g_restLockSession;

        void InitBubbles(RestLockSession* session)
        {
            if (!session || session->bubblesInitialized)
                return;
            for (int i = 0; i < RestLockSession::kBubbleCount; ++i)
            {
                session->bubbleX[i] = static_cast<float>(rand() % 1000) / 1000.0f;
                session->bubbleY[i] = static_cast<float>(rand() % 1000) / 1000.0f;
                float angle = static_cast<float>(rand() % 360) * 3.14159265f / 180.0f;
                float speed = 0.008f + static_cast<float>(rand() % 8) / 1000.0f;
                session->bubbleVX[i] = std::cos(angle) * speed;
                session->bubbleVY[i] = std::sin(angle) * speed;
                int r = 200 + rand() % 56, g = 200 + rand() % 56, b = 200 + rand() % 56;
                switch (rand() % 6)
                {
                case 0: r = 255; break;
                case 1: g = 255; break;
                case 2: b = 255; break;
                case 3: r = 255; g = 255; break;
                case 4: r = 255; b = 255; break;
                default: g = 255; b = 255; break;
                }
                session->bubbleColor[i] = RGB(r, g, b);
            }
            session->bubblesInitialized = true;
        }

        void UpdateBubbles(RestLockSession* session)
        {
            if (!session)
                return;
            for (int i = 0; i < RestLockSession::kBubbleCount; ++i)
            {
                session->bubbleX[i] += session->bubbleVX[i];
                session->bubbleY[i] += session->bubbleVY[i];
                if (session->bubbleX[i] < 0.0f) session->bubbleX[i] += 1.0f;
                if (session->bubbleX[i] > 1.0f) session->bubbleX[i] -= 1.0f;
                if (session->bubbleY[i] < 0.0f) session->bubbleY[i] += 1.0f;
                if (session->bubbleY[i] > 1.0f) session->bubbleY[i] -= 1.0f;
            }
        }

        void DrawBubbles(HDC hdc, const RECT& rc, RestLockSession* session)
        {
            if (!session)
                return;
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            const int radius = 90;
            for (int i = 0; i < RestLockSession::kBubbleCount; ++i)
            {
                int cx = static_cast<int>(session->bubbleX[i] * w);
                int cy = static_cast<int>(session->bubbleY[i] * h);
                HBRUSH br = ::CreateSolidBrush(session->bubbleColor[i]);
                HGDIOBJ oldBr = ::SelectObject(hdc, br);
                HPEN pen = ::CreatePen(PS_NULL, 0, session->bubbleColor[i]);
                HGDIOBJ oldPen = ::SelectObject(hdc, pen);
                ::Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
                ::SelectObject(hdc, oldPen);
                ::SelectObject(hdc, oldBr);
                ::DeleteObject(pen);
                ::DeleteObject(br);
            }
        }

        RestLockSession* GetSession(HWND hwnd)
        {
            return reinterpret_cast<RestLockSession*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        void PaintRestLock(HWND hwnd, RestLockSession* session)
        {
            if (!session)
                return;

            InitBubbles(session);

            PAINTSTRUCT ps{};
            HDC hdc = ::BeginPaint(hwnd, &ps);
            if (!hdc)
                return;

            RECT rc;
            ::GetClientRect(hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            if (w <= 0 || h <= 0)
            {
                ::EndPaint(hwnd, &ps);
                return;
            }

            HDC memDC = ::CreateCompatibleDC(hdc);
            if (!memDC)
            {
                ::EndPaint(hwnd, &ps);
                return;
            }
            HBITMAP bmp = ::CreateCompatibleBitmap(hdc, w, h);
            if (!bmp)
            {
                ::DeleteDC(memDC);
                ::EndPaint(hwnd, &ps);
                return;
            }
            HGDIOBJ oldBmp = ::SelectObject(memDC, bmp);

            ::FillRect(memDC, &rc, reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
            DrawBubbles(memDC, rc, session);

            ::SetBkMode(memDC, TRANSPARENT);
            ::SetTextColor(memDC, RGB(0, 0, 0));

            ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
            std::wstring line1Str = rm ? rm->GetString(L"Children, go and see the scenery in the distance, or read aloud your favorite books or stories.") : L"Children, go and see the scenery in the distance, or read aloud your favorite books or stories.";
            std::wstring line2Str = rm ? rm->GetString(L"Press Ctrl+Shift+K to choose to close.") : L"Press Ctrl+Shift+K to choose to close.";
            const wchar_t* line1 = line1Str.c_str();
            const wchar_t* line2Text = line2Str.c_str();

            HFONT fontLarge = ::CreateFontW(
                -MulDiv(24, GetDeviceCaps(memDC, LOGPIXELSY), 72),
                0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            HGDIOBJ oldFont = ::SelectObject(memDC, fontLarge);

            RECT rcLine1 = rc;
            ::DrawTextW(memDC, line1, -1, &rcLine1, DT_CENTER | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
            int line1Height = rcLine1.bottom - rcLine1.top;

            HFONT fontSmall = ::CreateFontW(
                -MulDiv(12, GetDeviceCaps(memDC, LOGPIXELSY), 72),
                0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            ::SelectObject(memDC, fontSmall);
            RECT rcLine2 = rc;
            ::DrawTextW(memDC, line2Text, -1, &rcLine2, DT_CENTER | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
            int line2Height = rcLine2.bottom - rcLine2.top;

            std::wstring countdown = ctg::timeutils::FormatSecondsToMMSS(session->remainingSeconds);
            HFONT fontMedium = ::CreateFontW(
                -MulDiv(16, GetDeviceCaps(memDC, LOGPIXELSY), 72),
                0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            ::SelectObject(memDC, fontMedium);
            RECT rcLine3 = rc;
            ::DrawTextW(memDC, countdown.c_str(), -1, &rcLine3, DT_CENTER | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
            int line3Height = rcLine3.bottom - rcLine3.top;

            ::SelectObject(memDC, oldFont);
            ::DeleteObject(fontLarge);
            ::DeleteObject(fontSmall);
            ::DeleteObject(fontMedium);

            const int gap = 8;
            const int iconSize = 128;
            int totalBlockHeight = line1Height + gap + line2Height + gap + line3Height + gap + iconSize;
            int startY = (h - totalBlockHeight) / 2;

            fontLarge = ::CreateFontW(
                -MulDiv(24, GetDeviceCaps(memDC, LOGPIXELSY), 72),
                0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            ::SelectObject(memDC, fontLarge);
            RECT r1 = { 0, startY, w, startY + line1Height };
            ::DrawTextW(memDC, line1, -1, &r1, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
            ::SelectObject(memDC, oldFont);
            ::DeleteObject(fontLarge);

            fontSmall = ::CreateFontW(
                -MulDiv(12, GetDeviceCaps(memDC, LOGPIXELSY), 72),
                0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            ::SelectObject(memDC, fontSmall);
            RECT r2 = { 0, startY + line1Height + gap, w, startY + line1Height + gap + line2Height };
            ::DrawTextW(memDC, line2Text, -1, &r2, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
            ::SelectObject(memDC, oldFont);
            ::DeleteObject(fontSmall);

            fontMedium = ::CreateFontW(
                -MulDiv(16, GetDeviceCaps(memDC, LOGPIXELSY), 72),
                0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            ::SelectObject(memDC, fontMedium);
            RECT r3 = { 0, startY + line1Height + gap + line2Height + gap, w, startY + line1Height + gap + line2Height + gap + line3Height };
            ::DrawTextW(memDC, countdown.c_str(), -1, &r3, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
            ::SelectObject(memDC, oldFont);
            ::DeleteObject(fontMedium);

            if (session->hIcon128)
            {
                int iconX = (w - iconSize) / 2;
                int iconY = startY + line1Height + gap + line2Height + gap + line3Height + gap;
                ::DrawIconEx(memDC, iconX, iconY, session->hIcon128, iconSize, iconSize, 0, nullptr, DI_NORMAL);
            }

            ::BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

            ::SelectObject(memDC, oldBmp);
            ::DeleteObject(bmp);
            ::DeleteDC(memDC);
            ::EndPaint(hwnd, &ps);
        }

        void OnRestLockTimer(RestLockSession* session)
        {
            if (!session || session->windows.empty())
                return;

            session->remainingSeconds--;
            if (session->remainingSeconds < 0)
                session->remainingSeconds = 0;

            for (HWND h : session->windows)
            {
                if (::IsWindow(h))
                    ::InvalidateRect(h, nullptr, FALSE);
            }

            if (session->remainingSeconds <= 0)
            {
                HWND parent = session->parentToNotify;
                UINT msg = WM_CTGUARD_REST_LOCK_FINISHED;

                for (HWND h : session->windows)
                {
                    if (::IsWindow(h))
                        ::DestroyWindow(h);
                }
                session->windows.clear();
                session->primaryHwnd = nullptr;

                if (parent)
                    ::PostMessageW(parent, msg, 0, 0);
            }
        }

        void OnAnimTimer(RestLockSession* session)
        {
            if (!session || session->windows.empty())
                return;
            UpdateBubbles(session);
            for (HWND h : session->windows)
            {
                if (::IsWindow(h))
                    ::InvalidateRect(h, nullptr, FALSE);
            }
        }

        BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM lParam)
        {
            auto* session = reinterpret_cast<RestLockSession*>(lParam);
            if (!session)
                return FALSE;

            HWND parent = session->parentToNotify;
            HINSTANCE hInst = reinterpret_cast<HINSTANCE>(::GetClassLongPtrW(parent, GCLP_HMODULE));
            if (!hInst)
                hInst = ::GetModuleHandleW(nullptr);

            HWND hwnd = ::CreateWindowExW(
                WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                L"CTG_RestLockWindow",
                nullptr,
                WS_POPUP | WS_VISIBLE,
                lprcMonitor->left,
                lprcMonitor->top,
                lprcMonitor->right - lprcMonitor->left,
                lprcMonitor->bottom - lprcMonitor->top,
                parent,
                nullptr,
                hInst,
                session);

            if (hwnd)
            {
                session->windows.push_back(hwnd);
                if (!session->primaryHwnd)
                    session->primaryHwnd = hwnd;
            }

            return TRUE;
        }

        LRESULT CALLBACK RestLockWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            RestLockSession* session = GetSession(hwnd);

            if (msg == WM_CREATE)
            {
                auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                session = reinterpret_cast<RestLockSession*>(cs->lpCreateParams);
                ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(session));
                if (session && !session->hIcon128)
                {
                    HINSTANCE hInst = reinterpret_cast<HINSTANCE>(::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                    if (hInst)
                    {
                        session->hIcon128 = reinterpret_cast<HICON>(::LoadImageW(
                            hInst,
                            MAKEINTRESOURCEW(IDI_APP_ICON),
                            IMAGE_ICON,
                            128, 128,
                            LR_DEFAULTCOLOR));
                    }
                }
                return 0;
            }

            session = GetSession(hwnd);

            switch (msg)
            {
            case WM_PAINT:
                PaintRestLock(hwnd, session);
                return 0;

            case WM_TIMER:
                if (session && hwnd == session->primaryHwnd)
                {
                    if (wParam == kTimerIdRest)
                        OnRestLockTimer(session);
                    else if (wParam == kTimerIdAnim)
                        OnAnimTimer(session);
                }
                return 0;

            case WM_DESTROY:
                if (session && hwnd == session->primaryHwnd)
                {
                    ::KillTimer(hwnd, kTimerIdRest);
                    ::KillTimer(hwnd, kTimerIdAnim);
                    if (session->hIcon128)
                    {
                        ::DestroyIcon(session->hIcon128);
                        session->hIcon128 = nullptr;
                    }
                }
                return 0;

            case WM_ERASEBKGND:
                {
                    RECT rc;
                    ::GetClientRect(hwnd, &rc);
                    ::FillRect(reinterpret_cast<HDC>(wParam), &rc, reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
                }
                return 1;
            }

            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        bool RegisterRestLockClass(HINSTANCE hInst)
        {
            WNDCLASSW wc{};
            wc.lpfnWndProc = RestLockWndProc;
            wc.hInstance = hInst;
            wc.lpszClassName = L"CTG_RestLockWindow";
            wc.hCursor = ::LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));
            wc.hbrBackground = reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));
            return ::RegisterClassW(&wc) != 0;
        }

        // ----- Unavailable-time (shutdown) lock -----

        struct UnavailableLockSession
        {
            int totalSeconds{ 0 };
            int totalMinutes{ 0 };
            int remainingSeconds{ 0 };
            HWND parentToNotify{ nullptr };
            bool countdownEnded{ false };
            bool shutdownFailed{ false };
            std::vector<HWND> windows;
            HWND primaryHwnd{ nullptr };
            HICON hIcon128{ nullptr };

            static constexpr int kBubbleCount = 12;
            float bubbleX[kBubbleCount]{};
            float bubbleY[kBubbleCount]{};
            float bubbleVX[kBubbleCount]{};
            float bubbleVY[kBubbleCount]{};
            COLORREF bubbleColor[kBubbleCount]{};
            bool bubblesInitialized{ false };
        };

        constexpr UINT_PTR kTimerIdUnavailable = 10;
        constexpr UINT_PTR kTimerIdUnavailableAnim = 11;
        constexpr UINT kUnavailableTimerIntervalMs = 1000;
        constexpr UINT kUnavailableAnimIntervalMs = 120;

        UnavailableLockSession g_unavailableLockSession;

        UnavailableLockSession* GetUnavailableSession(HWND hwnd)
        {
            return reinterpret_cast<UnavailableLockSession*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        void InitBubblesUnavailable(UnavailableLockSession* session)
        {
            if (!session || session->bubblesInitialized)
                return;
            for (int i = 0; i < UnavailableLockSession::kBubbleCount; ++i)
            {
                session->bubbleX[i] = static_cast<float>(rand() % 1000) / 1000.0f;
                session->bubbleY[i] = static_cast<float>(rand() % 1000) / 1000.0f;
                float angle = static_cast<float>(rand() % 360) * 3.14159265f / 180.0f;
                float speed = 0.008f + static_cast<float>(rand() % 8) / 1000.0f;
                session->bubbleVX[i] = std::cos(angle) * speed;
                session->bubbleVY[i] = std::sin(angle) * speed;
                int r = 200 + rand() % 56, g = 200 + rand() % 56, b = 200 + rand() % 56;
                switch (rand() % 6)
                {
                case 0: r = 255; break;
                case 1: g = 255; break;
                case 2: b = 255; break;
                case 3: r = 255; g = 255; break;
                case 4: r = 255; b = 255; break;
                default: g = 255; b = 255; break;
                }
                session->bubbleColor[i] = RGB(r, g, b);
            }
            session->bubblesInitialized = true;
        }

        void UpdateBubblesUnavailable(UnavailableLockSession* session)
        {
            if (!session)
                return;
            for (int i = 0; i < UnavailableLockSession::kBubbleCount; ++i)
            {
                session->bubbleX[i] += session->bubbleVX[i];
                session->bubbleY[i] += session->bubbleVY[i];
                if (session->bubbleX[i] < 0.0f) session->bubbleX[i] += 1.0f;
                if (session->bubbleX[i] > 1.0f) session->bubbleX[i] -= 1.0f;
                if (session->bubbleY[i] < 0.0f) session->bubbleY[i] += 1.0f;
                if (session->bubbleY[i] > 1.0f) session->bubbleY[i] -= 1.0f;
            }
        }

        void DrawBubblesUnavailable(HDC hdc, const RECT& rc, UnavailableLockSession* session)
        {
            if (!session)
                return;
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            const int radius = 90;
            for (int i = 0; i < UnavailableLockSession::kBubbleCount; ++i)
            {
                int cx = static_cast<int>(session->bubbleX[i] * w);
                int cy = static_cast<int>(session->bubbleY[i] * h);
                HBRUSH br = ::CreateSolidBrush(session->bubbleColor[i]);
                HGDIOBJ oldBr = ::SelectObject(hdc, br);
                HPEN pen = ::CreatePen(PS_NULL, 0, session->bubbleColor[i]);
                HGDIOBJ oldPen = ::SelectObject(hdc, pen);
                ::Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
                ::SelectObject(hdc, oldPen);
                ::SelectObject(hdc, oldBr);
                ::DeleteObject(pen);
                ::DeleteObject(br);
            }
        }

        void PaintUnavailableLock(HWND hwnd, UnavailableLockSession* session)
        {
            if (!session)
                return;

            PAINTSTRUCT ps{};
            HDC hdc = ::BeginPaint(hwnd, &ps);
            if (!hdc)
                return;

            RECT rc;
            ::GetClientRect(hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            if (w <= 0 || h <= 0)
            {
                ::EndPaint(hwnd, &ps);
                return;
            }

            HDC memDC = ::CreateCompatibleDC(hdc);
            if (!memDC)
            {
                ::EndPaint(hwnd, &ps);
                return;
            }
            HBITMAP bmp = ::CreateCompatibleBitmap(hdc, w, h);
            if (!bmp)
            {
                ::DeleteDC(memDC);
                ::EndPaint(hwnd, &ps);
                return;
            }
            HGDIOBJ oldBmp = ::SelectObject(memDC, bmp);

            ::FillRect(memDC, &rc, reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

            const int gap = 8;
            int dpi = GetDeviceCaps(memDC, LOGPIXELSY);

            if (session->countdownEnded)
            {
                InitBubblesUnavailable(session);
                DrawBubblesUnavailable(memDC, rc, session);
                ::SetBkMode(memDC, TRANSPARENT);
                ::SetTextColor(memDC, RGB(0, 0, 0));
                ctg::i18n::ResourceManager* uirm = ctg::i18n::GetResourceManager();
                std::wstring line1MsgStr = uirm ? uirm->GetString(L"Children, it's time to sleep; those who go to bed early grow tall.") : L"Children, it's time to sleep; those who go to bed early grow tall.";
                std::wstring line2StrU = uirm ? uirm->GetString(L"Press Ctrl+Shift+K to choose to close.") : L"Press Ctrl+Shift+K to choose to close.";
                std::wstring line3FailStr = uirm ? uirm->GetString(L"Automatic shutdown failed") : L"Automatic shutdown failed";
                const wchar_t* line1Msg = line1MsgStr.c_str();
                const wchar_t* line2Text = line2StrU.c_str();
                const wchar_t* line3Fail = line3FailStr.c_str();

                HFONT fontLarge = ::CreateFontW(
                    -MulDiv(24, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                HGDIOBJ oldFont = ::SelectObject(memDC, fontLarge);
                RECT rcL1 = rc;
                ::DrawTextW(memDC, line1Msg, -1, &rcL1, DT_CENTER | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
                int line1Height = rcL1.bottom - rcL1.top;

                HFONT fontSmall = ::CreateFontW(
                    -MulDiv(12, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                ::SelectObject(memDC, fontSmall);
                RECT rcL2 = rc;
                ::DrawTextW(memDC, line2Text, -1, &rcL2, DT_CENTER | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
                int line2Height = rcL2.bottom - rcL2.top;
                RECT rcL3 = rc;
                ::DrawTextW(memDC, line3Fail, -1, &rcL3, DT_CENTER | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
                int line3Height = session->shutdownFailed ? (rcL3.bottom - rcL3.top) : 0;
                ::SelectObject(memDC, oldFont);
                ::DeleteObject(fontLarge);
                ::DeleteObject(fontSmall);

                int totalH = line1Height + gap + line2Height;
                if (session->shutdownFailed)
                    totalH += gap + line3Height;
                totalH += gap + 128;
                int startY = (h - totalH) / 2;

                fontLarge = ::CreateFontW(
                    -MulDiv(24, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                ::SelectObject(memDC, fontLarge);
                RECT r1 = { 0, startY, w, startY + line1Height };
                ::DrawTextW(memDC, line1Msg, -1, &r1, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
                ::SelectObject(memDC, oldFont);
                ::DeleteObject(fontLarge);

                fontSmall = ::CreateFontW(
                    -MulDiv(12, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                ::SelectObject(memDC, fontSmall);
                RECT r2 = { 0, startY + line1Height + gap, w, startY + line1Height + gap + line2Height };
                ::DrawTextW(memDC, line2Text, -1, &r2, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
                ::SelectObject(memDC, oldFont);
                ::DeleteObject(fontSmall);

                int iconTop = startY + line1Height + gap + line2Height + gap;
                if (session->shutdownFailed)
                {
                    fontSmall = ::CreateFontW(
                        -MulDiv(12, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                    ::SelectObject(memDC, fontSmall);
                    ::SetTextColor(memDC, RGB(255, 0, 0));
                    RECT r3 = { 0, iconTop, w, iconTop + line3Height };
                    ::DrawTextW(memDC, line3Fail, -1, &r3, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
                    ::SetTextColor(memDC, RGB(0, 0, 0));
                    ::SelectObject(memDC, oldFont);
                    ::DeleteObject(fontSmall);
                    iconTop += line3Height + gap;
                }

                if (session->hIcon128)
                {
                    int iconX = (w - 128) / 2;
                    int iconY = iconTop;
                    ::DrawIconEx(memDC, iconX, iconY, session->hIcon128, 128, 128, 0, nullptr, DI_NORMAL);
                }
            }
            else
            {
                ::SetBkMode(memDC, TRANSPARENT);
                ::SetTextColor(memDC, RGB(0, 0, 0));

                ctg::i18n::ResourceManager* uirm = ctg::i18n::GetResourceManager();
                std::wstring formatStr = uirm ? uirm->GetString(L"Children, it's time to sleep. Power off after %d minutes.") : L"Children, it's time to sleep. Power off after %d minutes.";
                std::wstring line2StrU = uirm ? uirm->GetString(L"Press Ctrl+Shift+K to choose to close.") : L"Press Ctrl+Shift+K to choose to close.";
                wchar_t line1[128]{};
                swprintf_s(line1, 128, formatStr.c_str(), session->totalMinutes);
                const wchar_t* line2Text = line2StrU.c_str();

                HFONT fontLarge = ::CreateFontW(
                    -MulDiv(24, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                HGDIOBJ oldFont = ::SelectObject(memDC, fontLarge);

                RECT rcLine1 = rc;
                ::DrawTextW(memDC, line1, -1, &rcLine1, DT_CENTER | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
                int line1Height = rcLine1.bottom - rcLine1.top;

                HFONT fontSmall = ::CreateFontW(
                    -MulDiv(12, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                ::SelectObject(memDC, fontSmall);
                RECT rcLine2 = rc;
                ::DrawTextW(memDC, line2Text, -1, &rcLine2, DT_CENTER | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
                int line2Height = rcLine2.bottom - rcLine2.top;

                std::wstring countdown = ctg::timeutils::FormatSecondsToMMSS(session->remainingSeconds);
                HFONT fontMedium = ::CreateFontW(
                    -MulDiv(16, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                ::SelectObject(memDC, fontMedium);
                RECT rcLine3 = rc;
                ::DrawTextW(memDC, countdown.c_str(), -1, &rcLine3, DT_CENTER | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
                int line3Height = rcLine3.bottom - rcLine3.top;

                ::SelectObject(memDC, oldFont);
                ::DeleteObject(fontLarge);
                ::DeleteObject(fontSmall);
                ::DeleteObject(fontMedium);

                int totalTextHeight = line1Height + gap + line2Height + gap + line3Height + gap + 128;
                int startY = (h - totalTextHeight) / 2;

                fontLarge = ::CreateFontW(
                    -MulDiv(24, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                ::SelectObject(memDC, fontLarge);
                RECT r1 = { 0, startY, w, startY + line1Height };
                ::DrawTextW(memDC, line1, -1, &r1, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
                ::SelectObject(memDC, oldFont);
                ::DeleteObject(fontLarge);

                fontSmall = ::CreateFontW(
                    -MulDiv(12, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                ::SelectObject(memDC, fontSmall);
                RECT r2 = { 0, startY + line1Height + gap, w, startY + line1Height + gap + line2Height };
                ::DrawTextW(memDC, line2Text, -1, &r2, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
                ::SelectObject(memDC, oldFont);
                ::DeleteObject(fontSmall);

                fontMedium = ::CreateFontW(
                    -MulDiv(16, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                ::SelectObject(memDC, fontMedium);
                ::SetTextColor(memDC, RGB(255, 0, 0));
                RECT r3 = { 0, startY + line1Height + gap + line2Height + gap, w, startY + line1Height + gap + line2Height + gap + line3Height };
                ::DrawTextW(memDC, countdown.c_str(), -1, &r3, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
                ::SetTextColor(memDC, RGB(0, 0, 0));
                ::SelectObject(memDC, oldFont);
                ::DeleteObject(fontMedium);

                if (session->hIcon128)
                {
                    int iconX = (w - 128) / 2;
                    int iconY = startY + line1Height + gap + line2Height + gap + line3Height + gap;
                    ::DrawIconEx(memDC, iconX, iconY, session->hIcon128, 128, 128, 0, nullptr, DI_NORMAL);
                }
            }

            ::BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

            ::SelectObject(memDC, oldBmp);
            ::DeleteObject(bmp);
            ::DeleteDC(memDC);
            ::EndPaint(hwnd, &ps);
        }

        void OnUnavailableLockTimer(UnavailableLockSession* session)
        {
            if (!session || session->windows.empty())
                return;

            if (session->countdownEnded)
                return;

            session->remainingSeconds--;
            if (session->remainingSeconds < 0)
                session->remainingSeconds = 0;

            for (HWND h : session->windows)
            {
                if (::IsWindow(h))
                    ::InvalidateRect(h, nullptr, FALSE);
            }

            if (session->remainingSeconds <= 0)
            {
                session->countdownEnded = true;
                session->shutdownFailed = !ctg::platform::ShutdownController::RequestShutdown();
                if (session->primaryHwnd && ::IsWindow(session->primaryHwnd))
                {
                    ::KillTimer(session->primaryHwnd, kTimerIdUnavailable);
                    ::SetTimer(session->primaryHwnd, kTimerIdUnavailableAnim, kUnavailableAnimIntervalMs, nullptr);
                }
                ctg::platform::ShutdownController::RequestShutdown();
                InitBubblesUnavailable(session);
                for (HWND h : session->windows)
                {
                    if (::IsWindow(h))
                        ::InvalidateRect(h, nullptr, FALSE);
                }
            }
        }

        void OnUnavailableLockAnimTimer(UnavailableLockSession* session)
        {
            if (!session || session->windows.empty())
                return;
            UpdateBubblesUnavailable(session);
            for (HWND h : session->windows)
            {
                if (::IsWindow(h))
                    ::InvalidateRect(h, nullptr, FALSE);
            }
        }

        BOOL CALLBACK EnumMonitorsProcUnavailable(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM lParam)
        {
            (void)hMonitor;
            (void)hdcMonitor;
            auto* session = reinterpret_cast<UnavailableLockSession*>(lParam);
            if (!session)
                return FALSE;

            HWND parent = session->parentToNotify;
            HINSTANCE hInst = parent ? reinterpret_cast<HINSTANCE>(::GetClassLongPtrW(parent, GCLP_HMODULE)) : nullptr;
            if (!hInst)
                hInst = ::GetModuleHandleW(nullptr);

            HWND hwnd = ::CreateWindowExW(
                WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                L"CTG_UnavailableLockWindow",
                nullptr,
                WS_POPUP | WS_VISIBLE,
                lprcMonitor->left,
                lprcMonitor->top,
                lprcMonitor->right - lprcMonitor->left,
                lprcMonitor->bottom - lprcMonitor->top,
                parent,
                nullptr,
                hInst,
                session);

            if (hwnd)
            {
                session->windows.push_back(hwnd);
                if (!session->primaryHwnd)
                    session->primaryHwnd = hwnd;
            }

            return TRUE;
        }

        LRESULT CALLBACK UnavailableLockWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            UnavailableLockSession* session = GetUnavailableSession(hwnd);

            if (msg == WM_CREATE)
            {
                auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                session = reinterpret_cast<UnavailableLockSession*>(cs->lpCreateParams);
                ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(session));
                if (session && !session->primaryHwnd)
                    session->primaryHwnd = hwnd;
                if (session && !session->hIcon128)
                {
                    HINSTANCE hInst = reinterpret_cast<HINSTANCE>(::GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                    if (hInst)
                    {
                        session->hIcon128 = reinterpret_cast<HICON>(::LoadImageW(
                            hInst,
                            MAKEINTRESOURCEW(IDI_APP_ICON),
                            IMAGE_ICON,
                            128, 128,
                            LR_DEFAULTCOLOR));
                    }
                }
                return 0;
            }

            session = GetUnavailableSession(hwnd);

            switch (msg)
            {
            case WM_PAINT:
                PaintUnavailableLock(hwnd, session);
                return 0;

            case WM_TIMER:
                if (session && hwnd == session->primaryHwnd)
                {
                    if (wParam == kTimerIdUnavailable)
                        OnUnavailableLockTimer(session);
                    else if (wParam == kTimerIdUnavailableAnim)
                        OnUnavailableLockAnimTimer(session);
                }
                return 0;

            case WM_DESTROY:
                if (session && hwnd == session->primaryHwnd)
                {
                    ::KillTimer(hwnd, kTimerIdUnavailable);
                    ::KillTimer(hwnd, kTimerIdUnavailableAnim);
                    if (session->hIcon128)
                    {
                        ::DestroyIcon(session->hIcon128);
                        session->hIcon128 = nullptr;
                    }
                }
                return 0;

            case WM_ERASEBKGND:
                {
                    RECT rc;
                    ::GetClientRect(hwnd, &rc);
                    ::FillRect(reinterpret_cast<HDC>(wParam), &rc, reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
                }
                return 1;
            }

            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        bool RegisterUnavailableLockClass(HINSTANCE hInst)
        {
            WNDCLASSW wc{};
            wc.lpfnWndProc = UnavailableLockWndProc;
            wc.hInstance = hInst;
            wc.lpszClassName = L"CTG_UnavailableLockWindow";
            wc.hCursor = ::LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));
            wc.hbrBackground = reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH));
            return ::RegisterClassW(&wc) != 0;
        }

    } // namespace

    void LockScreenWindow::ShowRestLock(HWND parent, int restMinutes, int usedMinutesForLine1, int warnMinutesForLine1)
    {
        HINSTANCE hInst = nullptr;
        if (parent)
            hInst = reinterpret_cast<HINSTANCE>(::GetClassLongPtrW(parent, GCLP_HMODULE));
        if (!hInst)
            hInst = ::GetModuleHandleW(nullptr);

        static bool classRegistered = false;
        if (!classRegistered)
        {
            if (!RegisterRestLockClass(hInst))
                return;
            classRegistered = true;
        }

        RestLockSession* session = &g_restLockSession;
        session->restMinutes = restMinutes;
        session->usedMinutesForLine1 = usedMinutesForLine1;
        session->warnMinutesForLine1 = warnMinutesForLine1;
        session->remainingSeconds = restMinutes * 60;
        session->parentToNotify = parent;
        session->windows.clear();
        session->primaryHwnd = nullptr;
        session->bubblesInitialized = false;

        srand(static_cast<unsigned>(::GetTickCount()));
        ::EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsProc, reinterpret_cast<LPARAM>(session));

        if (session->primaryHwnd && ::IsWindow(session->primaryHwnd))
        {
            ::SetTimer(session->primaryHwnd, kTimerIdRest, kTimerIntervalMs, nullptr);
            ::SetTimer(session->primaryHwnd, kTimerIdAnim, kAnimIntervalMs, nullptr);
        }
    }

    bool LockScreenWindow::CloseRestLockIfActive()
    {
        RestLockSession* session = &g_restLockSession;
        if (session->windows.empty() || !session->primaryHwnd || !::IsWindow(session->primaryHwnd))
            return false;

        ::KillTimer(session->primaryHwnd, kTimerIdRest);
        ::KillTimer(session->primaryHwnd, kTimerIdAnim);

        for (HWND h : session->windows)
        {
            if (::IsWindow(h))
                ::DestroyWindow(h);
        }
        session->windows.clear();
        session->primaryHwnd = nullptr;
        return true;
    }

    void LockScreenWindow::ShowUnavailableLock(HWND parent, int secondsToShutdown)
    {
        HINSTANCE hInst = nullptr;
        if (parent)
            hInst = reinterpret_cast<HINSTANCE>(::GetClassLongPtrW(parent, GCLP_HMODULE));
        if (!hInst)
            hInst = ::GetModuleHandleW(nullptr);

        static bool classRegistered = false;
        if (!classRegistered)
        {
            if (!RegisterUnavailableLockClass(hInst))
                return;
            classRegistered = true;
        }

        UnavailableLockSession* session = &g_unavailableLockSession;
        session->totalSeconds = secondsToShutdown;
        session->totalMinutes = (secondsToShutdown >= 60) ? (secondsToShutdown / 60) : 1;
        session->remainingSeconds = secondsToShutdown;
        session->parentToNotify = parent;
        session->countdownEnded = false;
        session->shutdownFailed = false;
        session->windows.clear();
        session->primaryHwnd = nullptr;
        session->bubblesInitialized = false;

        ::EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsProcUnavailable, reinterpret_cast<LPARAM>(session));

        if (session->primaryHwnd && ::IsWindow(session->primaryHwnd))
            ::SetTimer(session->primaryHwnd, kTimerIdUnavailable, kUnavailableTimerIntervalMs, nullptr);
    }

    bool LockScreenWindow::CloseUnavailableLockIfActive()
    {
        UnavailableLockSession* session = &g_unavailableLockSession;
        if (session->windows.empty() || !session->primaryHwnd || !::IsWindow(session->primaryHwnd))
            return false;

        ::KillTimer(session->primaryHwnd, kTimerIdUnavailable);
        ::KillTimer(session->primaryHwnd, kTimerIdUnavailableAnim);

        for (HWND h : session->windows)
        {
            if (::IsWindow(h))
                ::DestroyWindow(h);
        }
        session->windows.clear();
        session->primaryHwnd = nullptr;
        return true;
    }

} // namespace ui
} // namespace ctg
