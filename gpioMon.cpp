/**
 * Copyright © 2016 IBM Corporation
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

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace gpio
{

/* systemd service to kick start a target. */
constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_ROOT = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

using namespace phosphor::logging;

void GpioMonitor::gpioEventHandler()
{
    gpiod_line_event gpioLineEvent;
    auto findLineName = gpiod_line_name(gpioLine);
    std::string gpioLineMsg = "GPIO Line: ";

    if (findLineName == NULL)
    {
        gpioLineMsg += std::to_string(gpiod_line_offset(gpioLine));
    }
    else
    {
        gpioLineMsg += findLineName;
    }

    if (gpiod_line_event_read_fd(gpioEventDescriptor.native_handle(),
                                 &gpioLineEvent) < 0)
    {
        log<level::ERR>("Failed to read gpioLineEvent from fd");
        return;
    }

    std::string logMessage =
        "GPIO line " + gpioLineMsg +
        (gpioLineEvent.event_type == GPIOD_LINE_EVENT_RISING_EDGE
             ? " Asserted"
             : " Deasserted");

    log<level::INFO>(logMessage.c_str());

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

    /* if cnot required to continue monitoring then return */
    if (!continueAfterKeyPress)
    {
        return;
    }

    gpioEventDescriptor.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [this](const boost::system::error_code& ec) {
            if (ec)
            {
                log<level::ERR>("GPIO event handler error",
                                entry("ERR_MSG=%s", ec.message().c_str()));
                return;
            }
            gpioEventHandler();
        });
}

int GpioMonitor::requestGPIOEvents()
{

    std::string errMsg;
    auto findLineName = gpiod_line_name(gpioLine);
    std::string gpioLineMsg = "GPIO Line: ";

    if (findLineName == NULL)
    {
        gpioLineMsg += std::to_string(gpiod_line_offset(gpioLine));
    }
    else
    {
        gpioLineMsg += findLineName;
    }

    /* Request an event to monitor for respected gpio line */
    if (gpiod_line_request(gpioLine, &gpioConfig, 0) < 0)
    {
        errMsg = "Failed to request events for " + gpioLineMsg;
        log<level::ERR>(errMsg.c_str());
        return -1;
    }

    int gpioLineFd = gpiod_line_event_get_fd(gpioLine);
    if (gpioLineFd < 0)
    {
        errMsg = "Failed to get fd for " + gpioLineMsg;
        log<level::ERR>(errMsg.c_str());
        return -1;
    }

    gpioEventDescriptor.assign(gpioLineFd);

    errMsg = gpioLineMsg + " fd handler error: ";

    gpioEventDescriptor.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [errMsg, this](const boost::system::error_code& ec) {
            if (ec)
            {
                std::string msg = errMsg + std::string(ec.message());
                log<level::ERR>(msg.c_str());
                return;
            }
            gpioEventHandler();
        });
    return 0;
}
} // namespace gpio
} // namespace phosphor
