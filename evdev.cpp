// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "evdev.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <fcntl.h>
#include <libevdev/libevdev.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cerrno>

namespace phosphor
{
namespace gpio
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

int drainEvents(const NextEventFn& next, const EventHandler& handler)
{
    unsigned int flags = LIBEVDEV_READ_FLAG_NORMAL;

    while (true)
    {
        struct input_event ev{};
        int rc = next(flags, ev);

        if (rc == -EAGAIN && flags == LIBEVDEV_READ_FLAG_SYNC)
        {
            // All missed changes have been replayed, back to live events.
            flags = LIBEVDEV_READ_FLAG_NORMAL;
            continue;
        }

        if (rc == -EAGAIN)
        {
            // No more events to read for now.
            return 0;
        }

        if (rc < 0)
        {
            return rc;
        }

        if (rc == LIBEVDEV_READ_STATUS_SYNC &&
            flags == LIBEVDEV_READ_FLAG_NORMAL)
        {
            // SYN_DROPPED: the kernel buffer overflowed and events were
            // lost. Ask libevdev to replay the state changes we missed,
            // otherwise they are absorbed into its internal state and
            // never reach the handler. See the SYN_DROPPED handling
            // section of the libevdev documentation.
            lg2::warning("Input events were dropped, resynchronizing");
            flags = LIBEVDEV_READ_FLAG_SYNC;
            continue;
        }

        if (ev.type == EV_SYN && ev.code == SYN_REPORT)
        {
            continue;
        }

        if (!handler(ev))
        {
            return 0;
        }
    }
}

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

void Evdev::readEvents(const EventHandler& handler)
{
    auto rc = drainEvents(
        [this](unsigned int flags, input_event& ev) {
            return libevdev_next_event(devicePtr.get(), flags, &ev);
        },
        handler);
    if (rc < 0)
    {
        // The fd stays readable after an error, so returning here would
        // just call us again in a busy loop.
        lg2::error("Failed to read events from {DEVICEPATH}: {RC}",
                   "DEVICEPATH", path, "RC", rc);
        sd_event_exit(event.get(), rc);
    }
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
