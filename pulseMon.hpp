#pragma once

#include "gpioEdgeMon.hpp"

#include <boost/asio/placeholders.hpp>
#include <boost/asio/steady_timer.hpp>
#include <memory>

namespace phosphor
{
namespace gpio
{

enum pulseEventMonType
{
    PULSE_START = 1,
    PULSE_STOP,
    PULSE_BOTH
};

/** @class PulseMonitor
 *  @brief Responsible for catching pulsing signal state change
 *  condition.
 */
class PulseMonitor : public Monitor
{
  public:
    PulseMonitor() = delete;
    ~PulseMonitor() = default;
    PulseMonitor(const PulseMonitor&) = delete;
    PulseMonitor& operator=(const PulseMonitor&) = delete;
    PulseMonitor(PulseMonitor&&) = delete;
    PulseMonitor& operator=(PulseMonitor&&) = delete;

    /** @brief Constructs PulseMonitor object.
     *
     *  @param[in] line        - GPIO line from libgpiod
     *  @param[in] config      - configuration of line with event
     *  @param[in] io          - io service
     *  @param[in] lineMsg     - GPIO line message to be used for log
     *  @param[in] continueRun - Whether to continue after event occur
     *  @param[in] timeoutInterval  - Timeout in milliseconds to determine
     *                                pulsing signal is inactive
     *  @param[in] pulseRequestType  - Pulse event type to monitor
     */
    PulseMonitor(gpiod_line* line, gpiod_line_request_config& config,
                 boost::asio::io_context& io, const std::string& lineMsg,
                 bool continueRun, const unsigned int& timeoutInterval,
                 const pulseEventMonType pulseRequestType) :
        Monitor(lineMsg, continueRun),
        gpioMon(std::make_shared<GpioEdgeMonitor>(
            line, config, io, lineMsg,
            /*continue gpio mon*/ true,
            /*don't create log each GPIO state change*/ false)),
        timeout(timeoutInterval), pulseMonType(pulseRequestType),
        timer(std::make_shared<boost::asio::steady_timer>(
            io, std::chrono::milliseconds(timeoutInterval)))
    {
        gpioMon->performEventAction = std::bind(&PulseMonitor::handleGpioEvent,
                                                this, std::placeholders::_1);
    };

    /** @brief Start monitoring for Pulse event
     *
     *  @return  - 0 on success and -1 otherwise
     */
    int startMonitoring() override;

    /** @brief Cancel monitoring for Pulse event */
    void stopMonitoring();

  private:
    /** @brief GPIO monitoring object */
    std::shared_ptr<GpioEdgeMonitor> gpioMon;

    /** @brief Pulse timeout */
    const unsigned int timeout;

    /** @brief Pulse monitor type */
    const pulseEventMonType pulseMonType;

    /** @brief Timeout interval timer for gpio signal */
    std::shared_ptr<boost::asio::steady_timer> timer;

    /** @brief Counter of GPIO state changes before timer expires.
     *  Used to track pulse state:
     *      0/1 is off
     *      2 is pulsing
     */
    std::shared_ptr<int> cntGpioEdgeEvents = std::make_shared<int>(0);

    /** @brief Resets the time remaining to timeout */
    void resetTimeRemaining();

    /** @brief Handler called on timer expiration */
    void handleTimeout(const boost::system::error_code& e);

    /** @brief Handle the GPIO event by starting target or resetting timer
     *
     *  @param[in] eventType  - The libgpiod GPIO event_type to be handled.
     */
    void handleGpioEvent(const int eventType);
};
} // namespace gpio
} // namespace phosphor
