#pragma once

#include <windows.h>

namespace ctg {
namespace ui {

    // Full-screen lock windows: rest lock (available time) and unavailable-time shutdown lock.
    class LockScreenWindow
    {
    public:
        // Show rest lock: full-screen, one window per monitor; 3 lines (prompt, Ctrl+Shift+K hint, countdown).
        // First line: "小宝贝，站起来看看远方的风景，或者朗读您最喜欢书籍与故事。" usedMinutesForLine1, warnMinutesForLine1 kept for API compatibility, not used for line1.
        static void ShowRestLock(HWND parent, int restMinutes, int usedMinutesForLine1, int warnMinutesForLine1);

        // Close rest lock if currently showing (e.g. user chose TempDisable or ExitApp via Ctrl+Shift+K with correct password).
        // Returns true if a rest lock was active and has been closed; then caller should call TimeController::OnRestLockFinished().
        static bool CloseRestLockIfActive();

        // Show "unavailable time" full-screen shutdown lock: 3 lines (prompt, Ctrl+Shift+K hint, countdown). No password. Countdown 0 triggers shutdown then bubbles.
        static void ShowUnavailableLock(HWND parent, int secondsToShutdown);

        // Close unavailable lock if currently showing. Returns true if it was active (caller may call TimeController::OnUnavailableLockDismissedByParent).
        static bool CloseUnavailableLockIfActive();
    };

} // namespace ui
} // namespace ctg
