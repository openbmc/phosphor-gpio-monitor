#include <iostream>
#include <systemd/sd-event.h>
#include <phosphor-logging/log.hpp>
#include "argument.hpp"
#include "gpio_presence.hpp"

using namespace phosphor::logging;
using namespace phosphor::gpio;
using namespace phosphor::gpio::presence;

int main(int argc, char* argv[])
{
    auto rc = -1;
    auto options = ArgumentParser(argc, argv);

    auto objpath = (options)["path"];
    auto key = (options)["key"];
    auto dev = (options)["dev"];
    if (argc < 4)
    {
        std::cerr << std::endl << "Too few arguments" << std::endl;
        log<level::ERR>("Too few arguments");
        options.usage(argv);
    }
    else if (objpath == ArgumentParser::emptyString)
    {
        std::cerr << std::endl << "Path argument required" << std::endl;
        log<level::ERR>("Path argument required");
        options.usage(argv);
    }
    else if (key == ArgumentParser::emptyString)
    {
        std::cerr << std::endl << "GPIO key argument required" << std::endl;
        log<level::ERR>("GPIO key argument required");
        options.usage(argv);
    }
    else if (dev == ArgumentParser::emptyString)
    {
        std::cerr << std::endl << "Device argument required" << std::endl;
        log<level::ERR>("Device argument required");
        options.usage(argv);
    }
    else
    {
        auto bus = sdbusplus::bus::new_default();

        sd_event* event = nullptr;
        rc = sd_event_default(&event);
        if (rc < 0)
        {
            log<level::ERR>("Error creating a default sd_event handler");
            return rc;
        }
        EventPtr eventP{event};
        event = nullptr;

        auto name = (options)["name"];
        Presence presence(bus, objpath, dev, std::stoul(key), name, eventP);

        while (true)
        {
            // -1 denotes wait forever
            rc = sd_event_run(eventP.get(), (uint64_t) - 1);
            if (rc < 0)
            {
                log<level::ERR>("Failure in processing request",
                                entry("ERROR=%s", strerror(-rc)));
            }
        }
    }
    return rc;
}

