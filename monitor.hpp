#pragma once

#include <unistd.h>
#include <string>
#include <systemd/sd-event.h>
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

using eventPtr = std::unique_ptr<sd_event, EventDeleter>;

/** @class Monitor
 *  @brief Responsible for catching GPIO state change
 *  condition and taking actions
 */
class Monitor
{
    public:
        Monitor() = delete;
        Monitor(const Monitor&) = delete;
        Monitor& operator=(const Monitor&) = delete;
        Monitor(Monitor&&) = delete;
        Monitor& operator=(Monitor&&) = delete;

        /** @brief Constructs Monitor object.
         *
         *  @param[in] path    - Path to gpio input device
         *  @param[in] state   - GPIO action that is of interest
         *  @param[in] target  - systemd unit to be started on GPIO
         *                       state change
         *  @param[in] event   - sd_event handler
         *  @param[in] handler - IO callback handler
         */
        Monitor(const std::string& path, uint32_t state,
                const std::string& target,
                const eventPtr& event,
                sd_event_io_handler_t handler)
            : path(path),
              state(state),
              target(target),
              event(event),
              callbackHandler(handler)
        {
            // Populate the file descriptor for passed in device
            openDevice();

            // And register callback handler when FD has some data
            registerCallback();
        }

        ~Monitor()
        {
            if (fd != -1)
            {
                close(fd);
            }

            if (eventSource)
            {
                eventSource = sd_event_source_unref(eventSource);
            }
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

    private:
        /** @brief Absolute path of GPIO input device */
        const std::string& path;

        /** @brief GPIO state change of interest */
        uint32_t state;

        /** @brief Systemd unit to be started when the condition is met */
        const std::string& target;

        /** @brief File descriptor for the gpio input device */
        int fd = -1;

        /** @brief Monitor to sd_event */
        const eventPtr& event;

        /** @brief event source */
        sd_event_source* eventSource = nullptr;

        /** @brief Callback handler when the FD has some data */
        sd_event_io_handler_t callbackHandler;

        /** @brief Populate the file descriptor for passed in device */
        void openDevice();

        /** @brief attaches FD to events and sets up callback handler */
        void registerCallback();
};

} // namespace gpio
} // namespace phosphor
