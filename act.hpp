#pragma once

#include "mon.hpp"

#include <functional>

namespace phosphor
{
namespace gpio
{
const int ASSERTED = 1;

/** @class Action
 *  @brief Provides data and methods necessary to perform action on event
 */
class Action
{
  public:
    /** @brief Function ptr for event action
     *
     *  @param[in] eventType  - The type of event used to determine which action
     *                          to perform. Events are either assert(1) or
     *                          deassert(2).
     */
    std::function<void(const int eventType)> eventAction =
        []([[maybe_unused]] const int eventType) {};

    /** @brief Function ptr for initialization action.
     *
     *  @param[in] eventType  - The type of event used to determine which action
     *                          to perform. Event state are either asserted(1)
     *                          or deasserted(2).
     */
    std::function<void(const int eventType)> initAction =
        []([[maybe_unused]] const int eventType) {};

};
} // namespace gpio
} // namespace phosphor
