// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2016 IBM Corporation

#include "monitor.hpp"

#include <systemd/sd-event.h>

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>

#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    CLI::App app{"Monitor GPIO line for requested state change"};

    // Read arguments.
    std::string path{};
    std::string key{};
    std::string polarity{};
    std::string target{};
    bool continueRun = false;

    /* Add an input option */
    app.add_option("-p,--path", path,
                   "Path of input device. Ex: /dev/input/event2")
        ->required();
    app.add_option("-k,--key", key, "Input GPIO key number")->required();
    app.add_option("-r,--polarity", polarity,
                   "Asertion polarity to look for. This is 0 / 1")
        ->required();
    app.add_option("-t,--target", target,
                   "Systemd unit to be called on GPIO state change")
        ->required();
    app.add_flag("-c,--continue", continueRun,
                 "Whether or not to continue after key pressed");

    /* Due to the way this process is loaded from systemd, we can end up with
     * an empty extra parameter. Go ahead and ignore it. */
    app.allow_extras();

    /* Parse input parameter */
    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::Error& e)
    {
        return app.exit(e);
    }

    sd_event* event = nullptr;
    auto r = sd_event_default(&event);
    if (r < 0)
    {
        lg2::error("Error creating a default sd_event handler");
        return r;
    }
    phosphor::gpio::EventPtr eventP{event};
    event = nullptr;

    // Create a monitor object and let it do all the rest
    phosphor::gpio::Monitor monitor(path, std::stoi(key), std::stoi(polarity),
                                    target, eventP, continueRun);

    // Wait for client requests until this application has processed
    // at least one expected GPIO state change
    while (!monitor.completed())
    {
        // -1 denotes wait for ever
        r = sd_event_run(eventP.get(), (uint64_t)-1);
        if (r < 0)
        {
            lg2::error("Failure in processing request: {RC}", "RC", r);
            return r;
        }
    }

    return 0;
}
