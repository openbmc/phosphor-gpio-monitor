#pragma once

#include <gpiod.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <map>
#include <vector>

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
     *  @param[in] line            - GPIO line from libgpiod
     *  @param[in] config          - configuration of line with event
     *  @param[in] io              - io service
     *  @param[in] lineMsg         - GPIO line message to be used for log
     *  @param[in] objPath         - D-Bus service path for this GPIO
     *  @param[in] healthyPolarity - Definition of healthy event
     *  @param[in] currentHealth   - Current GPIO status
     */
    GpioMonitor(gpiod_line* line, gpiod_line_request_config& config,
                boost::asio::io_context& io, const std::string& lineMsg,
                std::string& objPath, bool& healthyPolarity,
                int& currentHealth) :
        gpioLine(line), gpioConfig(config), gpioEventDescriptor(io),
        gpioLineMsg(lineMsg), objectPath(objPath),
        healthyPolarity(healthyPolarity), currentHealth(currentHealth)
    {
        requestGPIOEvents();
    };

    bool getCurrentHealth()
    {
        return currentHealth;
    }

  private:
    /** @brief GPIO line */
    gpiod_line* gpioLine;

    /** @brief GPIO line configuration */
    gpiod_line_request_config gpioConfig;

    /** @brief GPIO event descriptor */
    boost::asio::posix::stream_descriptor gpioEventDescriptor;

    /** @brief GPIO line name message */
    std::string gpioLineMsg;

    /** @brief D-Bus object path */
    std::string objectPath;

    /** @brief healthy polarity */
    bool healthyPolarity;

    /** @brief current status for GPIO */
    bool currentHealth;

    /** @brief register handler for gpio event
     *
     *  @return  - 0 on success and -1 otherwise
     */
    int requestGPIOEvents();

    /** @brief Schedule an event handler for GPIO event to trigger */
    void scheduleEventHandler();

    /** @brief Handle the GPIO event and starts configured target */
    void gpioEventHandler();
};

} // namespace gpio
} // namespace phosphor
