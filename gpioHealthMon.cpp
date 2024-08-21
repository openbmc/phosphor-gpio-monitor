/**
 * Copyright © 2024 Google
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

#include "gpioHealthMon.hpp"

#include <boost/asio/io_context.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>

#include <iostream>

namespace phosphor
{
namespace gpio
{

/* systemd service to kick start a target. */
constexpr auto DBUS_SERVICE = "xyz.openbmc_project.HealthMonitor";
constexpr auto DBUS_INTERFACE = "xyz.openbmc_project.HealthMonitor.GetStatus";

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
    bool isRising;

    if (gpiod_line_event_read_fd(gpioEventDescriptor.native_handle(),
                                 &gpioLineEvent) < 0)
    {
        lg2::error("Failed to read {GPIO} from fd", "GPIO", gpioLineMsg);
        return;
    }

    if (gpioLineEvent.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
    {
        lg2::info("{GPIO} Asserted", "GPIO", gpioLineMsg);
        isRising = true;
    }
    else
    {
        lg2::info("{GPIO} Deasserted", "GPIO", gpioLineMsg);
        isRising = false;
    }

    if ((isRising == healthyPolarity && !currentHealth) ||
        (isRising != healthyPolarity && currentHealth))
    {
        // Creating io_context
        boost::asio::io_context io;

        // Creating shared connection
        auto conn = sdbusplus::asio::connection(io);

        // Making the async D-Bus method call
        conn.async_method_call([](boost::system::error_code ec) {
            if (ec)
            {
                lg2::error("error with async_method_call: {ERROR}", "ERROR",
                           ec.message());
                return;
            }
        }, DBUS_SERVICE, objectPath, DBUS_INTERFACE, "set_healthy");
        currentHealth = !currentHealth;
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
