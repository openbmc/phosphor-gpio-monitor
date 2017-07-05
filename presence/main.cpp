#include <iostream>
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
        auto name = (options)["name"];
        Presence presence(bus, objpath, dev, std::stoul(key), name);

        rc = 0;
    }
    return rc;
}

