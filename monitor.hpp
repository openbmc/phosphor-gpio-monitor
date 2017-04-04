#pragma once

#include <unistd.h>
#include <string>
#include <linux/input.h>
#include "file.hpp"
namespace phosphor
{
namespace gpio
{
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
         *  @param[in] path     - Path to gpio input device
         *  @param[in] code     - GPIO code to monitor
         *  @param[in] value    - GPIO key value to look for
         *  @param[in] target   - systemd unit to be started on GPIO
         *                        value change
         */
        Monitor(const std::string& path,
                decltype(input_event::code) code,
                decltype(input_event::code) value,
                const std::string& target)
            : path(path),
              code(code),
              value(value),
              target(target),
              FD(openDevice())
        {
            // Nothing
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

        /** @brief Manages File descriptor */
        FileDescriptor FD;

        /** @brief Opens the device and populates the descriptor */
        int openDevice();
};

} // namespace gpio
} // namespace phosphor
