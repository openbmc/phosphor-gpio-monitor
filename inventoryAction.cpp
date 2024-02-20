#include "inventoryAction.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>

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
        lg2::error("Error in mapper call to get service name, path: {PATH}, "
                   "interface: {INTERFACE}, error: {ERROR}",
                   "PATH", path, "INTERFACE", interface, "ERROR", e);
        elog<InternalFailure>();
    }

    return mapperResponse.begin()->first;
}

InventoryAction::ObjectMap InventoryAction::getObjectMap(bool present)
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

void InventoryAction::updateInventory(const int eventType)
{
    bool present = (eventType == ASSERTED) ? true : false;
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
} // namespace gpio
} // namespace phosphor
