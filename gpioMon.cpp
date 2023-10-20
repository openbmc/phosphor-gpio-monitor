/**
 * Copyright © 2019 Facebook
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gpioMon.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace gpio
{

/* systemd service to kick start a target. */
constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_ROOT = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

constexpr auto falling = "FALLING";
constexpr auto rising = "RISING";

void GpioMonitor::scheduleEventHandler()
{
    gpioEventDescriptor.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [this](const boost::system::error_code& ec) {
        if (ec)
        {
            lg2::error("{GPIO} event handler error: {ERROR}", "GPIO",
                       gpioLineMsg, "ERROR", ec.message());
            return;
        }
        gpioEventHandler();
    });
}

void GpioMonitor::gpioEventHandler()
{
    gpiod_line_event gpioLineEvent;

    if (gpiod_line_event_read_fd(gpioEventDescriptor.native_handle(),
                                 &gpioLineEvent) < 0)
    {
        lg2::error("Failed to read {GPIO} from fd", "GPIO", gpioLineMsg);
        return;
    }

    if (gpioLineEvent.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
    {
        lg2::info("{GPIO} Asserted", "GPIO", gpioLineMsg);
    }
    else
    {
        lg2::info("{GPIO} Deasserted", "GPIO", gpioLineMsg);
    }

    /* Execute the target if it is defined. */
    if (!target.empty())
    {
        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_ROOT,
                                          SYSTEMD_INTERFACE, "StartUnit");
        method.append(target);
        method.append("replace");

        bus.call_noreply(method);
    }

    std::vector<std::string> targetsToStart;
    if (gpioLineEvent.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
    {
        auto risingFind = targets.find(rising);
        if (risingFind != targets.end())
        {
            targetsToStart = risingFind->second;
        }
    }
    else
    {
        auto fallingFind = targets.find(falling);
        if (fallingFind != targets.end())
        {
            targetsToStart = fallingFind->second;
        }
    }

    /* Execute the multi targets if it is defined. */
    if (!targetsToStart.empty())
    {
        auto bus = sdbusplus::bus::new_default();
        for (auto& tar : targetsToStart)
        {
            auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_ROOT,
                                              SYSTEMD_INTERFACE, "StartUnit");
            method.append(tar, "replace");
            bus.call_noreply(method);
        }
    }

    /* if not required to continue monitoring then return */
    if (!continueAfterEvent)
    {
        return;
    }

    /* Schedule a wait event */
    scheduleEventHandler();
}

int GpioMonitor::requestGPIOEvents()
{
    /* Request an event to monitor for respected gpio line */
    if (gpiod_line_request(gpioLine, &gpioConfig, 0) < 0)
    {
        lg2::error("Failed to request {GPIO}", "GPIO", gpioLineMsg);
        return -1;
    }

    int gpioLineFd = gpiod_line_event_get_fd(gpioLine);
    if (gpioLineFd < 0)
    {
        lg2::error("Failed to get fd for {GPIO}", "GPIO", gpioLineMsg);
        return -1;
    }

    lg2::info("{GPIO} monitoring started", "GPIO", gpioLineMsg);

    /* Assign line fd to descriptor for monitoring */
    gpioEventDescriptor.assign(gpioLineFd);

    /* Schedule a wait event */
    scheduleEventHandler();

    return 0;
}
} // namespace gpio
} // namespace phosphor
