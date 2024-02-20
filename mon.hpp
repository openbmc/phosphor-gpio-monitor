#pragma once

#include <functional>
#include <string>

namespace phosphor
{
namespace gpio
{

/** @class Monitor
 *  @brief Responsible for catching state changes
 */
class Monitor
{
  public:
    /** @brief Constructs Monitor object.
     *
     *  @param[in] msg     - Message to be used for log
     *  @param[in] continueRun - Whether to continue after event occur
     */
    Monitor(const std::string& msg, const bool& continueRun) :
        msg(msg), continueAfterEvent(continueRun){};

    /** @brief Monitor event message */
    const std::string msg;

    /** @brief Start monitoring for event, callable only once
     *
     *  @return  - 0 on success and -1 otherwise
     */
    virtual int startMonitoring() = 0;

    /** @brief Stop monitoring for event */
    void stopMonitoring()
    {
        continueAfterEvent = false;
    };

    /** @brief Function ptr called when event has occurred to perform action
     *
     *  @param[in] eventType  - The type of event used to determine which action
     *                          to perform. Events are either assert(1) or
     *                          deassert(2).
     */
    std::function<void(const int eventType)> performEventAction =
        []([[maybe_unused]] const int eventType) {};

    /** @brief Function ptr called when event monitoring is initialized based on
     *         event state at that time.
     *
     *  @param[in] eventType  - The type of event used to determine which action
     *                          to perform. Event state are either asserted(1)
     *                          or deasserted(2).
     */
    std::function<void(const int eventType)> performInitEventAction =
        []([[maybe_unused]] const int eventType) {};

  protected:
    /** @brief If the monitor should continue after event */
    bool continueAfterEvent;
};

} // namespace gpio
} // namespace phosphor
