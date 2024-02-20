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

#include "gpioEdgeMon.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace gpio
{

void GpioEdgeMonitor::scheduleEventHandler()
{
    gpioEventDescriptor.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [this](const boost::system::error_code& ec) {
        if (ec)
        {
            /* Monitoring was not cancelled */
            if (ec != boost::asio::error::operation_aborted)
            {
                lg2::error("{GPIO} event handler error: {ERROR}", "GPIO", msg,
                           "ERROR", ec.message());
            }
            return;
        }

        handleGpioEvent();
    });
}

void GpioEdgeMonitor::handleGpioEvent()
{
    gpiod_line_event gpioLineEvent;

    if (gpiod_line_event_read_fd(gpioEventDescriptor.native_handle(),
                                 &gpioLineEvent) < 0)
    {
        lg2::error("Failed to read {GPIO} from fd", "GPIO", msg);
        return;
    }

    if (verbose)
    {
        if (gpioLineEvent.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
        {
            lg2::info("{GPIO} Asserted", "GPIO", msg);
        }
        else
        {
            lg2::info("{GPIO} Deasserted", "GPIO", msg);
        }
    }
    performEventAction(gpioLineEvent.event_type);

    /* if not required to continue monitoring then return */
    if (!continueAfterEvent)
    {
        return;
    }

    /* Schedule a wait event */
    scheduleEventHandler();
}

int GpioEdgeMonitor::startMonitoring()
{
    std::string flags;

    /* The GPIO should be set up for monitoring only once */
    if (gpioEventDescriptor.is_open())
    {
        lg2::error("{GPIO} was already initialized for monitoring.", "GPIO",
                   msg);
        return -1;
    }

    /* Request an event to monitor for respected gpio line */
    if (gpiod_line_request(gpioLine, &gpioConfig, 0) < 0)
    {
        lg2::error("Failed to request {GPIO}: {ERRNO}", "GPIO", msg, "ERRNO",
                   errno);
        return -1;
    }

    int gpioLineFd = gpiod_line_event_get_fd(gpioLine);
    if (gpioLineFd < 0)
    {
        lg2::error("Failed to get fd for {GPIO}", "GPIO", msg);
        return -1;
    }

    if (gpioConfig.flags & GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE)
    {
        flags += " Bias DISABLE";
    }
    else if (gpioConfig.flags & GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP)
    {
        flags += " Bias PULL_UP";
    }
    else if (gpioConfig.flags & GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN)
    {
        flags += " Bias PULL_DOWN";
    }

    if (gpioConfig.flags & GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW)
    {
        flags += " ActiveLow";
    }

    if (!flags.empty())
    {
        flags = "[" + flags + "]";
    }

    lg2::info("{GPIO} {FLAGS} monitoring started", "GPIO", msg, "FLAGS", flags);

    /* Assign line fd to descriptor for monitoring */
    gpioEventDescriptor.assign(gpioLineFd);

    /* Get line value and convert to expect input */
    performInitEventAction((gpiod_line_get_value(gpioLine) == 1)
                               ? GPIOD_LINE_EVENT_RISING_EDGE
                               : GPIOD_LINE_EVENT_FALLING_EDGE);

    /* Schedule a wait event */
    scheduleEventHandler();

    return 0;
}

void GpioEdgeMonitor::stopMonitoring()
{
    gpioEventDescriptor.cancel();
    Monitor::stopMonitoring();
}
} // namespace gpio
} // namespace phosphor
