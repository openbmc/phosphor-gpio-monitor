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

constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

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

// Populate the file descriptor for passed in device
int Presence::openDevice()
{
    using namespace phosphor::logging;

    auto fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        log<level::ERR>("Failed to open device path",
                        entry("DEVICEPATH=%s", path.c_str()),
                        entry("ERRNO=%d", errno));
        elog<InternalFailure>();
    }
    return fd;
}

// Initializes the event device with the fd
void Presence::initEvDev()
{
    if (devicePtr)
    {
        // Init can be done only once per device
        return;
    }

    struct libevdev* evdev = nullptr;
    auto rc = libevdev_new_from_fd((fd)(), &evdev);
    if (rc < 0)
    {
        log<level::ERR>("Failed to initialize evdev");
        elog<InternalFailure>();
        return;
    }

    // Packing in the unique_ptr
    devicePtr.reset(evdev);
}

void Presence::determinePresence()
{
    auto present = false;
    auto value = static_cast<int>(0);
    auto fetch_rc = libevdev_fetch_event_value(devicePtr.get(), EV_KEY,
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
}

// Callback handler when there is an activity on the FD
int Presence::processEvents(sd_event_source* es, int fd,
                            uint32_t revents, void* userData)
{
    auto presence = static_cast<Presence*>(userData);

    presence->analyzeEvent();
    return 0;
}


// Analyzes the GPIO event
void Presence::analyzeEvent()
{

    // Data returned
    struct input_event ev {};
    int rc = 0;

    // While testing, observed that not having a loop here was leading
    // into events being missed.
    while (rc >= 0)
    {
        // Wait until no more events are available on the device.
        rc = libevdev_next_event(devicePtr.get(),
                                 LIBEVDEV_READ_FLAG_NORMAL, &ev);
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
                }
                updateInventory(present);
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
    invIntf.emplace("xyz.openbmc_project.Inventory.Item",
                    std::move(invProp));
    invObj.emplace(std::move(inventory), std::move(invIntf));

    return invObj;
}

void Presence::updateInventory(bool present)
{
    ObjectMap invObj = getObjectMap(present);

    log<level::INFO>("Updating inventory present property",
                     entry("PRESENT=%d", present),
                     entry("PATH=%s", inventory));

    auto invService = getService(INVENTORY_PATH, INVENTORY_INTF, bus);

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

// Attaches the FD to event loop and registers the callback handler
void Presence::registerCallback()
{
    decltype(eventSource.get()) sourcePtr = nullptr;
    auto rc = sd_event_add_io(event.get(), &sourcePtr, (fd)(),
                              EPOLLIN, callbackHandler, this);
    eventSource.reset(sourcePtr);

    if (rc < 0)
    {
        log<level::ERR>("Failed to register callback handler",
                        entry("ERROR=%s", strerror(-rc)));
        elog<InternalFailure>();
    }
}

} // namespace presence
} // namespace gpio
} // namespace phosphor

