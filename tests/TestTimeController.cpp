#include "TestFramework.h"

#include "TimeController.h"
#include "AppConfig.h"

using namespace ctg::core;
using namespace ctg::config;

namespace {

    struct FakeEvents : ITimeControllerEvents
    {
        int warnCount = 0;
        int restLockCount = 0;
        int unavailableLockCount = 0;

        void OnShowUsageWarning() override { ++warnCount; }
        void OnStartRestLock() override { ++restLockCount; }
        void OnStartUnavailableLock() override { ++unavailableLockCount; }
    };

    TimeController MakeControllerForTest(FakeEvents& events)
    {
        AppConfig cfg = MakeDefaultConfig();
        cfg.warnMinutes = 2;            // M
        cfg.maxContinuousMinutes = 5;   // N (小一点方便测试)

        TimeController controller(cfg);
        controller.SetEvents(&events);
        return controller;
    }
}

CTG_TEST(TimeController_WarningAndRestLock_InAvailable)
{
    FakeEvents events;
    auto controller = MakeControllerForTest(events);

    // N = 5, M = 2 => warn at 3 minutes, lock at 5 minutes.
    const int totalSeconds = 5 * 60;
    for (int i = 0; i < totalSeconds; ++i)
    {
        controller.Tick(TimeSegment::Available);
    }

    CTG_EXPECT_TRUE(events.warnCount >= 1);
    CTG_EXPECT_TRUE(events.restLockCount == 1);
    CTG_EXPECT_TRUE(controller.GetLockState() == LockState::RestLock);
}

CTG_TEST(TimeController_EnterUnavailable_TriggersLock)
{
    FakeEvents events;
    auto controller = MakeControllerForTest(events);

    controller.Tick(TimeSegment::Unavailable);
    CTG_EXPECT_TRUE(events.unavailableLockCount == 1);
    CTG_EXPECT_TRUE(controller.GetLockState() == LockState::UnavailableLock);
}

CTG_TEST(TimeController_TemporarySuspend_InAvailable)
{
    FakeEvents events;
    auto controller = MakeControllerForTest(events);

    // Start temporary suspend Y = 4 minutes in available segment, M = 2.
    // => 前 (Y - M) = 2 分钟不预警不锁；到第 2 分钟后开始预警；第 4 分钟锁屏。
    controller.StartTemporarySuspend(4, TimeSegment::Available);

    // 模拟 2 分钟内：不应触发任何事件。
    for (int i = 0; i < 2 * 60; ++i)
    {
        controller.Tick(TimeSegment::Available);
    }
    CTG_EXPECT_TRUE(events.warnCount == 0);
    CTG_EXPECT_TRUE(events.restLockCount == 0);

    // 再过 2 分钟：应至少出现一次预警和一次休息锁屏。
    for (int i = 0; i < 2 * 60; ++i)
    {
        controller.Tick(TimeSegment::Available);
    }

    CTG_EXPECT_TRUE(events.warnCount >= 1);
    CTG_EXPECT_TRUE(events.restLockCount == 1);
    CTG_EXPECT_TRUE(controller.GetLockState() == LockState::RestLock);
}

CTG_TEST(TimeController_TemporarySuspend_InUnavailable_SuppressesLock)
{
    FakeEvents events;
    auto controller = MakeControllerForTest(events);

    // 当前在非可用时间段内启动临时停用，Y = 3 分钟。
    controller.StartTemporarySuspend(3, TimeSegment::Unavailable);

    for (int i = 0; i < 3 * 60; ++i)
    {
        controller.Tick(TimeSegment::Unavailable);
    }

    // 在 Y 分钟内不应触发不可用锁屏事件。
    CTG_EXPECT_TRUE(events.unavailableLockCount == 0);
}

