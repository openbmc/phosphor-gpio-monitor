/**
 * Copyright © 2017 IBM Corporation
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

/**
 * This program is a utility for accessing GPIOs.
 * Actions:
 *   low:  Set a GPIO low
 *   high: Set a GPIO high
 *   low_high: Set a GPIO low, delay if requested, set it high
 *   high_low: Set a GPIO high, delay if requested, set it low
 */

#include "gpio.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <thread>

using namespace phosphor::gpio;

typedef void (*gpioFunction)(GPIO&, unsigned int);
using gpioFunctionMap = std::map<std::string, gpioFunction>;

/**
 * Sets a GPIO low
 *
 * @param[in] gpio - the GPIO object
 * @param[in] delayInMS - Unused in this function
 */
void low(GPIO& gpio, unsigned int)
{
    gpio.set(GPIO::Value::low);
}

/**
 * Sets a GPIO high
 *
 * @param[in] gpio - the GPIO object
 * @param[in] delayInMS - Unused in this function
 */
void high(GPIO& gpio, unsigned int)
{
    gpio.set(GPIO::Value::high);
}

/**
 * Sets a GPIO high, then delays, then sets it low
 *
 * @param[in] gpio - the GPIO object
 * @param[in] delayInMS - The delay in between the sets
 */
void highLow(GPIO& gpio, unsigned int delayInMS)
{
    gpio.set(GPIO::Value::high);

    std::chrono::milliseconds delay{delayInMS};
    std::this_thread::sleep_for(delay);

    gpio.set(GPIO::Value::low);
}

/**
 * Sets a GPIO low, then delays, then sets it high
 *
 * @param[in] gpio - the GPIO to write
 * @param[in] delayInMS - The delay in between the sets
 */
void lowHigh(GPIO& gpio, unsigned int delayInMS)
{
    gpio.set(GPIO::Value::low);

    std::chrono::milliseconds delay{delayInMS};
    std::this_thread::sleep_for(delay);

    gpio.set(GPIO::Value::high);
}

/**
 * The actions supported by this program
 */
static const gpioFunctionMap functions{
    {"low", low}, {"high", high}, {"low_high", lowHigh}, {"high_low", highLow}};

/**
 * Returns the number value of the argument passed in.
 *
 * @param[in] name - the argument name
 * @param[in] parser - the argument parser
 * @param[in] argv - arv from main()
 */
template <typename T>
T getValueFromArg(const char* name, const char* value)
{
    char* p = NULL;
    auto val = strtol(value, &p, 10);

    // strol sets p on error, also we don't allow negative values
    if (*p || (val < 0))
    {
        lg2::error("Invalid {NAME} value passed in", "NAME", name);
    }
    return static_cast<T>(val);
}

int main(int argc, char** argv)
{
    CLI::App app{"Gpio utils tool"};

    // Read arguments.
    std::string path{};
    std::string action{};
    std::string gpio{};
    std::string delay{};

    /* Add an input option */
    app.add_option("-p,--path", path,
                   "The path to the GPIO device. Example: /dev/gpiochip0")
        ->required();
    app.add_option(
           "-a,--action", action,
           "The action to do. Valid actions: low, high, low_high, high_low")
        ->required();
    app.add_option("-g,--gpio", gpio, "he GPIO number.  Example: 1")
        ->required();
    app.add_option("-d,--delay", delay,
                   "The delay in ms in between a toggle. Example: 5");

    /* Parse input parameter */
    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::Error& e)
    {
        return app.exit(e);
    }

    auto gpioNum = getValueFromArg<gpioNum_t>("gpio", gpio.c_str());

    // Not all actions require a delay, so not required
    unsigned int delayVal = 0;
    if (!delay.empty())
    {
        delayVal = getValueFromArg<decltype(delayVal)>("delay", delay.c_str());
    }

    auto function = functions.find(action);
    if (!functions.contains(action))
    {
        lg2::error("Invalid action value passed in: {ACTION}", "ACTION",
                   action);
        return -1;
    }

    GPIO gpioVal{path, gpioNum, GPIO::Direction::output};

    try
    {
        function->second(gpioVal, delayVal);
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what();
        return -1;
    }

    return 0;
}
