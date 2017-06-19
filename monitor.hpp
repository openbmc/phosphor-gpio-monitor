#pragma once

#include <unistd.h>
#include <string>
#include <linux/input.h>
#include <libevdev/libevdev.h>
#include <systemd/sd-event.h>
#include <sdbusplus/bus.hpp>
#include "file.hpp"
namespace phosphor
{
namespace gpio
{

/* Need a custom deleter for freeing up sd_event */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        event = sd_event_unref(event);
    }
};
using EventPtr = std::unique_ptr<sd_event, EventDeleter>;

/* Need a custom deleter for freeing up sd_event_source */
struct EventSourceDeleter
{
    void operator()(sd_event_source* eventSource) const
    {
        eventSource = sd_event_source_unref(eventSource);
    }
};
using EventSourcePtr = std::unique_ptr<sd_event_source, EventSourceDeleter>;

/* Need a custom deleter for freeing up evdev struct */
struct EvdevDeleter
{
    void operator()(struct libevdev* device) const
    {
        libevdev_free(device);
    }
};
using EvdevPtr = std::unique_ptr<struct libevdev, EvdevDeleter>;

/** @class Monitor
 *  @brief Responsible for catching GPIO state change
 *  condition and taking actions
 */
class Monitor
{
    public:
        Monitor() = delete;
        ~Monitor() = default;
        Monitor(const Monitor&) = delete;
        Monitor& operator=(const Monitor&) = delete;
        Monitor(Monitor&&) = delete;
        Monitor& operator=(Monitor&&) = delete;

        /** @brief Constructs Monitor object.
         *
         *  @param[in] path     - Path to gpio input device
         *  @param[in] key      - GPIO key to monitor
         *  @param[in] polarity - GPIO assertion polarity to look for
         *  @param[in] target   - systemd unit to be started on GPIO
         *                        value change
         *  @param[in] event    - sd_event handler
         *  @param[in] handler  - IO callback handler. Defaults to one in this
         *                        class
         *  @param[in] useEvDev - Whether to use EvDev to retrieve events
         */
        Monitor(const std::string& path,
                decltype(input_event::code) key,
                decltype(input_event::value) polarity,
                const std::string& target,
                EventPtr& event,
                sd_event_io_handler_t handler = Monitor::processEvents,
                bool useEvDev = true)
            : path(path),
              key(key),
              polarity(polarity),
              target(target),
              event(event),
              callbackHandler(handler),
              fd(openDevice())
        {
            if (useEvDev)
            {
                // If we are asked to use EvDev, do that initialization.
                initEvDev();
            }

            // And register callback handler when FD has some data
            registerCallback();
        }

        /** @brief Callback handler when the FD has some activity on it
         *
         *  @param[in] es       - Populated event source
         *  @param[in] fd       - Associated File descriptor
         *  @param[in] revents  - Type of event
         *  @param[in] userData - User data that was passed during registration
         *
         *  @return             - 0 or positive number on success and negative
         *                        errno otherwise
         */
        static int processEvents(sd_event_source* es, int fd,
                                 uint32_t revents, void* userData);

        /** @brief Returns the completion state of this handler */
        inline auto completed() const
        {
            return complete;
        }

    private:
        /** @brief Absolute path of GPIO input device */
        const std::string& path;

        /** @brief GPIO key code that is of interest */
        decltype(input_event::code) key;

        /** @brief GPIO key value that is of interest */
        decltype(input_event::value) polarity;

        /** @brief Systemd unit to be started when the condition is met */
        const std::string& target;

        /** @brief Monitor to sd_event */
        EventPtr& event;

        /** @brief event source */
        EventSourcePtr eventSource;

        /** @brief Callback handler when the FD has some data */
        sd_event_io_handler_t callbackHandler;

        /** @brief File descriptor manager */
        FileDescriptor fd;

        /** event structure */
        EvdevPtr device;

        /** @brief Completion indicator */
        bool complete = false;

        /** @brief Opens the device and populates the descriptor */
        int openDevice();

        /** @brief attaches FD to events and sets up callback handler */
        void registerCallback();

        /** @brief Analyzes the GPIO event and starts configured target */
        void analyzeEvent();

        /** @brief Initializes evdev handle with the fd */
        void initEvDev();
};

} // namespace gpio
} // namespace phosphor
