// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "evdev.hpp"

#include <libevdev/libevdev.h>
#include <linux/input.h>

#include <cerrno>
#include <deque>
#include <vector>

#include <gtest/gtest.h>

using namespace phosphor::gpio;

namespace
{

struct Read
{
    int rc;
    input_event ev;
};

input_event makeEvent(uint16_t type, uint16_t code, int32_t value)
{
    input_event ev{};
    ev.type = type;
    ev.code = code;
    ev.value = value;
    return ev;
}

// Plays back a scripted sequence of libevdev_next_event() results and
// records the flags each read was made with.
class FakeDevice
{
  public:
    explicit FakeDevice(std::deque<Read> script) : script(std::move(script)) {}

    int next(unsigned int flags, input_event& ev)
    {
        flagsSeen.push_back(flags);
        if (script.empty())
        {
            return -EAGAIN;
        }
        auto read = script.front();
        script.pop_front();
        ev = read.ev;
        return read.rc;
    }

    NextEventFn fn()
    {
        return [this](unsigned int flags, input_event& ev) {
            return next(flags, ev);
        };
    }

    std::deque<Read> script;
    std::vector<unsigned int> flagsSeen;
};

constexpr auto success = LIBEVDEV_READ_STATUS_SUCCESS;
constexpr auto resync = LIBEVDEV_READ_STATUS_SYNC;
constexpr uint16_t key = 41;

} // namespace

TEST(DrainEvents, passesEventsAndSkipsSynReport)
{
    FakeDevice dev({
        {success, makeEvent(EV_KEY, key, 1)},
        {success, makeEvent(EV_SYN, SYN_REPORT, 0)},
        {success, makeEvent(EV_KEY, key, 0)},
        {success, makeEvent(EV_SYN, SYN_REPORT, 0)},
    });

    std::vector<int32_t> values;
    drainEvents(dev.fn(), [&](const input_event& ev) {
        values.push_back(ev.value);
        return true;
    });

    EXPECT_EQ(values, (std::vector<int32_t>{1, 0}));
    EXPECT_TRUE(dev.script.empty());
}

TEST(DrainEvents, stopsWhenHandlerReturnsFalse)
{
    FakeDevice dev({
        {success, makeEvent(EV_KEY, key, 1)},
        {success, makeEvent(EV_KEY, key, 0)},
    });

    int calls = 0;
    drainEvents(dev.fn(), [&](const input_event&) {
        ++calls;
        return false;
    });

    EXPECT_EQ(calls, 1);
    EXPECT_EQ(dev.script.size(), 1U);
}

TEST(DrainEvents, stopsOnError)
{
    FakeDevice dev({
        {-ENODEV, {}},
        {success, makeEvent(EV_KEY, key, 1)},
    });

    int calls = 0;
    drainEvents(dev.fn(), [&](const input_event&) {
        ++calls;
        return true;
    });

    EXPECT_EQ(calls, 0);
}

TEST(DrainEvents, replaysMissedChangesAfterSynDropped)
{
    // The key went down, events were dropped (during which it was
    // released), then it was pressed again. libevdev reports SYN_DROPPED,
    // then in sync mode replays the release as a delta, then signals the
    // end of the sync with -EAGAIN.
    FakeDevice dev({
        {success, makeEvent(EV_KEY, key, 1)},
        {resync, makeEvent(EV_SYN, SYN_DROPPED, 0)},
        {resync, makeEvent(EV_KEY, key, 0)},
        {resync, makeEvent(EV_SYN, SYN_REPORT, 0)},
        {-EAGAIN, {}},
        {success, makeEvent(EV_KEY, key, 1)},
    });

    std::vector<int32_t> values;
    drainEvents(dev.fn(), [&](const input_event& ev) {
        values.push_back(ev.value);
        return true;
    });

    EXPECT_EQ(values, (std::vector<int32_t>{1, 0, 1}));
    EXPECT_TRUE(dev.script.empty());

    // Reads after SYN_DROPPED use sync mode until libevdev says it is done.
    EXPECT_EQ(dev.flagsSeen,
              (std::vector<unsigned int>{
                  LIBEVDEV_READ_FLAG_NORMAL, LIBEVDEV_READ_FLAG_NORMAL,
                  LIBEVDEV_READ_FLAG_SYNC, LIBEVDEV_READ_FLAG_SYNC,
                  LIBEVDEV_READ_FLAG_SYNC, LIBEVDEV_READ_FLAG_NORMAL,
                  LIBEVDEV_READ_FLAG_NORMAL}));
}
