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

// Populate the file descriptor for passed in device
int Presence::openDevice()
{
    using namespace phosphor::logging;

    int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        log<level::ERR>("Failed to open device",
                        entry("DEVICE=%s", device.c_str()));
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
    evdev = nullptr;
}

void Presence::determinePresence()
{
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
    return;
}

} // namespace presence
} // namespace gpio
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
