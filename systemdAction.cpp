#include "systemdAction.hpp"

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace gpio
{

/* systemd service to kick start a target. */
constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_ROOT = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

void SystemdAction::startTargets(const int& eventType)
{
    /* Execute the target if it is defined. */
    if (!target.empty())
    {
        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_ROOT,
                                          SYSTEMD_INTERFACE, "StartUnit");
        method.append(target);
        method.append("replace");

        bus.call_noreply(method);
    }

    std::vector<std::string> targetsToStart;
    if (eventType == ASSERTED)
    {
        auto assertedFind = targets.find(assertedKeyword);
        if (assertedFind != targets.end())
        {
            targetsToStart = assertedFind->second;
        }
    }
    else
    {
        auto deassertedFind = targets.find(deassertedKeyword);
        if (deassertedFind != targets.end())
        {
            targetsToStart = deassertedFind->second;
        }
    }

    /* Execute the multi targets if it is defined. */
    if (!targetsToStart.empty())
    {
        auto bus = sdbusplus::bus::new_default();
        for (auto& tar : targetsToStart)
        {
            auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_ROOT,
                                              SYSTEMD_INTERFACE, "StartUnit");
            method.append(tar, "replace");
            bus.call_noreply(method);
        }
    }
}
} // namespace gpio
} // namespace phosphor
