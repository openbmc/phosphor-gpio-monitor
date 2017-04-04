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

#include <iostream>
#include <string>
#include <systemd/sd-event.h>
#include <phosphor-logging/log.hpp>
#include "argument.hpp"
#include "monitor.hpp"

using namespace phosphor::logging;
static void exitWithError(const char* err, char** argv)
{
    phosphor::gpio::ArgumentParser::usage(argv);
    std::cerr << "ERROR: " << err << "\n";
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    sd_event* event = nullptr;

    // Read arguments.
    auto options = phosphor::gpio::ArgumentParser(argc, argv);

    // Parse out path argument.
    auto path = (options)["path"];
    if (path == phosphor::gpio::ArgumentParser::emptyString)
    {
        exitWithError("path not specified.", argv);
    }

    // Parse out emit-code that we are interested in
    // http://lxr.free-electrons.com/source/Documentation/"
    // "devicetree/bindings/input/gpio-keys.txt
    // GPIO Key UP - 103
    // GPIO Key DOWN - 108
    auto state = (options)["state"];
    if (state == phosphor::gpio::ArgumentParser::emptyString)
    {
        exitWithError("state not specified.", argv);
    }

    // Parse out target argument. It is fine if the caller does not
    // pass this if they are not interested in calling into any target
    // on meeting a condition.
    auto target = (options)["target"];

    auto r = sd_event_default(&event);
    if (r < 0)
    {
        log<level::ERR>("Error creating a default sd_event handler");
        return r;
    }
    phosphor::gpio::eventPtr eventP;
    eventP.reset(event);
    event = nullptr;

    // Create a monitor object and let it do all the rest
    phosphor::gpio::Monitor monitor(path, std::stoi(state),
                                    target, eventP,
                                    phosphor::gpio::Monitor::processEvents);
    // Wait for events
    sd_event_loop(eventP.get());

    return 0;
}
