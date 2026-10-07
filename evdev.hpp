// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once
#include "file.hpp"

#include <libevdev/libevdev.h>
#include <systemd/sd-event.h>

#include <sdbusplus/message.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace phosphor
{
namespace gpio
{

/* Need a custom deleter for freeing up sd_event */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        sd_event_unref(event);
    }
};
using EventPtr = std::unique_ptr<sd_event, EventDeleter>;

/* Need a custom deleter for freeing up sd_event_source */
struct EventSourceDeleter
{
    void operator()(sd_event_source* eventSource) const
    {
        sd_event_source_unref(eventSource);
    }
};
using EventSourcePtr = std::unique_ptr<sd_event_source, EventSourceDeleter>;

/* Need a custom deleter for freeing up evdev struct */
struct FreeEvDev
{
    void operator()(struct libevdev* device) const
    {
        libevdev_free(device);
    }
};
using EvdevPtr = std::unique_ptr<struct libevdev, FreeEvDev>;

/** @brief Reads the next input event, with the same contract as
 *         libevdev_next_event(): takes the read flags, fills in the event
 *         and returns a LIBEVDEV_READ_STATUS_* value or a negative errno.
 *         Passed in rather than calling libevdev directly so the read loop
 *         can be unit tested without an input device.
 */
using NextEventFn = std::function<int(unsigned int flags, input_event& ev)>;

/** @brief Handles one input event. Returns false to stop reading. */
using EventHandler = std::function<bool(const input_event& ev)>;

/** @brief Reads events until none are left or the handler asks to stop.
 *         SYN_REPORT markers are not passed to the handler. If the
 *         kernel reports dropped events (SYN_DROPPED), the state changes
 *         that were missed are replayed to the handler as events.
 *
 *  @param[in] next    - Reads the next event
 *  @param[in] handler - Called for each event read
 *
 *  @return 0 once there is nothing left to read or the handler stopped,
 *          or a negative errno if reading failed (for example -ENODEV
 *          when the device has gone away)
 */
int drainEvents(const NextEventFn& next, const EventHandler& handler);

/** @class Evdev
 *  @brief Responsible for catching GPIO state changes conditions and taking
 *  actions
 */
class Evdev
{
    using Property = std::string;
    using Value = std::variant<bool, std::string>;
    // Association between property and its value
    using PropertyMap = std::map<Property, Value>;
    using Interface = std::string;
    // Association between interface and the D-Bus property
    using InterfaceMap = std::map<Interface, PropertyMap>;
    using Object = sdbusplus::object_path;
    // Association between object and the interface
    using ObjectMap = std::map<Object, InterfaceMap>;

  public:
    Evdev() = delete;
    ~Evdev() = default;
    Evdev(const Evdev&) = delete;
    Evdev& operator=(const Evdev&) = delete;
    Evdev(Evdev&&) = delete;
    Evdev& operator=(Evdev&&) = delete;

    /** @brief Constructs Evdev object.
     *
     *  @param[in] path      - Device path to read for GPIO pin state
     *  @param[in] key       - GPIO key to monitor
     *  @param[in] event     - sd_event handler
     *  @param[in] handler   - IO callback handler.
     *  @param[in] useEvDev  - Whether to use EvDev to retrieve events
     */
    Evdev(const std::string& path, const unsigned int key, EventPtr& event,
          sd_event_io_handler_t handler, bool useEvDev = true) :
        path(path), key(key), event(event), callbackHandler(handler),
        fd(openDevice())

    {
        if (useEvDev)
        {
            // If we are asked to use EvDev, do that initialization.
            initEvDev();
        }

        // Register callback handler when FD has some data
        registerCallback();
    }

  protected:
    /** @brief Device path to read for GPIO pin state */
    const std::string path;

    /** @brief GPIO key to monitor */
    const unsigned int key;

    /** @brief Event structure */
    EvdevPtr devicePtr;

    /** @brief Monitor to sd_event */
    EventPtr& event;

    /** @brief Callback handler when the FD has some data */
    sd_event_io_handler_t callbackHandler;

    /** @brief event source */
    EventSourcePtr eventSource;

    /** @brief Opens the device and populates the descriptor */
    int openDevice();

    /** @brief attaches FD to events and sets up callback handler */
    void registerCallback();

    /** @brief File descriptor manager */
    FileDescriptor fd;

    /** @brief Initializes evdev handle with the fd */
    void initEvDev();

    /** @brief Reads all pending events from the device. If the device
     *         fails, asks the event loop to exit so systemd can restart
     *         the service.
     *
     *  @param[in] handler - Called for each event read
     */
    void readEvents(const EventHandler& handler);
};

} // namespace gpio
} // namespace phosphor
