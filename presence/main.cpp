#include <iostream>
#include <phosphor-logging/log.hpp>
#include "argument.hpp"
#include "gpio_presence.hpp"

using namespace phosphor::logging;
using namespace phosphor::gpio;

int main(int argc, char* argv[])
{
    auto options = ArgumentParser(argc, argv);

    auto inventory = options["inventory"];
    auto key = options["key"];
    auto path = options["path"];
    if (argc < 4)
    {
        std::cerr << "Too few arguments\n";
        options.usage(argv);
    }

    if (inventory == ArgumentParser::emptyString)
    {
        std::cerr << "Inventory argument required\n";
        options.usage(argv);
    }

    if (key == ArgumentParser::emptyString)
    {
        std::cerr << "GPIO key argument required\n";
        options.usage(argv);
    }

    if (path == ArgumentParser::emptyString)
    {
        std::cerr << "Device path argument required\n";
        options.usage(argv);
    }

    return 0;
}

