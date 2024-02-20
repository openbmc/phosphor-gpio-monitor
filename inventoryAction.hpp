#pragma once

#include "act.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <map>
#include <vector>

namespace phosphor
{
namespace gpio
{

/** @class InventoryAction
 *  @brief Provides mean for controlling dbus inventory presence property.
 */
class InventoryAction : public Action
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
    InventoryAction() = delete;

    /** @brief Constructs InventoryAction object.
     *
     *  @param[in] inventory        - Object path under inventory that
     *                                will be created
     *  @param[in] extraInterfaces  - List of interfaces to associate to
     *                                inventory item
     *  @param[in] name             - PrettyName of inventory object
     */
    InventoryAction(const std::string& inventory,
                    const std::vector<std::string>& interfaces,
                    const std::string& name) :
        inventory(inventory),
        interfaces(interfaces), name(name)
    {
        initAction = std::bind(&InventoryAction::updateInventory, this,
                               std::placeholders::_1);
        eventAction = initAction;
    };

  private:
    /** @brief Object path under inventory that will be created */
    const std::string inventory;

    /** @brief List of interfaces to associate to inventory item */
    const std::vector<std::string> interfaces;

    /** @brief PrettyName of inventory object */
    const std::string name;

    /** @brief Returns the object map for the inventory object */
    ObjectMap getObjectMap(bool present);

    /** @brief Updates the inventory state */
    void updateInventory(const int eventType);
};
} // namespace gpio
} // namespace phosphor
