// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "gpio_presence.hpp"

#include <systemd/sd-event.h>

#include <CLI/CLI.hpp>
#include <phosphor-logging/lg2.hpp>

#include <iostream>

using namespace phosphor::gpio;
using namespace phosphor::gpio::presence;

/**
 * Pulls out the path,device pairs from the string
 * passed in
 *
 * @param[in] driverString - space separated path,device pairs
 * @param[out] drivers - vector of device,path tuples filled in
 *                       from driverString
 *
 * @return int - 0 if successful, < 0 else
 */
static int getDrivers(const std::string& driverString,
                      std::vector<Driver>& drivers)
{
    std::istringstream stream{driverString};

    while (true)
    {
        std::string entry;

        // Extract each path,device pair
        stream >> entry;

        if (entry.empty())
        {
            break;
        }

        // Extract the path and device and save them
        auto pos = entry.rfind(',');
        if (pos != std::string::npos)
        {
            auto path = entry.substr(0, pos);
            auto device = entry.substr(pos + 1);

            drivers.emplace_back(std::move(device), std::move(path));
        }
        else
        {
            lg2::error("Invalid path,device combination: {ENTRY}", "ENTRY",
                       entry);
            return -1;
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    CLI::App app{"Monitor gpio presence status"};

    std::string path{};
    std::string key{};
    std::string name{};
    std::string inventory{};
    std::string drivers{};
    std::string ifaces{};

    /* Add an input option */
    app.add_option(
           "-p,--path", path,
           " Path of device to read for GPIO pin state to determine presence of inventory item")
        ->required();
    app.add_option("-k,--key", key, "Input GPIO key number")->required();
    app.add_option("-n,--name", name, "Pretty name of the inventory item")
        ->required();
    app.add_option("-i,--inventory", inventory,
                   "Object path under inventory that will be created")
        ->required();
    app.add_option(
           "-d,--drivers", drivers,
           "List of drivers to bind when card is added and unbind when card is removed\n"
           "Format is a space separated list of path,device pairs.\n"
           "For example: /sys/bus/i2c/drivers/some-driver,3-0068")
        ->expected(0, 1);
    app.add_option(
           "-e,--extra-ifaces", ifaces,
           "List of interfaces to associate to inventory item\n"
           "Format is a comma separated list of interfaces.\n"
           "For example: /xyz/openbmc_project/.../1,/xyz/openbmc_project/.../2")
        ->expected(0, 1);

    /* Parse input parameter */
    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::Error& e)
    {
        return app.exit(e);
    }

    std::vector<Driver> driverList;

    // Driver list is optional
    if (!drivers.empty())
    {
        if (getDrivers(drivers, driverList) < 0)
        {
            lg2::error("Failed to parser drivers: {DRIVERS}", "DRIVERS",
                       drivers);
            return -1;
        }
    }

    std::vector<Interface> ifaceList;

    // Extra interfaces list is optional
    if (!ifaces.empty())
    {
        std::stringstream ss(ifaces);
        Interface iface;
        while (std::getline(ss, iface, ','))
        {
            ifaceList.push_back(iface);
        }
    }

    auto bus = sdbusplus::bus::new_default();
    auto rc = 0;
    sd_event* event = nullptr;
    rc = sd_event_default(&event);
    if (rc < 0)
    {
        lg2::error("Error creating a default sd_event handler");
        return rc;
    }
    EventPtr eventP{event};
    event = nullptr;

    Presence presence(bus, inventory, path, std::stoul(key), name, eventP,
                      driverList, ifaceList);

    while (true)
    {
        // -1 denotes wait forever
        rc = sd_event_run(eventP.get(), (uint64_t)-1);
        if (rc < 0)
        {
            lg2::error("Failure in processing request: {RC}", "RC", rc);
            break;
        }
    }
    return rc;
}
