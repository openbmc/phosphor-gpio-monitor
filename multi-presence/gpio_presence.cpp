/**
 * Copyright © 2019 Facebook
 * Copyright © 2023 9elements GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gpio_presence.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace gpio
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

std::string getService(const std::string& path, const std::string& interface,
                       sdbusplus::bus_t& bus)
{
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                          MAPPER_INTERFACE, "GetObject");

    mapperCall.append(path);
    mapperCall.append(std::vector<std::string>({interface}));

    std::map<std::string, std::vector<std::string>> mapperResponse;
    try
    {
        auto mapperResponseMsg = bus.call(mapperCall);
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Error in mapper call to get service name, path: {PATH}, interface: {INTERFACE}, error: {ERROR}",
            "PATH", path, "INTERFACE", interface, "ERROR", e);
        elog<InternalFailure>();
    }

    return mapperResponse.begin()->first;
}

GpioPresence::ObjectMap GpioPresence::getObjectMap(bool present)
{
    ObjectMap invObj;
    InterfaceMap invIntf;
    PropertyMap invProp;

    invProp.emplace("Present", present);
    invProp.emplace("PrettyName", name);
    invIntf.emplace("xyz.openbmc_project.Inventory.Item", std::move(invProp));
    // Add any extra interfaces we want to associate with the inventory item
    for (auto& iface : interfaces)
    {
        invIntf.emplace(iface, PropertyMap());
    }
    invObj.emplace(std::move(inventory), std::move(invIntf));

    return invObj;
}

void GpioPresence::updateInventory(bool present)
{
    ObjectMap invObj = getObjectMap(present);

    lg2::info(
        "Updating inventory present property value to {PRESENT}, path: {PATH}",
        "PRESENT", present, "PATH", inventory);

    auto bus = sdbusplus::bus::new_default();
    auto invService = getService(INVENTORY_PATH, INVENTORY_INTF, bus);

    // Update inventory
    auto invMsg = bus.new_method_call(invService.c_str(), INVENTORY_PATH,
                                      INVENTORY_INTF, "Notify");
    invMsg.append(std::move(invObj));
    try
    {
        auto invMgrResponseMsg = bus.call(invMsg);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error(
            "Error in inventory manager call to update inventory: {ERROR}",
            "ERROR", e);
        elog<InternalFailure>();
    }
}

void GpioPresence::scheduleEventHandler()
{
    std::string gpio = std::string(gpioLineMsg);

    gpioEventDescriptor.async_wait(
        boost::asio::posix::stream_descriptor::wait_read,
        [this, gpio](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            // we were cancelled
            return;
        }
        if (ec)
        {
            lg2::error("{GPIO} event handler error: {ERROR}", "GPIO", gpio,
                       "ERROR", ec.message());
            return;
        }
        gpioEventHandler();
        });
}

void GpioPresence::cancelEventHandler()
{
    gpioEventDescriptor.cancel();
}

void GpioPresence::gpioEventHandler()
{
    gpiod_line_event gpioLineEvent;

    if (gpiod_line_event_read_fd(gpioEventDescriptor.native_handle(),
                                 &gpioLineEvent) < 0)
    {
        lg2::error("Failed to read {GPIO} from fd", "GPIO", gpioLineMsg);
        return;
    }

    if (gpioLineEvent.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
    {
        lg2::info("{GPIO} Asserted", "GPIO", gpioLineMsg);
    }
    else
    {
        lg2::info("{GPIO} Deasserted", "GPIO", gpioLineMsg);
    }
    updateInventory(gpioLineEvent.event_type == GPIOD_LINE_EVENT_RISING_EDGE);

    /* Schedule a wait event */
    scheduleEventHandler();
}

int GpioPresence::requestGPIOEvents()
{
    std::string flags;

    /* Request an event to monitor for respected gpio line */
    if (gpiod_line_request(gpioLine, &gpioConfig, 0) < 0)
    {
        lg2::error("Failed to request {GPIO}: {ERRNO}", "GPIO", gpioLineMsg,
                   "ERRNO", errno);
        return -1;
    }

    int gpioLineFd = gpiod_line_event_get_fd(gpioLine);
    if (gpioLineFd < 0)
    {
        lg2::error("Failed to get fd for {GPIO}", "GPIO", gpioLineMsg);
        return -1;
    }

    if (gpioConfig.flags & GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE)
    {
        flags += " Bias DISABLE";
    }
    else if (gpioConfig.flags & GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP)
    {
        flags += " Bias PULL_UP";
    }
    else if (gpioConfig.flags & GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN)
    {
        flags += " Bias PULL_DOWN";
    }

    if (gpioConfig.flags & GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW)
    {
        flags += " ActiveLow";
    }

    if (!flags.empty())
    {
        flags = "[" + flags + "]";
    }

    lg2::info("{GPIO} {FLAGS} monitoring started", "GPIO", gpioLineMsg, "FLAGS",
              flags);

    /* Assign line fd to descriptor for monitoring */
    gpioEventDescriptor.assign(gpioLineFd);

    updateInventory(gpiod_line_get_value(gpioLine));

    /* Schedule a wait event */
    scheduleEventHandler();

    return 0;
}
} // namespace gpio
} // namespace phosphor
