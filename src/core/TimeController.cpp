#include "TimeController.h"

namespace ctg {
namespace core {

    namespace {

        std::uint32_t ToSeconds(int minutes)
        {
            if (minutes <= 0)
                return 0;
            return static_cast<std::uint32_t>(minutes) * 60U;
        }
    } // namespace

    TimeController::TimeController(const config::AppConfig& cfg)
    {
        UpdateConfig(cfg);
    }

    void TimeController::SetEvents(ITimeControllerEvents* events) noexcept
    {
        m_events = events;
    }

    void TimeController::UpdateConfig(const config::AppConfig& cfg)
    {
        const int warnMinutes = cfg.warnMinutes;
        const int maxMinutes = cfg.maxContinuousMinutes;

        if (warnMinutes > 0 && maxMinutes > warnMinutes)
        {
            m_warnThresholdSeconds = ToSeconds(maxMinutes - warnMinutes);
        }
        else
        {
            m_warnThresholdSeconds = 0;
        }

        m_lockThresholdSeconds = ToSeconds(maxMinutes);
        m_warnMinutesConfig = warnMinutes;

        // Reset runtime state when configuration changes.
        ResetUsageCounter();
        ResetTemporarySuspend();
        m_lockState = LockState::None;
        m_lastSegment = TimeSegment::Available;
    }

    void TimeController::Tick(TimeSegment segment)
    {
        // Decrease temporary suspend countdown and advance elapsed time when active.
        const bool tempActiveNow = (m_tempSuspendActive && m_tempRemainingSeconds > 0);
        if (tempActiveNow)
        {
            --m_tempRemainingSeconds;
            ++m_tempElapsedSeconds;
        }

        // Handle unavailable segment.
        if (segment == TimeSegment::Unavailable)
        {
            m_lastSegment = segment;

            if (tempActiveNow)
            {
                // During temporary suspend we must not trigger unavailable lock.
                m_lockState = LockState::None;
                return;
            }

            // Parent already entered correct password this segment; do not show lock again.
            if (m_unavailableParentGranted)
            {
                m_lockState = LockState::None;
                return;
            }

            m_continuousSeconds = 0;
            m_warningShown = false;

            if (m_lockState != LockState::UnavailableLock)
            {
                m_lockState = LockState::UnavailableLock;
                if (m_events)
                {
                    m_events->OnStartUnavailableLock();
                }
            }

            return;
        }

        // From here on we are in available segment.
        if (m_lockState == LockState::RestLock)
        {
            // During rest lock we don't advance usage counter.
            m_lastSegment = segment;
            return;
        }

        if (m_lastSegment != TimeSegment::Available)
        {
            // Switched from unavailable -> available; start fresh.
            ResetUsageCounter();
            m_lockState = LockState::None;
            m_unavailableParentGranted = false;
        }

        m_lastSegment = segment;

        // If temporary suspend is active in available segment, use its own schedule.
        if (m_tempSuspendActive)
        {
            const std::uint32_t elapsedTemp = m_tempElapsedSeconds;

            // First, handle rest-lock at the end of Y minutes.
            if (!m_tempRestTriggered &&
                m_tempTotalSeconds > 0 &&
                elapsedTemp >= m_tempTotalSeconds)
            {
                m_tempRestTriggered = true;
                m_lockState = LockState::RestLock;
                if (m_events)
                {
                    m_events->OnStartRestLock();
                }
                // Once rest lock is triggered, we do not continue normal counting.
                return;
            }

            // Then handle warning at (Y - M) minutes if applicable.
            if (!m_tempWarnShown &&
                m_tempWarnAtSeconds > 0 &&
                elapsedTemp > m_tempWarnAtSeconds &&
                elapsedTemp < m_tempTotalSeconds)
            {
                m_tempWarnShown = true;
                if (m_events)
                {
                    m_events->OnShowUsageWarning();
                }
            }

            // While temporary suspend is active and not yet finished,
            // normal continuousSeconds-based schedule is suppressed.
            if (m_tempRemainingSeconds > 0)
            {
                return;
            }

            // If suspend just ended without triggering rest lock, fall back to normal logic.
            if (m_tempRemainingSeconds == 0)
            {
                ResetTemporarySuspend();
                ResetUsageCounter();
            }
        }

        // Advance continuous usage counter by one second.
        ++m_continuousSeconds;

        // Check thresholds only when not already locked.
        if (m_lockState == LockState::None)
        {
            if (!m_warningShown &&
                m_warnThresholdSeconds > 0 &&
                m_continuousSeconds >= m_warnThresholdSeconds)
            {
                m_warningShown = true;
                if (m_events)
                {
                    m_events->OnShowUsageWarning();
                }
            }

            if (m_lockThresholdSeconds > 0 &&
                m_continuousSeconds >= m_lockThresholdSeconds)
            {
                m_lockState = LockState::RestLock;
                if (m_events)
                {
                    m_events->OnStartRestLock();
                }
            }
        }
    }

    void TimeController::OnRestLockFinished()
    {
        // After rest lock, start a new cycle in available segment.
        m_lockState = LockState::None;
        ResetUsageCounter();
        ResetTemporarySuspend();
        m_lastSegment = TimeSegment::Available;
    }

    void TimeController::OnUnavailableLockDismissedByParent()
    {
        m_lockState = LockState::None;
        m_unavailableParentGranted = true;
    }

    void TimeController::StartTemporarySuspend(int minutesY, TimeSegment currentSegment)
    {
        if (minutesY <= 0)
        {
            ResetTemporarySuspend();
            return;
        }

        const std::uint32_t totalSeconds = ToSeconds(minutesY);
        m_tempSuspendActive = true;
        m_tempTotalSeconds = totalSeconds;
        m_tempRemainingSeconds = totalSeconds;
        m_tempElapsedSeconds = 0;
        m_tempRestTriggered = false;

        // In available segment we still want a warning M minutes before end of Y.
        // In unavailable segment, there is no warning nor lock until Y reaches 0.
        if (currentSegment == TimeSegment::Available &&
            m_warnMinutesConfig > 0 &&
            minutesY > m_warnMinutesConfig)
        {
            // Warn at (Y - M) minutes from the start of temporary suspend.
            m_tempWarnAtSeconds =
                static_cast<std::uint32_t>((minutesY - m_warnMinutesConfig) * 60);
        }
        else
        {
            m_tempWarnAtSeconds = 0;
        }

        m_tempWarnShown = false;

        // During temporary suspend we ignore normal continuous usage counters.
        ResetUsageCounter();
    }

    void TimeController::ResetUsageCounter()
    {
        m_continuousSeconds = 0;
        m_warningShown = false;
    }

    void TimeController::ResetTemporarySuspend()
    {
        m_tempSuspendActive = false;
        m_tempRemainingSeconds = 0;
        m_tempTotalSeconds = 0;
        m_tempWarnAtSeconds = 0;
        m_tempElapsedSeconds = 0;
        m_tempWarnShown = false;
        m_tempRestTriggered = false;
    }

} // namespace core
} // namespace ctg

