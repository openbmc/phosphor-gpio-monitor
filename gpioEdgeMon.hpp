#pragma once

#include "mon.hpp"

#include <gpiod.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <map>
#include <vector>

namespace phosphor
{
namespace gpio
{

/** @class GpioEdgeMonitor
 *  @brief Responsible for catching GPIO state change
 *  condition.
 */
class GpioEdgeMonitor : public Monitor
{
  public:
    GpioEdgeMonitor() = delete;
    ~GpioEdgeMonitor() = default;
    GpioEdgeMonitor(const GpioEdgeMonitor&) = delete;
    GpioEdgeMonitor& operator=(const GpioEdgeMonitor&) = delete;
    GpioEdgeMonitor(GpioEdgeMonitor&&) = delete;
    GpioEdgeMonitor& operator=(GpioEdgeMonitor&&) = delete;

    /** @brief Constructs GpioEdgeMonitor object.
     *
     *  @param[in] line        - GPIO line from libgpiod
     *  @param[in] config      - configuration of line with event
     *  @param[in] io          - io service
     *  @param[in] lineMsg     - GPIO line message to be used for log
     *  @param[in] continueRun - Whether to continue after event occur
     *  @param[in] verbose     - Whether to print log on GPIO event
     */
    GpioEdgeMonitor(gpiod_line* line, gpiod_line_request_config& config,
                    boost::asio::io_context& io, const std::string& lineMsg,
                    const bool& continueRun, const bool& verbose = true) :
        Monitor(lineMsg, continueRun),
        gpioLine(line), gpioConfig(config), gpioEventDescriptor(io),
        verbose(verbose){};

    /** @brief Start monitoring for GPIO event
     *
     *  @return  - 0 on success and -1 otherwise
     */
    int startMonitoring() override;

    /** @brief Cancel monitoring for GPIO event */
    void stopMonitoring();

  private:
    /** @brief GPIO line */
    gpiod_line* gpioLine;

    /** @brief GPIO line configuration */
    gpiod_line_request_config gpioConfig;

    /** @brief GPIO event descriptor */
    boost::asio::posix::stream_descriptor gpioEventDescriptor;

    /** @brief If logs should be created on GPIO event */
    const bool verbose;

    /** @brief Schedule an event handler for GPIO event to trigger */
    void scheduleEventHandler();

    /** @brief Handle the GPIO event */
    void handleGpioEvent();
};

} // namespace gpio
} // namespace phosphor
