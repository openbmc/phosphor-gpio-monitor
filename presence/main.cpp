#include <iostream>
#include <phosphor-logging/log.hpp>
#include "argument.hpp"
#include "gpio_presence.hpp"

using namespace phosphor::logging;
using namespace phosphor::gpio;

int main(int argc, char* argv[])
{
    auto rc = -1;
    auto options = ArgumentParser(argc, argv);

    auto objpath = options["path"];
    auto key = options["key"];
    auto dev = options["dev"];
    if (argc < 4)
    {
        std::cerr << "Too few arguments\n";
        options.usage(argv);
    }
    else if (objpath == ArgumentParser::emptyString)
    {
        std::cerr << "Path argument required\n";
        options.usage(argv);
    }
    else if (key == ArgumentParser::emptyString)
    {
        std::cerr << "GPIO key argument required\n";
        options.usage(argv);
    }
    else if (dev == ArgumentParser::emptyString)
    {
        std::cerr << "Device argument required\n";
        options.usage(argv);
    }
    else
    {
        rc = 0;
    }
    return rc;
}

