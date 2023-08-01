#include "evdev.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <fcntl.h>
#include <libevdev/libevdev.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace gpio
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

// Populate the file descriptor for passed in device
int Evdev::openDevice()
{
    auto fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        lg2::error("Failed to open {DEVICEPATH}: {ERRNO}", "DEVICEPATH", path,
                   "ERRNO", errno);
        elog<InternalFailure>();
    }
    return fd;
}

// Initializes the event device with the fd
void Evdev::initEvDev()
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
        lg2::error("Failed to initialize evdev");
        elog<InternalFailure>();
        return;
    }

    // Packing in the unique_ptr
    devicePtr.reset(evdev);
}

// Attaches the FD to event loop and registers the callback handler
void Evdev::registerCallback()
{
    decltype(eventSource.get()) sourcePtr = nullptr;
    auto rc = sd_event_add_io(event.get(), &sourcePtr, (fd)(), EPOLLIN,
                              callbackHandler, this);
    eventSource.reset(sourcePtr);

    if (rc < 0)
    {
        lg2::error("Failed to register callback handler: {RC}", "RC", rc);
        elog<InternalFailure>();
    }
}

} // namespace gpio
} // namespace phosphor
