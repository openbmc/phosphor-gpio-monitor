/**
 * Copyright © 2019 Facebook
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

#include "gpioHandler.hpp"

#include <CLI/CLI.hpp>
#include <boost/asio/io_context.hpp>
#include <fstream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace gpio
{

std::map<std::string, int> gpioPolarityMap = {
    /**< Only watch falling edge events. */
    {"FALLING", GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE},
    /**< Only watch rising edge events. */
    {"RISING", GPIOD_LINE_REQUEST_EVENT_RISING_EDGE},
    /**< Monitor both types of events. */
    {"BOTH", GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES}};

const std::map<std::string, int> biasMap = {
    /**< Set bias as is. */
    {"AS_IS", 0},
    /**< Disable bias. */
    {"DISABLE", GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE},
    /**< Enable pull-up. */
    {"PULL_UP", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP},
    /**< Enable pull-down. */
    {"PULL_DOWN", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN}};
} // namespace gpio
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

    nlohmann::json monObj;
    file >> monObj;
    file.close();

    std::vector<std::unique_ptr<phosphor::gpio::GpioHandler>> gpioHandlers;

    for (auto& obj : monObj)
    {
        /* GPIO Line message */
        std::string lineMsg = "GPIO Line ";

        /* GPIO line */
        gpiod_line* line = NULL;

        /* Log message string */
        std::string errMsg;

        /* GPIO line configuration, default to monitor both edge */
        struct gpiod_line_request_config config
        {
            "gpio_monitor", GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, 0
        };

        /* flag to monitor */
        bool flag = false;

        /* Signal monitor object */
        std::unique_ptr<phosphor::gpio::Monitor> eventMonObj;

        /* Action performing object */
        std::unique_ptr<phosphor::gpio::Action> actionObj;

        /* Systemd Target Start Action Only */
        /* target to start */
        std::string target;

        /* multi targets to start */
        std::map<std::string, std::vector<std::string>> targets;

        /* Inventory Presence Monitoring Action Only */
        /* Pretty name of the inventory object */
        std::string name;

        /* Object path under inventory that will be created */
        std::string inventory;

        /* List of interfaces to associate to inventory item */
        std::vector<std::string> extraInterfaces;

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

        if (line == NULL)
        {
            lg2::error("Failed to find the {GPIO}", "GPIO", errMsg);
            return -1;
        }

        /* Parse optional bias */
        if (obj.find("Bias") != obj.end())
        {
            std::string biasName = obj["Bias"].get<std::string>();
            auto findBias = phosphor::gpio::biasMap.find(biasName);
            if (findBias == phosphor::gpio::biasMap.end())
            {
                lg2::error("{GPIO}: Bias unknown: {BIAS}", "GPIO", lineMsg,
                           "BIAS", biasName);
                return -1;
            }

            config.flags = findBias->second;
        }

        /* Parse optional active level */
        if (obj.find("ActiveLow") != obj.end() && obj["ActiveLow"].get<bool>())
        {
            config.flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
        }

        /* If not monitoring inventory presence, default to systemd target
         * action as no target(s) are required to be specified.
         */
        if (obj.find("Inventory") == obj.end() || obj.find("Name") == obj.end())
        {
            /* Get flag if monitoring needs to continue after first event */
            if (obj.find("Continue") != obj.end())
            {
                flag = obj["Continue"];
            }

            /* Get event to be monitored */
            if (obj.find("EventMon") != obj.end())
            {
                std::string eventStr = obj["EventMon"];

                auto findEvent = phosphor::gpio::gpioPolarityMap.find(eventStr);
                if (findEvent == phosphor::gpio::gpioPolarityMap.end())
                {
                    lg2::error("{GPIO}: event missing: {EVENT}", "GPIO",
                               lineMsg, "EVENT", eventStr);
                    return -1;
                }
                /* If it is not defined then both rising falling edge will
                 * be monitored
                 */
                config.request_type = findEvent->second;
            }

            /* Parse out target argument. It is fine if the user does not
             * pass this if they are not interested in calling into any target
             * on meeting a condition.
             */
            if (obj.find("Target") != obj.end())
            {
                target = obj["Target"];
            }

            /* Parse out the targets argument if multi-targets are needed.
             * Change the key to be generic regardless of whether pulse or gpio
             * monitoring.
             */
            if (obj.find("Targets") != obj.end())
            {
                auto eventType = "RISING";
                obj.at("Targets").get_to(targets);
                auto assertTargets = targets.extract(eventType);
                if (!assertTargets.empty())
                {
                    assertTargets.key() = phosphor::gpio::assertedKeyword;
                    targets.insert(std::move(assertTargets));
                }

                eventType = "FALLING";
                auto deassertTargets = targets.extract(eventType);
                if (!deassertTargets.empty())
                {
                    deassertTargets.key() = phosphor::gpio::deassertedKeyword;
                    targets.insert(std::move(deassertTargets));
                }
            }

            actionObj = std::make_unique<phosphor::gpio::SystemdAction>(
                target, targets);
        }
        else
        {
            /* Always monitor for presence */
            flag = true;
            inventory = obj["Inventory"].get<std::string>();
            name = obj["Name"].get<std::string>();

            /* Parse optional extra interfaces */
            if (obj.find("ExtraInterfaces") != obj.end())
            {
                obj.at("ExtraInterfaces").get_to(extraInterfaces);
            }

            actionObj = std::make_unique<phosphor::gpio::InventoryAction>(
                inventory, extraInterfaces, name);
        }

        eventMonObj = std::make_unique<phosphor::gpio::GpioEdgeMonitor>(
            line, config, io, lineMsg, flag);

        gpioHandlers.push_back(std::make_unique<phosphor::gpio::GpioHandler>(
            eventMonObj, actionObj));
    }
    io.run();

    return 0;
}
