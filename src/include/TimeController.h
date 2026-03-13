#pragma once

#include <cstdint>

#include "AppConfig.h"

namespace ctg {
namespace core {

    // Describes in which kind of time segment the controller currently is.
    enum class TimeSegment
    {
        Available,
        Unavailable
    };

    // High‑level lock state from the controller's point of view.
    enum class LockState
    {
        None,
        RestLock,        // In "rest" lock screen inside available time
        UnavailableLock  // In "unavailable" shutdown lock screen
    };

    // Events interface implemented by the UI / application layer.
    struct ITimeControllerEvents
    {
        virtual ~ITimeControllerEvents() = default;

        // Reached (N - M) minutes of continuous use in available time.
        virtual void OnShowUsageWarning() = 0;

        // Reached N minutes of continuous use in available time.
        virtual void OnStartRestLock() = 0;

        // Currently in unavailable time segment (need shutdown lock screen).
        virtual void OnStartUnavailableLock() = 0;
    };

    // Core timing logic: consumes elapsed seconds and current time segment,
    // emits high‑level events when thresholds are crossed.
    class TimeController
    {
    public:
        explicit TimeController(const config::AppConfig& cfg);

        void SetEvents(ITimeControllerEvents* events) noexcept;

        // Update configuration (typically after user changes settings).
        void UpdateConfig(const config::AppConfig& cfg);

        // Called once per second by the timer worker.
        void Tick(TimeSegment segment);

        // Called when a rest lock screen has fully finished and closed.
        void OnRestLockFinished();

        // Called when the unavailable-time lock is dismissed by parent entering correct password.
        // For the rest of the current unavailable segment, the lock will not be shown again.
        void OnUnavailableLockDismissedByParent();

        // Start or restart temporary suspend for Y minutes.
        // currentSegment is the segment at the moment of starting suspend.
        void StartTemporarySuspend(int minutesY, TimeSegment currentSegment);

        bool IsTemporarySuspendActive() const noexcept { return m_tempSuspendActive; }
        std::uint32_t GetTemporarySuspendRemainingSeconds() const noexcept { return m_tempRemainingSeconds; }

        // Expose current state for diagnostics / tests.
        LockState GetLockState() const noexcept { return m_lockState; }

        // For tests: returns the accumulated continuous usage seconds
        // within the current available segment (outside temporary suspend).
        std::uint32_t GetContinuousSeconds() const noexcept { return m_continuousSeconds; }

    private:
        void ResetUsageCounter();
        void ResetTemporarySuspend();

        // Helpers derived from configuration.
        std::uint32_t m_warnThresholdSeconds{ 0 };   // (N - M) * 60
        std::uint32_t m_lockThresholdSeconds{ 0 };   // N * 60
        int m_warnMinutesConfig{ 0 };                // M (for temporary suspend logic)

        ITimeControllerEvents* m_events{ nullptr };

        // Normal available‑time usage counters.
        std::uint32_t m_continuousSeconds{ 0 };
        bool m_warningShown{ false };
        TimeSegment m_lastSegment{ TimeSegment::Available };
        LockState m_lockState{ LockState::None };

        // Temporary suspend state (section 6.5).
        bool m_tempSuspendActive{ false };
        std::uint32_t m_tempRemainingSeconds{ 0 };   // counts down to 0
        std::uint32_t m_tempTotalSeconds{ 0 };       // Y * 60
        std::uint32_t m_tempWarnAtSeconds{ 0 };      // (Y - M) * 60, 0 if no warning
        std::uint32_t m_tempElapsedSeconds{ 0 };     // seconds since temporary suspend started
        bool m_tempWarnShown{ false };
        bool m_tempRestTriggered{ false };

        // Parent entered correct password on unavailable lock; do not show it again this segment.
        bool m_unavailableParentGranted{ false };
    };

} // namespace core
} // namespace ctg

