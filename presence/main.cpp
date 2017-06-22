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

    auto objpath = (options)["path"];
    auto key = (options)["key"];
    if (argc < 3)
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
    else
    {
        rc = 0;
    }
    return rc;
}

