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

    auto value = static_cast<int>(0);
    auto fetch_rc = libevdev_fetch_event_value(gpioDev.get(), EV_KEY,
                    key, &value);
    if (0 == fetch_rc)
    {
        log<level::ERR>("Device does not support event type",
                        entry("KEYCODE=%d", key));
        elog<InternalFailure>();
    }
    return;
}

} // namespace presence
} // namespace gpio
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
