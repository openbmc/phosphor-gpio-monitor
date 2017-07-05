#include <iostream>
#include <libevdev/libevdev.h>
#include <fcntl.h>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include "xyz/openbmc_project/Common/error.hpp"
#include "gpio_presence.hpp"

namespace phosphor
{
namespace gpio
{
namespace presence
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

constexpr auto  INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

std::string getInvService(sdbusplus::bus::bus& bus)
{
    return getService(INVENTORY_PATH, INVENTORY_INTF, bus);
}

std::string getService(const std::string& path,
                       const std::string& interface,
                       sdbusplus::bus::bus& bus)
{
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME,
                                          MAPPER_PATH,
                                          MAPPER_INTERFACE,
                                          "GetObject");

    mapperCall.append(path);
    mapperCall.append(std::vector<std::string>({interface}));

    auto mapperResponseMsg = bus.call(mapperCall);
    if (mapperResponseMsg.is_method_error())
    {
        log<level::ERR>("Error in mapper call to get service name",
                        entry("PATH=%s", path.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
        elog<InternalFailure>();
    }


    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);

    if (mapperResponse.empty())
    {
        log<level::ERR>(
            "Error in mapper response for getting service name",
            entry("PATH=%s", path.c_str()),
            entry("INTERFACE=%s", interface.c_str()));
        elog<InternalFailure>();
    }

    return mapperResponse.begin()->first;
}

std::unique_ptr<libevdev, FreeEvDev>  evdevOpen(int fd)
{
    libevdev* gpioDev = nullptr;

    auto rc = libevdev_new_from_fd(fd, &gpioDev);
    if (!rc)
    {
        return decltype(evdevOpen(0))(gpioDev);
    }

    log<level::ERR>("Failed to get libevdev from file descriptor",
                    entry("RC=%d", rc));
    elog<InternalFailure>();

    return decltype(evdevOpen(0))(nullptr);
}


void Presence::determinePresence()
{
    FileDescriptor gpioFd{open(device.c_str(), 0)};

    auto gpioDev = evdevOpen(gpioFd());
    auto present = false;
    auto value = static_cast<int>(0);
    auto fetch_rc = libevdev_fetch_event_value(gpioDev.get(), EV_KEY,
                    key, &value);
    if (0 == fetch_rc)
    {
        log<level::ERR>("Device does not support event type",
                        entry("KEYCODE=%d", key));
        elog<InternalFailure>();
        return;
    }
    if (value > 0)
    {
        present = true;

    }

    updateInventory(present);
    return;
}


Presence::ObjectMap Presence::getObjectMap(bool present)
{
    ObjectMap invObj;
    InterfaceMap invIntf;
    PropertyMap invProp;

    invProp.emplace("Present", present);
    invProp.emplace("PrettyName", name);
    invIntf.emplace("xyz.openbmc_project.Inventory.Item",
                    std::move(invProp));
    invObj.emplace(std::move(path), std::move(invIntf));

    return invObj;
}

void Presence::updateInventory(bool present)
{
    using namespace phosphor::logging;

    ObjectMap invObj = getObjectMap(present);

    std::string invService;

    invService = getInvService(bus);

    // Update inventory
    auto invMsg = bus.new_method_call(invService.c_str(),
                                      INVENTORY_PATH,
                                      INVENTORY_INTF,
                                      "Notify");
    invMsg.append(std::move(invObj));
    auto invMgrResponseMsg = bus.call(invMsg);
    if (invMgrResponseMsg.is_method_error())
    {
        log<level::ERR>(
            "Error in inventory manager call to update inventory");
        elog<InternalFailure>();
    }
}


} // namespace presence
} // namespace gpio
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
