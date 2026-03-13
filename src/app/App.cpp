#include "App.h"

#include <array>
#include <cwchar>
#include <fstream>

#include "AppMessages.h"
#include "TimeUtils.h"
#include "ControlDialog.h"
#include "ConfigDialog.h"
#include "ReminderDialog.h"
#include "LockScreenWindow.h"
#include "ShutdownController.h"
#include "ResourceManager.h"
#include "AutoStartManager.h"
#include "ConfigManager.h"
#include "resource.h"
#include "StringUtils.h"

using ctg::timeutils::GetNowMinutesOfDay;
using ctg::timeutils::MinutesOfDay;

namespace {

    constexpr UINT_PTR kTimerId = 1;
    constexpr UINT kTimerIntervalMs = 1000;

    const wchar_t kSingleInstanceMutexName[] = L"Local\\ChildTimeGuard_SingleInstance";

    // Returns true if the process was started with --restart (e.g. by LaunchSelfAndRequestExit).
    bool HasRestartInCommandLine()
    {
        const wchar_t* cmdLine = ::GetCommandLineW();
        if (!cmdLine)
            return false;
        const wchar_t sub[] = L"--restart";
        const size_t subLen = (sizeof(sub) / sizeof(sub[0])) - 1;
        for (const wchar_t* p = cmdLine; *p; ++p)
        {
            if (std::wcsncmp(p, sub, subLen) == 0)
            {
                wchar_t after = p[subLen];
                if (after == L'\0' || after == L' ' || after == L'\t')
                    return true;
            }
        }
        return false;
    }

    std::wstring ResolveLanguageCode(const std::string& configLanguage)
    {
        if (configLanguage.empty() || configLanguage == "auto")
        {
            wchar_t localeName[LOCALE_NAME_MAX_LENGTH]{};
            if (::GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) > 0)
            {
                if (localeName[0] == L'z' && localeName[1] == L'h')
                    return L"zh-CN";
            }
            return L"en-US";
        }
        std::wstring code = ctg::strutils::Utf8ToUtf16(configLanguage);
        return code.empty() ? L"en-US" : code;
    }

    ctg::core::TimeSegment GetCurrentSegment(const ctg::config::AppConfig& cfg)
    {
        MinutesOfDay now = GetNowMinutesOfDay();
        const MinutesOfDay start = cfg.usableTime.startMinutes;
        const MinutesOfDay end = cfg.usableTime.endMinutes;

        if (start < end)
        {
            if (now >= start && now < end)
                return ctg::core::TimeSegment::Available;
        }

        return ctg::core::TimeSegment::Unavailable;
    }

    // Launch current exe in new process with --restart, then post WM_CLOSE to exit this process. Returns true on success.
    bool LaunchSelfAndRequestExit(HWND hwndToClose)
    {
        std::array<wchar_t, MAX_PATH> exePath{};
        if (::GetModuleFileNameW(nullptr, exePath.data(), static_cast<DWORD>(exePath.size())) == 0)
            return false;

        std::wstring cmdLine = L"\"";
        cmdLine += exePath.data();
        cmdLine += L"\" --restart";

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};

        if (!::CreateProcessW(
                nullptr,
                &cmdLine[0],
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                nullptr,
                &si,
                &pi))
        {
            return false;
        }

        if (pi.hProcess)
            ::CloseHandle(pi.hProcess);
        if (pi.hThread)
            ::CloseHandle(pi.hThread);

        ::PostMessageW(hwndToClose, WM_CLOSE, 0, 0);
        return true;
    }

    struct HotkeyFailureParams
    {
        HINSTANCE hInstance{ nullptr };
        std::wstring title;
        std::wstring message;
    };

    INT_PTR CALLBACK HotkeyFailureDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            {
                HotkeyFailureParams* p = reinterpret_cast<HotkeyFailureParams*>(lParam);
                if (p)
                {
                    ::SetWindowTextW(hDlg, p->title.c_str());
                    ::SetDlgItemTextW(hDlg, IDC_HOTKEY_FAILURE_MSG, p->message.c_str());

                    HICON hIcon = reinterpret_cast<HICON>(::LoadImageW(
                        p->hInstance,
                        MAKEINTRESOURCEW(IDI_APP_ICON),
                        IMAGE_ICON,
                        0, 0,
                        LR_DEFAULTCOLOR));
                    if (hIcon)
                    {
                        ::SendMessageW(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
                        ::SendMessageW(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
                    }

                    ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
                    if (rm)
                        ::SetDlgItemTextW(hDlg, IDOK, rm->GetString(L"OK").c_str());
                }
                ::SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
            }
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                ::EndDialog(hDlg, IDOK);
                return TRUE;
            }
            break;
        }
        return FALSE;
    }
}

App::App() = default;

App::~App()
{
    Shutdown();
}

int App::Run(HINSTANCE hInstance, int nCmdShow)
{
    (void)nCmdShow;

    if (!Initialize(hInstance))
        return -1;

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

bool App::Initialize(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    // Load configuration (defaults are applied inside ConfigManager when Load fails).
    const bool configLoaded = m_configManager.Load();
    const auto& cfg = m_configManager.Get();

    /* Auto-start when config missing/failed or autoStart enabled. */
    {
        std::array<wchar_t, MAX_PATH> exePath{};
        if (::GetModuleFileNameW(nullptr, exePath.data(), static_cast<DWORD>(exePath.size())) > 0)
        {
            const bool shouldSetAutoStart = !configLoaded || cfg.autoStart;
            if (shouldSetAutoStart)
                ctg::platform::AutoStartManager::EnableAutoStart(L"ChildTimeGuard", exePath.data());
        }
    }

    std::wstring languageCode = ResolveLanguageCode(cfg.languageCode);
    m_resourceManager.SetLanguage(languageCode);

    // Ensure external language.json exists and is valid; then load from it.
    {
        const std::wstring langPath = ctg::config::ConfigManager::GetLanguageFilePath();
        const DWORD attrs = ::GetFileAttributesW(langPath.c_str());
        const bool exists = (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
        bool needWrite = !exists;
        if (exists && !m_resourceManager.ValidateFileFormat(langPath))
            needWrite = true;
        if (needWrite)
        {
            std::string content = ctg::i18n::ReadEmbeddedLanguageJson();
            if (!content.empty())
            {
                std::wstring dir = langPath;
                const auto pos = dir.find_last_of(L"\\/");
                if (pos != std::wstring::npos)
                {
                    dir.resize(pos);
                    if (!dir.empty())
                        ::CreateDirectoryW(dir.c_str(), nullptr);
                }
                std::ofstream out(langPath, std::ios::binary);
                if (out)
                    out.write(content.data(), static_cast<std::streamsize>(content.size()));
            }
        }
        m_resourceManager.LoadFromFile(langPath);
    }

    ctg::i18n::SetResourceManager(&m_resourceManager);

    // Single-instance: create mutex and wait. With --restart wait until we get it; without --restart fail immediately if already running.
    {
        const bool hasRestart = HasRestartInCommandLine();
        HANDLE hMutex = ::CreateMutexW(nullptr, FALSE, kSingleInstanceMutexName);
        if (!hMutex)
            return false;

        const DWORD waitMs = hasRestart ? INFINITE : 0;
        const DWORD waitResult = ::WaitForSingleObject(hMutex, waitMs);

        if (waitResult != WAIT_OBJECT_0)
        {
            ::CloseHandle(hMutex);
            if (!hasRestart)
            {
                ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
                HotkeyFailureParams params;
                params.hInstance = m_hInstance;
                params.title = rm ? rm->GetString(L"Child Time Guard") : std::wstring(L"Child Time Guard");
                params.message = rm ? rm->GetString(L"There is no need to run the application again; after running, you can use Ctrl Shift K to confirm.") : std::wstring(L"There is no need to run the application again; after running, you can use Ctrl Shift K to confirm.");
                ::DialogBoxParamW(m_hInstance, MAKEINTRESOURCEW(IDD_HOTKEY_FAILURE_DIALOG), nullptr, HotkeyFailureDlgProc, reinterpret_cast<LPARAM>(&params));
            }
            return false;
        }

        m_singleInstanceMutex = hMutex;
    }

    // Create TimeController with current configuration.
    m_timeController = std::make_unique<ctg::core::TimeController>(cfg);
    m_timeController->SetEvents(this);

    // Register a hidden window class for hosting the message loop and hotkey/timer logic.
    WNDCLASSW wclass{};
    wclass.lpfnWndProc = &App::WndProc;
    wclass.hInstance = m_hInstance;
    wclass.lpszClassName = L"CTG_HiddenHostWindow";
    wclass.hCursor = ::LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));

    if (!::RegisterClassW(&wclass))
    {
        DWORD err = ::GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS)
            return false;
    }

    m_hwnd = ::CreateWindowExW(
        0,
        wclass.lpszClassName,
        L"ChildTimeGuardHidden",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        m_hInstance,
        this);

    if (!m_hwnd)
        return false;

    // 不显示隐藏窗口。

    // Register Ctrl+Shift+K hotkey.
    const bool hotkeyOk = m_hotkeyManager.Register(m_hwnd);
    if (!hotkeyOk)
    {
        ctg::i18n::ResourceManager* rm = ctg::i18n::GetResourceManager();
        HotkeyFailureParams params;
        params.hInstance = m_hInstance;
        params.title = rm ? rm->GetString(L"Child Time Guard") : std::wstring(L"Child Time Guard");
        params.message = rm ? rm->GetString(L"Failed to register Ctrl+Shift+K hotkey.") : std::wstring(L"Failed to register Ctrl+Shift+K hotkey.");
        ::DialogBoxParamW(m_hInstance, MAKEINTRESOURCEW(IDD_HOTKEY_FAILURE_DIALOG), m_hwnd, HotkeyFailureDlgProc, reinterpret_cast<LPARAM>(&params));
    }

    // Create stop event for the background timer thread.
    m_stopEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_stopEvent)
        return false;

    // Start background timer thread (ticks every second).
    m_timerThread = ::CreateThread(
        nullptr,
        0,
        &App::TimerThreadProc,
        this,
        0,
        nullptr);
    if (!m_timerThread)
    {
        ::CloseHandle(m_stopEvent);
        m_stopEvent = nullptr;
        return false;
    }

    // First run: no valid config -> show config dialog (Cancel and X disabled); on OK save and restart.
    if (!configLoaded)
    {
        ctg::config::AppConfig firstRunCfg = m_configManager.Get();
        if (ctg::ui::ConfigDialog::ShowModal(m_hInstance, m_hwnd, firstRunCfg, true))
        {
            m_configManager.Set(firstRunCfg);
            m_configManager.Save();
            ctg::ui::LockScreenWindow::CloseRestLockIfActive();
            ctg::ui::LockScreenWindow::CloseUnavailableLockIfActive();
            LaunchSelfAndRequestExit(m_hwnd);
        }
        return false;
    }

    return true;
}

void App::Shutdown()
{
    if (m_singleInstanceMutex)
    {
        ::CloseHandle(m_singleInstanceMutex);
        m_singleInstanceMutex = nullptr;
    }

    if (m_hwnd)
    {
        // Signal timer thread to stop.
        if (m_stopEvent)
        {
            ::SetEvent(m_stopEvent);
        }
        if (m_timerThread)
        {
            ::WaitForSingleObject(m_timerThread, INFINITE);
            ::CloseHandle(m_timerThread);
            m_timerThread = nullptr;
        }
        if (m_stopEvent)
        {
            ::CloseHandle(m_stopEvent);
            m_stopEvent = nullptr;
        }

        m_hotkeyManager.Unregister();
        ::DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    App* app = nullptr;
    if (msg == WM_NCCREATE)
    {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(createStruct->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else
    {
        app = reinterpret_cast<App*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (app)
    {
        return app->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT App::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_HOTKEY:
        {
        if (wParam == ctg::platform::HotkeyManager::kHotkeyId)
        {
                ctg::ui::ControlDialogResult result;
                bool ok = ctg::ui::ControlDialog::ShowModal(
                    m_hInstance,
                    hwnd,
                    &m_configManager,
                    m_timeController.get(),
                    &result);
                if (ok)
                {
                    if (result.action == ctg::ui::ControlDialogAction::TempDisable ||
                        result.action == ctg::ui::ControlDialogAction::ExitApp)
                    {
                        if (ctg::ui::LockScreenWindow::CloseRestLockIfActive() && m_timeController)
                            m_timeController->OnRestLockFinished();
                        if (ctg::ui::LockScreenWindow::CloseUnavailableLockIfActive() && m_timeController)
                            m_timeController->OnUnavailableLockDismissedByParent();
                    }
                    switch (result.action)
                    {
                    case ctg::ui::ControlDialogAction::OpenConfig:
                        {
                            ctg::config::AppConfig cfg = m_configManager.Get();
                            if (ctg::ui::ConfigDialog::ShowModal(m_hInstance, hwnd, cfg))
                            {
                                m_configManager.Set(cfg);
                                m_configManager.Save();
                                // 保存后先显式关闭所有锁屏，再启动新进程并退出，确保锁屏立即消失。
                                ctg::ui::LockScreenWindow::CloseRestLockIfActive();
                                ctg::ui::LockScreenWindow::CloseUnavailableLockIfActive();
                                if (!LaunchSelfAndRequestExit(hwnd))
                                {
                                    if (m_timeController)
                                        m_timeController->UpdateConfig(m_configManager.Get());
                                }
                            }
                        }
                        break;
                    case ctg::ui::ControlDialogAction::TempDisable:
                        if (m_timeController)
                        {
                            ctg::core::TimeSegment seg = GetCurrentSegment(m_configManager.Get());
                            m_timeController->StartTemporarySuspend(result.tempDisableMinutes, seg);
                        }
                        break;
                    case ctg::ui::ControlDialogAction::ExitApp:
                        ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
                        break;
                    default:
                        break;
                    }
                }
        }
        }
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;

    case WM_CTGUARD_SHOW_REMINDER:
        {
            int usedMinutes = static_cast<int>(wParam);
            int totalSeconds = static_cast<int>(lParam);
            ctg::ui::ReminderDialog::ShowOnce(m_hInstance, hwnd, usedMinutes, totalSeconds);
        }
        break;

    case WM_CTGUARD_SHOW_REST_LOCK:
        {
            const auto& cfg = m_configManager.Get();
            int usedMinutes = cfg.maxContinuousMinutes - cfg.warnMinutes;
            if (usedMinutes < 0)
                usedMinutes = 0;
            ctg::ui::LockScreenWindow::ShowRestLock(hwnd, cfg.restMinutes, usedMinutes, cfg.warnMinutes);
        }
        break;

    case WM_CTGUARD_REST_LOCK_FINISHED:
        if (m_timeController)
            m_timeController->OnRestLockFinished();
        break;

    case WM_CTGUARD_SHOW_UNAVAILABLE_LOCK:
        {
            const auto& cfg = m_configManager.Get();
            int totalSeconds = cfg.warnMinutes * 60;
            ctg::ui::LockScreenWindow::ShowUnavailableLock(hwnd, totalSeconds);
        }
        break;

    case WM_CTGUARD_UNAVAILABLE_LOCK_DISMISSED:
        if (m_timeController)
            m_timeController->OnUnavailableLockDismissedByParent();
        break;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

void App::OnTimerTick()
{
    if (!m_timeController)
        return;

    const auto& cfg = m_configManager.Get();
    ctg::core::TimeSegment segment = GetCurrentSegment(cfg);

    m_timeController->Tick(segment);
}

// ITimeControllerEvents 实现（由定时器线程调用，通过 PostMessage 转到主线程执行 UI）。

void App::OnShowUsageWarning()
{
    if (!m_timeController)
        return;

    const auto& cfg = m_configManager.Get();
    std::uint32_t seconds = m_timeController->GetContinuousSeconds();
    int usedMinutes = static_cast<int>(seconds / 60);
    int totalSeconds = cfg.warnMinutes * 60;

    ::PostMessageW(m_hwnd, WM_CTGUARD_SHOW_REMINDER, static_cast<WPARAM>(usedMinutes), static_cast<LPARAM>(totalSeconds));
}

void App::OnStartRestLock()
{
    ::PostMessageW(m_hwnd, WM_CTGUARD_SHOW_REST_LOCK, 0, 0);
    // OnRestLockFinished() is called when rest lock window closes (WM_CTGUARD_REST_LOCK_FINISHED).
}

void App::OnStartUnavailableLock()
{
    ::PostMessageW(m_hwnd, WM_CTGUARD_SHOW_UNAVAILABLE_LOCK, 0, 0);
}

DWORD WINAPI App::TimerThreadProc(LPVOID param)
{
    auto* app = static_cast<App*>(param);
    if (!app)
        return 0;

    while (true)
    {
        DWORD wait = ::WaitForSingleObject(app->m_stopEvent, kTimerIntervalMs);
        if (wait == WAIT_OBJECT_0)
            break;
        if (wait == WAIT_TIMEOUT)
        {
            app->OnTimerTick();
        }
        else
        {
            break;
        }
    }

    return 0;
}

