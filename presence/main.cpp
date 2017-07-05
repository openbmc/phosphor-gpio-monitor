#include <iostream>
#include <phosphor-logging/log.hpp>
#include "argument.hpp"
#include "gpio_presence.hpp"

using namespace phosphor::logging;
using namespace phosphor::gpio;
using namespace phosphor::gpio::presence;

int main(int argc, char* argv[])
{
    auto options = ArgumentParser(argc, argv);

    auto objpath = options["path"];
    auto key = options["key"];
    auto dev = options["dev"];
    if (argc < 4)
    {
        std::cerr << "Too few arguments\n";
        options.usage(argv);
    }

    if (objpath == ArgumentParser::emptyString)
    {
        std::cerr << "Path argument required\n";
        options.usage(argv);
    }

    if (key == ArgumentParser::emptyString)
    {
        std::cerr << "GPIO key argument required\n";
        options.usage(argv);
    }

    if (dev == ArgumentParser::emptyString)
    {
        std::cerr << "Device argument required\n";
        options.usage(argv);
    }

    auto bus = sdbusplus::bus::new_default();
    auto name = options["name"];
    Presence presence(bus, objpath, dev, std::stoul(key), name);

    return 0;
}

