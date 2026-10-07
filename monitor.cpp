// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2016 IBM Corporation

#include "monitor.hpp"

#include <fcntl.h>

#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace gpio
{

// systemd service to kick start a target.
constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_ROOT = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

// Callback handler when there is an activity on the FD
int Monitor::processEvents(sd_event_source*, int, uint32_t, void* userData)
{
    lg2::info("GPIO line altered");
    auto monitor = static_cast<Monitor*>(userData);

    monitor->analyzeEvent();
    return 0;
}

void Monitor::checkKeySupported() const
{
    if (devicePtr && !libevdev_has_event_code(devicePtr.get(), EV_KEY, key))
    {
        lg2::error("{DEVICEPATH} does not report EV_KEY code {KEY}, the "
                   "configured GPIO will never be seen",
                   "DEVICEPATH", path, "KEY", key);
    }
}

// Analyzes the GPIO event
void Monitor::analyzeEvent()
{
    readEvents([this](const input_event& ev) {
        if (!isKeyEvent(ev, key) || ev.value != polarity)
        {
            return true;
        }

        // If the code/value is what we are interested in, declare done.
        // User supplied systemd unit
        if (!target.empty())
        {
            auto bus = sdbusplus::bus::new_default();
            auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_ROOT,
                                              SYSTEMD_INTERFACE, "StartUnit");
            method.append(target);
            method.append("replace");

            bus.call_noreply(method);
        }

        if (!continueAfterKeyPress)
        {
            // This marks the completion of handling the gpio assertion
            // and the app can exit
            complete = true;
        }
        return false;
    });
}

} // namespace gpio
} // namespace phosphor
