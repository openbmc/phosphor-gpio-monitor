#pragma once

#include <memory>
#include "file.hpp"

namespace phosphor
{
namespace gpio
{


/**
 * Represents a GPIO.
 *
 * Operations are setting low or high.
 *
 * Read support may be added in the future.
 */
class GPIO
{
    public:

        /**
         * If the GPIO is an input or output
         */
        enum class Direction
        {
            input,
            output
        };

        /**
         * The possible values - low or high
         */
        enum class Value
        {
            low,
            high
        };

        GPIO() = delete;
        GPIO(const GPIO&) = delete;
        GPIO(GPIO&&) = default;
        GPIO& operator=(const GPIO&) = delete;
        GPIO& operator=(GPIO&&) = default;
        ~GPIO() = default;

        /**
         * Constructor
         *
         * @param[in] device - the GPIO device file
         * @param[in] gpio - the GPIO number
         * @param[in] direction - the GPIO direction
         */
        GPIO(const std::string& device,
             unsigned int gpio,
             Direction direction) :
            device(device),
            gpio(gpio),
            direction(direction)
        {
        }

        /**
         * Sets the GPIO high
         */
        inline void setHigh()
        {
            setGPIO(Value::high);
        }

        /**
         * Sets the GPIO low
         */
        inline void setLow()
        {
            setGPIO(Value::low);
        }

    private:

        /**
         * Requests a GPIO line from the GPIO device
         *
         * @param[in] defaultValue - The default value, required for
         *                           output GPIOs only.
         */
        void requestLine(Value defaultValue = Value::high);

        /**
         * Sets the GPIO to low or high
         *
         * Requests the GPIO line if it hasn't been done already.
         */
        void setGPIO(Value value);

        /**
         * The GPIO device name, like /dev/gpiochip0
         */
        const std::string device;

        /**
         * The GPIO number
         */
        const unsigned int gpio;

        /**
         * The GPIO direction
         */
        const Direction direction;

        /**
         * File descriptor for the GPIO line
         */
        std::unique_ptr<FileDescriptor> lineFD;
};

}
}
