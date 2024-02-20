#pragma once

#include "gpioEdgeMon.hpp"
#include "inventoryAction.hpp"
#include "pulseMon.hpp"
#include "systemdAction.hpp"

namespace phosphor
{
namespace gpio
{

/** @class GpioHandler
 *  @brief Responsible for catching state change
 *  condition and taking action if defined.
 */
class GpioHandler
{
  public:
    GpioHandler() = delete;
    ~GpioHandler() = default;
    GpioHandler(const GpioHandler&) = delete;
    GpioHandler& operator=(const GpioHandler&) = delete;
    GpioHandler(GpioHandler&&) = delete;
    GpioHandler& operator=(GpioHandler&&) = delete;

    /** @brief Constructs GpioHandler object.
     *
     *  @param[in] monitorType - Type of signal to monitor
     *  @param[in] actionType  - Type of action to perform
     */
    GpioHandler(std::unique_ptr<Monitor>& monitorType,
                std::unique_ptr<Action>& actionType) :
        monObj(std::move(monitorType)),
        actObj(std::move(actionType))
    {
        monObj->performInitEventAction = actObj->initAction;
        monObj->performEventAction = actObj->eventAction;
        monObj->startMonitoring();
    };

  private:
    /** @brief Monitoring object */
    std::unique_ptr<Monitor> monObj;

    /** @brief Action object */
    std::unique_ptr<Action> actObj;
};
} // namespace gpio
} // namespace phosphor
