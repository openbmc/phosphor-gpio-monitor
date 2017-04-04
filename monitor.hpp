#pragma once

#include <unistd.h>
#include <string>
#include <linux/input.h>
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
         *  @param[in] code     - GPIO code to monitor
         *  @param[in] value    - GPIO key value to look for
         *  @param[in] target   - systemd unit to be started on GPIO
         *                        value change
         *  @param[in] event    - sd_event handler
         *  @param[in] handler  - IO callback handler. Defaults to one in this
         *                        class
         */
        Monitor(const std::string& path,
                decltype(input_event::code) code,
                decltype(input_event::code) value,
                const std::string& target,
                EventPtr& event,
                sd_event_io_handler_t handler = Monitor::processEvents)
            : path(path),
              code(code),
              value(value),
              target(target),
              event(event),
              callbackHandler(handler),
              FD(openDevice()),
              bus(sdbusplus::bus::new_default())
        {
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
        decltype(input_event::code) code;

        /** @brief GPIO key value that is of interest */
        decltype(input_event::value) value;

        /** @brief Systemd unit to be started when the condition is met */
        const std::string& target;

        /** @brief Monitor to sd_event */
        EventPtr& event;

        /** @brief Callback handler when the FD has some data */
        sd_event_io_handler_t callbackHandler;

        /** @brief File descriptor manager */
        FileDescriptor FD;

        /** @brief sdbusplus handler */
        sdbusplus::bus::bus bus;

        /** @brief Completion indicator */
        bool complete = false;

        /** @brief Opens the device and populates the descriptor */
        int openDevice();

        /** @brief attaches FD to events and sets up callback handler */
        void registerCallback();

        /** @brief Analyzes the GPIO event and starts configured target
         *
         *  @return - For now, returns zero
         */
        int analyzeEvent();
};

} // namespace gpio
} // namespace phosphor
