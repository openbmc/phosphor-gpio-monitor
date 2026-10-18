// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include <systemd/sd-event.h>
#include <time.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

namespace phosphor
{
namespace gpio
{

/** @class DelayedAction
 *  @brief Runs a callback once, after a delay, from the sd_event loop
 *         without blocking it in the meantime.
 */
class DelayedAction
{
  public:
    DelayedAction() = delete;
    ~DelayedAction() = default;
    DelayedAction(const DelayedAction&) = delete;
    DelayedAction& operator=(const DelayedAction&) = delete;
    DelayedAction(DelayedAction&&) = delete;
    DelayedAction& operator=(DelayedAction&&) = delete;

    /** @brief Constructs DelayedAction object.
     *
     *  @param[in] event  - sd_event loop the action runs on
     *  @param[in] action - Callback to run when the delay expires
     */
    DelayedAction(sd_event* event, std::function<void()> action) :
        event(event), action(std::move(action))
    {}

    /** @brief Runs the action after the delay. Does nothing if the action
     *         is already scheduled.
     *
     *  @param[in] delay - Time to wait before running the action
     *
     *  @return 0 on success or a negative errno if the timer could not be
     *          created
     */
    int schedule(std::chrono::microseconds delay)
    {
        if (pending())
        {
            return 0;
        }

        sd_event_source* source = nullptr;
        // 1us accuracy so sd-event doesn't add its default 250ms of slack.
        auto rc = sd_event_add_time_relative(event, &source, CLOCK_MONOTONIC,
                                             delay.count(), 1, expired, this);
        if (rc < 0)
        {
            return rc;
        }
        timer.reset(source);
        return 0;
    }

    /** @brief Cancels the action if it has not run yet.
     *
     *  @return true if a scheduled action was canceled
     */
    bool cancel()
    {
        auto wasPending = pending();
        timer.reset();
        return wasPending;
    }

    /** @brief Returns whether the action is scheduled and has not run yet */
    bool pending() const
    {
        return timer != nullptr;
    }

  private:
    /** @brief Timer callback. The timer is released before the action
     *         runs, so pending() is false inside it and the action may
     *         schedule itself again.
     */
    static int expired(sd_event_source*, uint64_t, void* userData)
    {
        auto self = static_cast<DelayedAction*>(userData);
        self->timer.reset();
        self->action();
        return 0;
    }

    /** @brief sd_event loop the timer is attached to */
    sd_event* event;

    /** @brief Callback to run when the timer expires */
    std::function<void()> action;

    /** @brief One-shot timer, set while the action is pending */
    std::unique_ptr<sd_event_source, decltype(&sd_event_source_unref)> timer{
        nullptr, sd_event_source_unref};
};

} // namespace gpio
} // namespace phosphor
