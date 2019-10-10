#pragma once

#include <gpiod.h>

#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

namespace phosphor
{
namespace gpio
{

/** @class GpioMonitor
 *  @brief Responsible for catching GPIO state change
 *  condition and starting systemd targets.
 */
class GpioMonitor
{
  public:
    GpioMonitor() = delete;
    ~GpioMonitor() = default;
    GpioMonitor(const GpioMonitor&) = delete;
    GpioMonitor& operator=(const GpioMonitor&) = delete;
    GpioMonitor(GpioMonitor&&) = delete;
    GpioMonitor& operator=(GpioMonitor&&) = delete;

    /** @brief Constructs GpioMonitor object.
     *
     *  @param[in] path     - Path to gpio input device
     *  @param[in] key      - GPIO key to monitor
     *  @param[in] polarity - GPIO assertion polarity to look for
     *  @param[in] target   - systemd unit to be started on GPIO
     *                        value change
     *  @param[in] event    - sd_event handler
     *  @param[in] continueRun - Whether to continue after key pressed
     *  @param[in] handler  - IO callback handler. Defaults to one in this
     *                        class
     *  @param[in] useEvDev - Whether to use EvDev to retrieve events
     */
    GpioMonitor(gpiod_line* line, gpiod_line_request_config config,
                boost::asio::io_service& io, const std::string target,
                std::string lineMsg, bool continueRun) :
        gpioLine(line),
        gpioConfig(config), gpioEventDescriptor(io), target(target),
        gpioLineMsg(lineMsg), continueAfterKeyPress(continueRun)
    {
        requestGPIOEvents();
    };

  private:
    /** @brief GPIO line */
    gpiod_line* gpioLine;

    /** @brief GPIO line configuration */
    gpiod_line_request_config gpioConfig;

    /** @brief GPIO event descriptor */
    boost::asio::posix::stream_descriptor gpioEventDescriptor;

    /** @brief Systemd unit to be started when the condition is met */
    const std::string target;

    /** @brief GPIO line name message */
    std::string gpioLineMsg;

    /** @brief If the monitor should continue after key press */
    bool continueAfterKeyPress;

    /** @brief register handler for gpio event
     *
     *  @return  - 0 on success and -1 otherwise
     */
    int requestGPIOEvents();

    /** @brief Wait for the GPIO event to trigger */
    void gpioEventWait();

    /** @brief Handle the GPIO event and starts configured target */
    void gpioEventHandler();
};

} // namespace gpio
} // namespace phosphor
