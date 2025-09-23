// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2019 Facebook

#include "gpioMon.hpp"

#include <CLI/CLI.hpp>
#include <boost/asio/io_context.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <fstream>

namespace phosphor
{
namespace gpio
{

std::map<std::string, int> polarityMap = {
    /**< Only watch falling edge events. */
    {"FALLING", GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE},
    /**< Only watch rising edge events. */
    {"RISING", GPIOD_LINE_REQUEST_EVENT_RISING_EDGE},
    /**< Monitor both types of events. */
    {"BOTH", GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES}};

}
} // namespace phosphor

int main(int argc, char** argv)
{
    boost::asio::io_context io;

    CLI::App app{"Monitor GPIO line for requested state change"};

    std::string gpioFileName;

    /* Add an input option */
    app.add_option("-c,--config", gpioFileName, "Name of config json file")
        ->required()
        ->check(CLI::ExistingFile);

    /* Parse input parameter */
    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::Error& e)
    {
        return app.exit(e);
    }

    /* Get list of gpio config details from json file */
    std::ifstream file(gpioFileName);
    if (!file)
    {
        lg2::error("GPIO monitor config file not found: {FILE}", "FILE",
                   gpioFileName);
        return -1;
    }

    nlohmann::json gpioMonObj;
    file >> gpioMonObj;
    file.close();

    std::vector<std::unique_ptr<phosphor::gpio::GpioMonitor>> gpios;

    for (auto& obj : gpioMonObj)
    {
        /* GPIO Line message */
        std::string lineMsg = "GPIO Line ";

        /* GPIO line */
        gpiod_line* line = nullptr;

        /* GPIO line configuration, default to monitor both edge */
        struct gpiod_line_request_config config{
            "gpio_monitor", GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, 0};

        /* flag to monitor */
        bool flag = false;

        /* target to start */
        std::string target;

        /* multi targets to start */
        std::map<std::string, std::vector<std::string>> targets;

        if (obj.find("LineName") == obj.end())
        {
            /* If there is no line Name defined then gpio num nd chip
             * id must be defined. GpioNum is integer mapping to the
             * GPIO key configured by the kernel
             */
            if (obj.find("GpioNum") == obj.end() ||
                obj.find("ChipId") == obj.end())
            {
                lg2::error("Failed to find line name or gpio number: {FILE}",
                           "FILE", gpioFileName);
                return -1;
            }

            std::string chipIdStr = obj["ChipId"];
            int gpioNum = obj["GpioNum"];

            lineMsg += std::to_string(gpioNum);

            /* Get the GPIO line */
            line = gpiod_line_get(chipIdStr.c_str(), gpioNum);
        }
        else
        {
            /* Find the GPIO line */
            std::string lineName = obj["LineName"];
            lineMsg += lineName;
            line = gpiod_line_find(lineName.c_str());
        }

        if (line == nullptr)
        {
            lg2::error("Failed to find the {GPIO}", "GPIO", lineMsg);
            continue;
        }

        /* Get event to be monitored, if it is not defined then
         * Both rising falling edge will be monitored.
         */
        if (obj.find("EventMon") != obj.end())
        {
            std::string eventStr = obj["EventMon"];
            auto findEvent = phosphor::gpio::polarityMap.find(eventStr);
            if (findEvent == phosphor::gpio::polarityMap.end())
            {
                lg2::error("{GPIO}: event missing: {EVENT}", "GPIO", lineMsg,
                           "EVENT", eventStr);
                return -1;
            }

            config.request_type = findEvent->second;
        }

        /* Get flag if monitoring needs to continue after first event */
        if (obj.find("Continue") != obj.end())
        {
            flag = obj["Continue"];
        }

        /* Parse out target argument. It is fine if the user does not
         * pass this if they are not interested in calling into any target
         * on meeting a condition.
         */
        if (obj.find("Target") != obj.end())
        {
            target = obj["Target"];
        }

        /* Parse out the targets argument if multi-targets are needed.*/
        if (obj.find("Targets") != obj.end())
        {
            obj.at("Targets").get_to(targets);
        }

        /* Create a monitor object and let it do all the rest */
        gpios.push_back(std::make_unique<phosphor::gpio::GpioMonitor>(
            line, config, io, target, targets, lineMsg, flag));
    }
    io.run();

    return 0;
}
