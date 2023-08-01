/**
 * Copyright © 2017 IBM Corporation
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
#include "gpio.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>

#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <cstring>

namespace phosphor
{
namespace gpio
{

void GPIO::set(Value value)
{
    assert(direction == Direction::output);

    requestLine(value);

    gpiohandle_data data{};
    data.values[0] = static_cast<gpioValue_t>(value);

    auto rc = ioctl(lineFD(), GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    if (rc == -1)
    {
        lg2::error("Failed SET_LINE_VALUES ioctl: {ERRNO}", "ERRNO", errno);
        throw std::runtime_error("Failed SET_LINE_VALUES ioctl");
    }
}

void GPIO::requestLine(Value defaultValue)
{
    // Only need to do this once
    if (lineFD)
    {
        return;
    }

    FileDescriptor fd{open(device.c_str(), 0)};
    if (fd() == -1)
    {
        lg2::error("Failed opening {DEVICE}: {ERRNO}", "DEVICE", device,
                   "ERRNO", errno);
        throw std::runtime_error("Failed opening GPIO device");
    }

    // Make an ioctl call to request the GPIO line, which will
    // return the descriptor to use to access it.
    gpiohandle_request request{};
    strncpy(request.consumer_label, "phosphor-gpio-util",
            sizeof(request.consumer_label));

    request.flags = (direction == Direction::output) ? GPIOHANDLE_REQUEST_OUTPUT
                                                     : GPIOHANDLE_REQUEST_INPUT;

    request.lineoffsets[0] = gpio;
    request.lines = 1;

    if (direction == Direction::output)
    {
        request.default_values[0] = static_cast<gpioValue_t>(defaultValue);
    }

    auto rc = ioctl(fd(), GPIO_GET_LINEHANDLE_IOCTL, &request);
    if (rc == -1)
    {
        lg2::error("Failed GET_LINEHANDLE ioctl {GPIO}: {ERRNO}", "GPIO", gpio,
                   "ERRNO", errno);
        throw std::runtime_error("Failed GET_LINEHANDLE ioctl");
    }

    lineFD.set(request.fd);
}

} // namespace gpio
} // namespace phosphor
