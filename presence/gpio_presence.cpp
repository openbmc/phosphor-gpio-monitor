// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "gpio_presence.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <fcntl.h>
#include <libevdev/libevdev.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

#include <fstream>

namespace phosphor
{
namespace gpio
{
namespace presence
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

std::string getService(const std::string& path, const std::string& interface,
                       sdbusplus::bus_t& bus)
{
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                          MAPPER_INTERFACE, "GetObject");

    mapperCall.append(path);
    mapperCall.append(std::vector<std::string>({interface}));

    std::map<std::string, std::vector<std::string>> mapperResponse;
    try
    {
        auto mapperResponseMsg = bus.call(mapperCall);
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Error in mapper call to get service name, path: {PATH}, interface: {INTERFACE}, error: {ERROR}",
            "PATH", path, "INTERFACE", interface, "ERROR", e);
        elog<InternalFailure>();
    }

    return mapperResponse.begin()->first;
}

void Presence::determinePresence()
{
    auto present = false;
    auto value = static_cast<int>(0);
    auto fetch_rc =
        libevdev_fetch_event_value(devicePtr.get(), EV_KEY, key, &value);
    if (0 == fetch_rc)
    {
        lg2::error("Device does not support event type, key: {KEYCODE}",
                   "KEYCODE", key);
        elog<InternalFailure>();
        return;
    }
    if (value > 0)
    {
        present = true;
    }

    updateInventory(present);
}

// Callback handler when there is an activity on the FD
int Presence::processEvents(sd_event_source*, int, uint32_t, void* userData)
{
    auto presence = static_cast<Presence*>(userData);

    presence->analyzeEvent();
    return 0;
}

// Analyzes the GPIO event
void Presence::analyzeEvent()
{
    // Data returned
    struct input_event ev{};
    int rc = 0;

    // While testing, observed that not having a loop here was leading
    // into events being missed.
    while (rc >= 0)
    {
        // Wait until no more events are available on the device.
        rc = libevdev_next_event(devicePtr.get(), LIBEVDEV_READ_FLAG_NORMAL,
                                 &ev);
        if (rc < 0)
        {
            // There was an error waiting for events, mostly that there are no
            // events to be read.. So continue waiting...
            return;
        }

        if (rc == LIBEVDEV_READ_STATUS_SUCCESS)
        {
            if (ev.type == EV_SYN && ev.code == SYN_REPORT)
            {
                continue;
            }
            else if (ev.code == key)
            {
                auto present = false;
                if (ev.value > 0)
                {
                    present = true;
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(delay));
                    bindOrUnbindDrivers(present);
                    updateInventory(present);
                }
                else
                {
                    updateInventory(present);
                    bindOrUnbindDrivers(present);
                }
            }
        }
    }

    return;
}

Presence::ObjectMap Presence::getObjectMap(bool present)
{
    ObjectMap invObj;
    InterfaceMap invIntf;
    PropertyMap invProp;

    invProp.emplace("Present", present);
    invProp.emplace("PrettyName", name);
    invIntf.emplace("xyz.openbmc_project.Inventory.Item", std::move(invProp));
    // Add any extra interfaces we want to associate with the inventory item
    for (auto& iface : ifaces)
    {
        invIntf.emplace(iface, PropertyMap());
    }
    invObj.emplace(std::move(inventory), std::move(invIntf));

    return invObj;
}

void Presence::updateInventory(bool present)
{
    ObjectMap invObj = getObjectMap(present);

    lg2::info(
        "Updating inventory present property value to {PRESENT}, path: {PATH}",
        "PRESENT", present, "PATH", inventory);

    auto invService = getService(INVENTORY_PATH, INVENTORY_INTF, bus);

    // Update inventory
    auto invMsg = bus.new_method_call(invService.c_str(), INVENTORY_PATH,
                                      INVENTORY_INTF, "Notify");
    invMsg.append(std::move(invObj));
    try
    {
        auto invMgrResponseMsg = bus.call(invMsg);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Error in inventory manager call to update inventory: {ERROR}",
            "ERROR", e);
        elog<InternalFailure>();
    }
}

void Presence::bindOrUnbindDrivers(bool present)
{
    auto action = (present) ? "bind" : "unbind";

    for (auto& driver : drivers)
    {
        auto path = std::get<pathField>(driver) / action;
        auto device = std::get<deviceField>(driver);

        if (present)
        {
            lg2::info("Binding a {DEVICE} driver: {PATH}", "DEVICE", device,
                      "PATH", path);
        }
        else
        {
            lg2::info("Unbinding a {DEVICE} driver: {PATH}", "DEVICE", device,
                      "PATH", path);
        }

        std::ofstream file;

        file.exceptions(std::ofstream::failbit | std::ofstream::badbit |
                        std::ofstream::eofbit);

        try
        {
            file.open(path);
            file << device;
            file.close();
        }
        catch (const std::exception& e)
        {
            lg2::error(
                "Failed binding or unbinding a {DEVICE} after a card was removed or added, path: {PATH}, error: {ERROR}",
                "DEVICE", device, "PATH", path, "ERROR", e);
        }
    }
}

} // namespace presence
} // namespace gpio
} // namespace phosphor
