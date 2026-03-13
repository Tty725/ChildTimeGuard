#pragma once

#include <windows.h>

namespace ctg {
namespace platform {

    // 封装 Ctrl+Shift+K 全局热键注册/注销。
    class HotkeyManager
    {
    public:
        // 使用给定窗口句柄注册 Ctrl+Shift+K 热键。
        // 返回 true 表示注册成功，false 表示失败（例如键已被占用）。
        bool Register(HWND targetWindow);

        // 取消注册热键。
        void Unregister();

        // 应在消息循环中配合 WM_HOTKEY 使用。
        static constexpr int kHotkeyId = 1;

    private:
        HWND m_targetWindow{ nullptr };
        bool m_registered{ false };
    };

} // namespace platform
} // namespace ctg

