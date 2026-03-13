#include "HotkeyManager.h"

namespace ctg {
namespace platform {

    bool HotkeyManager::Register(HWND targetWindow)
    {
        m_targetWindow = targetWindow;

        if (m_registered)
        {
            // 先尝试注销旧的，再重新注册。
            Unregister();
        }

        // Use Ctrl+Shift+K as development hotkey to reduce conflicts.
        const UINT modifiers = MOD_CONTROL | MOD_SHIFT;
        const UINT vk = 'K';

        if (::RegisterHotKey(m_targetWindow, kHotkeyId, modifiers, vk))
        {
            m_registered = true;
            return true;
        }

        m_registered = false;
        return false;
    }

    void HotkeyManager::Unregister()
    {
        if (m_registered && m_targetWindow)
        {
            ::UnregisterHotKey(m_targetWindow, kHotkeyId);
        }

        m_registered = false;
    }

} // namespace platform
} // namespace ctg

