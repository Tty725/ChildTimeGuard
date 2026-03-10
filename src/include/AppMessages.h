#pragma once

#include <windows.h>

// Custom messages for the hidden host window (WM_USER range for this window class).
// Used to marshal time-controller events from timer thread to UI thread.
#define WM_CTGUARD_SHOW_REMINDER        (WM_USER + 1)
#define WM_CTGUARD_SHOW_REST_LOCK       (WM_USER + 2)
#define WM_CTGUARD_REST_LOCK_FINISHED   (WM_USER + 3)
#define WM_CTGUARD_SHOW_UNAVAILABLE_LOCK (WM_USER + 4)
#define WM_CTGUARD_UNAVAILABLE_LOCK_DISMISSED (WM_USER + 5)
