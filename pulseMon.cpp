#include "pulseMon.hpp"

#include <boost/bind/bind.hpp>
#include <phosphor-logging/lg2.hpp>

namespace phosphor
{
namespace gpio
{
/* Minimium amount of state changes required without timing out to be considered
 * pulsing.
 */
const int MIN_CHANGE_TO_PULSE = 2;

int PulseMonitor::startMonitoring()
{
    /* By default, initial state isn't pulsing */
    performInitEventAction(PULSE_STOP);
    return gpioMon->startMonitoring();
}

void PulseMonitor::stopMonitoring()
{
    timer->cancel();
    gpioMon->stopMonitoring();
}

void PulseMonitor::handleGpioEvent([[maybe_unused]] const int eventType)
{
    resetTimeRemaining();

    /* Requires MIN_CHANGE_TO_PULSE GPIO edge events to occur without the timer
     * expiring before a pulse is considered started. Prevent a false pulse
     * start due to a single state change.
     */
    if (*cntGpioEdgeEvents == MIN_CHANGE_TO_PULSE)
    {
        /* Already pulsing, do nothing */
    }
    else if (++(*cntGpioEdgeEvents) == MIN_CHANGE_TO_PULSE)
    {
        /* Pulse detected */
        if (pulseMonType == PULSE_START || pulseMonType == PULSE_BOTH)
        {
            lg2::info("{GPIO} pulse detected", "GPIO", gpioMon->msg);
            performEventAction(PULSE_START);
            /* if not required to continue monitoring then return */
            if (!continueAfterEvent)
            {
                stopMonitoring();
            }
        }
    }
}

void PulseMonitor::resetTimeRemaining()
{
    lg2::debug("Reset Timer for {GPIO} to {TIME} milliseconds", "GPIO",
               gpioMon->msg, "TIME", timeout);
    timer->expires_after(std::chrono::milliseconds(timeout));
    timer->async_wait(boost::bind(&PulseMonitor::handleTimeout, this,
                                  boost::asio::placeholders::error));
}

// Optional callback function on timer expiration
void PulseMonitor::handleTimeout(const boost::system::error_code& e)
{
    /* Timeout was not cancelled */
    if (e != boost::asio::error::operation_aborted)
    {
        /* Take action only if GPIO state changed twice before timing out */
        if (*cntGpioEdgeEvents == MIN_CHANGE_TO_PULSE)
        {
            if (pulseMonType == PULSE_STOP || pulseMonType == PULSE_BOTH)
            {
                lg2::info("{GPIO} pulse ceased", "GPIO", gpioMon->msg);
                performEventAction(PULSE_STOP);
                /* if not required to continue monitoring then return */
                if (!continueAfterEvent)
                {
                    stopMonitoring();
                }
            }
        }
        *cntGpioEdgeEvents = 0;
    }
}
} // namespace gpio
} // namespace phosphor
