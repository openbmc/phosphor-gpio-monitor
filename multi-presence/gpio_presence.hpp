// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include <gpiod.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <cstdlib>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

static constexpr auto deviceField = 0;
static constexpr auto pathField = 1;
using Device = std::string;
using Path = std::filesystem::path;
using Driver = std::tuple<Device, Path>;
using Interface = std::string;

namespace phosphor
{
namespace gpio
{

/** @class GpioPresence
 *  @brief Responsible for catching GPIO state change
 *  condition and updating the inventory presence.
 */
class GpioPresence
{
    using Property = std::string;
    using Value = std::variant<bool, std::string>;
    // Association between property and its value
    using PropertyMap = std::map<Property, Value>;
    using Interface = std::string;
    // Association between interface and the D-Bus property
    using InterfaceMap = std::map<Interface, PropertyMap>;
    using Object = sdbusplus::message::object_path;
    // Association between object and the interface
    using ObjectMap = std::map<Object, InterfaceMap>;

  public:
    GpioPresence() = delete;
    ~GpioPresence() = default;
    GpioPresence(const GpioPresence&) = delete;
    GpioPresence& operator=(const GpioPresence&) = delete;

    /** @brief Constructs GpioPresence object.
     *
     *  @param[in] line             - GPIO line from libgpiod
     *  @param[in] config           - configuration of line with event
     *  @param[in] io               - io service
     *  @param[in] inventory        - Object path under inventory that
                                      will be created
     *  @param[in] extraInterfaces  - List of interfaces to associate to
                                      inventory item
     *  @param[in] name             - PrettyName of inventory object
     *  @param[in] lineMsg          - GPIO line message to be used for log
     */
    GpioPresence(gpiod_line* line, gpiod_line_request_config& config,
                 boost::asio::io_context& io, const std::string& inventory,
                 const std::vector<std::string>& extraInterfaces,
                 const std::string& name, const std::string& lineMsg) :
        gpioLine(line), gpioConfig(config), gpioEventDescriptor(io),
        inventory(inventory), interfaces(extraInterfaces), name(name),
        gpioLineMsg(lineMsg)
    {
        requestGPIOEvents();
    };

    GpioPresence(GpioPresence&& old) noexcept :
        gpioLine(old.gpioLine), gpioConfig(old.gpioConfig),
        gpioEventDescriptor(old.gpioEventDescriptor.get_executor()),
        inventory(std::move(old.inventory)),
        interfaces(std::move(old.interfaces)), name(std::move(old.name)),
        gpioLineMsg(std::move(old.gpioLineMsg))
    {
        old.cancelEventHandler();

        gpioEventDescriptor = std::move(old.gpioEventDescriptor);

        scheduleEventHandler();
    };

  private:
    /** @brief GPIO line */
    gpiod_line* gpioLine;

    /** @brief GPIO line configuration */
    gpiod_line_request_config gpioConfig;

    /** @brief GPIO event descriptor */
    boost::asio::posix::stream_descriptor gpioEventDescriptor;

    /** @brief Object path under inventory that will be created */
    const std::string inventory;

    /** @brief List of interfaces to associate to inventory item */
    const std::vector<std::string> interfaces;

    /** @brief PrettyName of inventory object */
    const std::string name;

    /** @brief GPIO line name message */
    const std::string gpioLineMsg;

    /** @brief register handler for gpio event
     *
     *  @return  - 0 on success and -1 otherwise
     */
    int requestGPIOEvents();

    /** @brief Schedule an event handler for GPIO event to trigger */
    void scheduleEventHandler();

    /** @brief Stop the event handler for GPIO events */
    void cancelEventHandler();

    /** @brief Handle the GPIO event and starts configured target */
    void gpioEventHandler();

    /** @brief Returns the object map for the inventory object */
    ObjectMap getObjectMap(bool present);

    /** @brief Updates the inventory */
    void updateInventory(bool present);
};

} // namespace gpio
} // namespace phosphor
