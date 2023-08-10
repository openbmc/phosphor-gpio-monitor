/**
 * Copyright © 2019 Facebook
 * Copyright © 2023 9elements GmbH
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

#include "gpio_presence.hpp"

#include <CLI/CLI.hpp>
#include <boost/asio/io_context.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <fstream>

namespace phosphor
{
namespace gpio
{

const std::map<std::string, int> biasMap = {
    /**< Set bias as is. */
    {"AS_IS", 0},
    /**< Disable bias. */
    {"DISABLE", GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE},
    /**< Enable pull-up. */
    {"PULL_UP", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP},
    /**< Enable pull-down. */
    {"PULL_DOWN", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN}};
}
} // namespace phosphor

int main(int argc, char** argv)
{
    boost::asio::io_context io;

    CLI::App app{"Monitor gpio presence status"};

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

    std::vector<std::unique_ptr<phosphor::gpio::GpioPresence>> gpios;

    for (auto& obj : gpioMonObj)
    {
        /* GPIO Line message */
        std::string lineMsg = "GPIO Line ";

        /* GPIO line */
        gpiod_line* line = NULL;

        /* GPIO line configuration, default to monitor both edge */
        struct gpiod_line_request_config config
        {
            "gpio_monitor", GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, 0
        };

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

            std::string chipIdStr = obj["ChipId"].get<std::string>();
            int gpioNum = obj["GpioNum"].get<int>();

            lineMsg += chipIdStr + " " + std::to_string(gpioNum);

            /* Get the GPIO line */
            line = gpiod_line_get(chipIdStr.c_str(), gpioNum);
        }
        else
        {
            /* Find the GPIO line */
            std::string lineName = obj["LineName"].get<std::string>();
            lineMsg += lineName;
            line = gpiod_line_find(lineName.c_str());
        }

        if (line == NULL)
        {
            lg2::error("Failed to find the {GPIO}", "GPIO", lineMsg);
            return -1;
        }

        /* Parse out inventory argument. */
        if (obj.find("Inventory") == obj.end())
        {
            lg2::error("{GPIO}: Inventory path not specified", "GPIO", lineMsg);
            return -1;
        }
        else
        {
            inventory = obj["Inventory"].get<std::string>();
        }

        if (obj.find("Name") == obj.end())
        {
            lg2::error("{GPIO}: Name path not specified", "GPIO", lineMsg);
            return -1;
        }
        else
        {
            name = obj["Name"].get<std::string>();
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

        /* Parse optional extra interfaces */
        if (obj.find("ExtraInterfaces") != obj.end())
        {
            obj.at("ExtraInterfaces").get_to(extraInterfaces);
        }

        /* Create a monitor object and let it do all the rest */
        gpios.push_back(std::make_unique<phosphor::gpio::GpioPresence>(
            line, config, io, inventory, extraInterfaces, lineMsg, name));
    }
    io.run();

    return 0;
}
