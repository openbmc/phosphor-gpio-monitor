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
    auto& path = (options)["path"];
    if (path == phosphor::gpio::ArgumentParser::emptyString)
    {
        exitWithError("path not specified.", argv);
    }

    // Parse out emit-code that we are interested in
    // http://lxr.free-electrons.com/source/Documentation/"
    // "devicetree/bindings/input/gpio-keys.txt
    // GPIO Key UP - 103
    // GPIO Key DOWN - 108
    auto& state = (options)["state"];
    if (state == phosphor::gpio::ArgumentParser::emptyString)
    {
        exitWithError("state not specified.", argv);
    }

    auto r = sd_event_default(&event);
    if (r < 0)
    {
        log<level::ERR>("Error creating a default sd_event handler");
        return r;
    }

    // Create a monitor object and let it do all the rest
    phosphor::gpio::Monitor monitor(path, std::stoi(state), event,
                            phosphor::gpio::Monitor::processEvents);

    // Wait for client requests until this application has processed
    // at least one expected GPIO state change
    while(!monitor.completed())
    {
        // -1 denotes wait for ever
        r = sd_event_run(event, (uint64_t)-1);
        if (r < 0)
        {
            log<level::ERR>("Failure in processing request",
                    entry("ERROR=%s", strerror(-r)));
            break;
        }
    }

    // Delete the event memory
    event = sd_event_unref(event);

    return 0;
}