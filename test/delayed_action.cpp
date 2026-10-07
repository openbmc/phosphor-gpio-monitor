// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "delayed_action.hpp"

#include <systemd/sd-event.h>

#include <chrono>

#include <gtest/gtest.h>

using phosphor::gpio::DelayedAction;
using namespace std::chrono_literals;

class DelayedActionTest : public ::testing::Test
{
  public:
    DelayedActionTest()
    {
        EXPECT_GE(sd_event_new(&event), 0);
    }

    ~DelayedActionTest()
    {
        sd_event_unref(event);
    }

    // Runs one loop iteration, waiting at most the given time.
    int runOnce(std::chrono::microseconds timeout)
    {
        return sd_event_run(event, timeout.count());
    }

    sd_event* event = nullptr;
    int runs = 0;
};

TEST_F(DelayedActionTest, runsAfterDelayWithoutBlocking)
{
    DelayedAction action(event, [this] { ++runs; });

    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(action.schedule(100ms), 0);

    // Scheduling returns straight away, the action has not run yet.
    EXPECT_TRUE(action.pending());
    EXPECT_EQ(runs, 0);

    // The loop stays free to dispatch other work before the delay expires.
    runOnce(1ms);
    EXPECT_EQ(runs, 0);

    while (runs == 0 && std::chrono::steady_clock::now() - start < 5s)
    {
        runOnce(500ms);
    }
    EXPECT_EQ(runs, 1);
    EXPECT_FALSE(action.pending());
    EXPECT_GE(std::chrono::steady_clock::now() - start, 100ms);
}

TEST_F(DelayedActionTest, cancelPreventsRun)
{
    DelayedAction action(event, [this] { ++runs; });

    EXPECT_EQ(action.schedule(20ms), 0);
    EXPECT_TRUE(action.cancel());
    EXPECT_FALSE(action.pending());

    runOnce(100ms);
    EXPECT_EQ(runs, 0);

    // Nothing left to cancel.
    EXPECT_FALSE(action.cancel());
}

TEST_F(DelayedActionTest, scheduleWhilePendingKeepsFirstTimer)
{
    DelayedAction action(event, [this] { ++runs; });

    EXPECT_EQ(action.schedule(20ms), 0);
    EXPECT_EQ(action.schedule(20ms), 0);

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < 200ms)
    {
        runOnce(50ms);
    }
    EXPECT_EQ(runs, 1);
}
