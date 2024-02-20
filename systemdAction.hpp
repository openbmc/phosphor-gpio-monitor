#pragma once
#include "act.hpp"

#include <map>
#include <string>
#include <vector>

namespace phosphor
{
namespace gpio
{
constexpr auto deassertedKeyword = "DEASSERTED";
constexpr auto assertedKeyword = "ASSERTED";

/** @class SystemdAction
 *  @brief Provides mean for starting systemd target(s)
 */
class SystemdAction : public Action
{
  public:
    SystemdAction() = delete;

    /** @brief Constructs SystemdAction object.
     *
     *  @param[in] target      - systemd unit to be started on GPIO
     *                           value change
     *  @param[in] targets     - systemd units to be started on GPIO
     *                           value change
     */
    SystemdAction(
        const std::string& target,
        const std::map<std::string, std::vector<std::string>>& targets) :
        target(target),
        targets(targets)
    {
        eventAction = std::bind(&SystemdAction::startTargets, this,
                                std::placeholders::_1);
    };

  private:
    /** @brief Systemd unit to be started when the condition is met */
    std::string target;

    /** @brief Multi systemd units to be started when the condition is met */
    std::map<std::string, std::vector<std::string>> targets;

    /** @brief Starts configured targets
     *
     *  @param[in] eventType  - The type of event used to start associated
     *                          targets. Events are either assert(1) or
     *                          deassert(2).
     */
    void startTargets(const int& eventType);
};
} // namespace gpio
} // namespace phosphor
