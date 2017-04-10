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

#include <fcntl.h>
#include <phosphor-logging/log.hpp>
#include "monitor.hpp"
#include "config.h"

namespace phosphor
{
namespace gpio
{

// systemd service to kick start a target.
constexpr auto SYSTEMD_SERVICE        = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_ROOT           = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE      = "org.freedesktop.systemd1.Manager";

using namespace phosphor::logging;

// Populate the file descriptor for passed in device
int Monitor::openDevice()
{
    using namespace phosphor::logging;

    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        log<level::ERR>("Failed to open device",
                entry("PATH=%s", path.c_str()));
        throw std::runtime_error("Failed to open device");
    }
    return fd;
}

// Attaches the FD to event loop and registers the callback handler
void Monitor::registerCallback()
{
    auto sourcePtr = eventSource.get();
    auto r = sd_event_add_io(event.get(), &sourcePtr, (fd)(),
                             EPOLLIN, callbackHandler, this);
    if (r < 0)
    {
        log<level::ERR>("Failed to register callback handler",
                entry("ERROR=%s", strerror(-r)));
        throw std::runtime_error("Failed to register callback handler");
    }
}

// Initializes the event device with the fd
void Monitor::initEvDev()
{
    if (device)
    {
        // Init can be done only once per device
        return;
    }

    struct libevdev* evdev = nullptr;
    auto rc = libevdev_new_from_fd((fd)(), &evdev);
    if (rc < 0)
    {
        log<level::ERR>("Failed to initialize evdev");
        throw std::runtime_error("Failed to initialize evdev");
    }

    // Packing in the unique_ptr
    device.reset(evdev);
    evdev = nullptr;
}

// Callback handler when there is an activity on the FD
int Monitor::processEvents(sd_event_source* es, int fd,
                           uint32_t revents, void* userData)
{
    log<level::INFO>("GPIO line altered");
    auto monitor = static_cast<Monitor*>(userData);

    // Initialize libevdev for this. Doing it here enables
    // gtest to use this infrastructure on arbitrary device
    // than /dev/input/
    monitor->initEvDev();
    monitor->analyzeEvent();
    return 0;
}

// Analyzes the GPIO event
void Monitor::analyzeEvent()
{
    // Data returned
    struct input_event ev{};
    int rc = 0;

    // While testing, observed that not having a loop here was leading
    // into events being missed.
    while (rc >= 0)
    {
        // Wait until no more events are available on the device.
        rc = libevdev_next_event(device.get(),
                                 LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc < 0)
        {
            // There was an error waiting for events, mostly that there are no
            // events to be read.. So continue waiting...
            return;
        };

        if (rc == LIBEVDEV_READ_STATUS_SUCCESS)
        {
            if (ev.type == EV_SYN && ev.code == SYN_REPORT)
            {
                continue;
            }
            else if (ev.code == key && ev.value == polarity)
            {
                // If the code/value is what we are interested in, declare done.
                // User supplied systemd unit
                if (!target.empty())
                {
                    auto bus = sdbusplus::bus::new_default();
                    auto method = bus.new_method_call(SYSTEMD_SERVICE,
                                                      SYSTEMD_ROOT,
                                                      SYSTEMD_INTERFACE,
                                                      "StartUnit");
                    method.append(target);
                    method.append("replace");

                    bus.call_noreply(method);
                }

                // This marks the completion of handling the gpio assertion
                // and the app can exit
                complete = true;
                return;
            }
        }
    };

    return;
}

} // namespace gpio
} // namespace phosphor
