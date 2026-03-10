#pragma once

#include <windows.h>

#include <memory>

#include "ConfigManager.h"
#include "TimeController.h"
#include "HotkeyManager.h"
#include "ResourceManager.h"

// Application entry class: wires message loop and core modules.
class App : public ctg::core::ITimeControllerEvents
{
public:
    App();
    ~App();

    int Run(HINSTANCE hInstance, int nCmdShow);

    // ITimeControllerEvents implementation.
    void OnShowUsageWarning() override;
    void OnStartRestLock() override;
    void OnStartUnavailableLock() override;

private:
    bool Initialize(HINSTANCE hInstance);
    void Shutdown();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void OnTimerTick();

    HINSTANCE m_hInstance{ nullptr };
    HWND m_hwnd{ nullptr };

    ctg::config::ConfigManager m_configManager;
    ctg::i18n::ResourceManager m_resourceManager;
    std::unique_ptr<ctg::core::TimeController> m_timeController;
    ctg::platform::HotkeyManager m_hotkeyManager;

    // Background timer thread state.
    HANDLE m_timerThread{ nullptr };
    HANDLE m_stopEvent{ nullptr };

    // Single-instance mutex: held until process exits.
    HANDLE m_singleInstanceMutex{ nullptr };

    static DWORD WINAPI TimerThreadProc(LPVOID param);
};

