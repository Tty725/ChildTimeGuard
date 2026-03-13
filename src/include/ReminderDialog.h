#pragma once

#include <windows.h>

namespace ctg {
namespace ui {

    // Non-fullscreen countdown reminder dialog.
    // Shows "used X minutes, lock in Y seconds" and a live countdown; "I got it" only closes the dialog.
    class ReminderDialog
    {
    public:
        // Show the reminder dialog (modal). Must be called from the UI thread.
        // totalSeconds is M*60; countdown display updates every second.
        static void ShowOnce(HINSTANCE hInstance, HWND parent, int usedMinutes, int totalSeconds);
    };

} // namespace ui
} // namespace ctg
