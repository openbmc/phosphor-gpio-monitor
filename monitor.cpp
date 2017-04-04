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
namespace phosphor
{
namespace gpio
{

// Populate the file descriptor for passed in device
void Monitor::openDevice()
{
    using namespace phosphor::logging;

    fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        log<level::ERR>("Failed to open device",
                entry("PATH=%s", path.c_str()));
        throw std::runtime_error("Failed to open device");
    }
}

} // namespace gpio
} // namespace phosphor
