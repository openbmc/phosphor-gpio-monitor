#pragma once
#include <string>
#include <libevdev/libevdev.h>
#include "file.hpp"

namespace phosphor
{
namespace gpio
{
namespace presence
{

/* Need a custom deleter for freeing up evdev struct */
struct FreeEvDev
{
    void operator()(struct libevdev* device) const
    {
        libevdev_free(device);
    }
};
using EvdevPtr = std::unique_ptr<struct libevdev, FreeEvDev>;

/** @class Presence
 *  @brief Responsible for determining and monitoring presence of
 *  inventory items and updating D-Bus accordingly.
 */
class Presence
{

        using Property = std::string;
        using Value = sdbusplus::message::variant<bool, std::string>;
        // Association between property and its value
        using PropertyMap = std::map<Property, Value>;
        using Interface = std::string;
        // Association between interface and the D-Bus property
        using InterfaceMap = std::map<Interface, PropertyMap>;
        using Object = sdbusplus::message::object_path;
        // Association between object and the interface
        using ObjectMap = std::map<Object, InterfaceMap>;

    public:
        Presence() = delete;
        ~Presence() = default;
        Presence(const Presence&) = delete;
        Presence& operator=(const Presence&) = delete;
        Presence(Presence&&) = delete;
        Presence& operator=(Presence&&) = delete;

        /** @brief Constructs Presence object.
         *
         *  @param[in] bus      - D-Bus bus Object
         *  @param[in] path     - Object path under inventory
                                  to display this inventory item
         *  @param[in] device   - Device to read for GPIO pin state
                                  to determine presence of inventory item
         *  @param[in] key      - GPIO key to monitor
         *  @param[in] name     - Pretty name of the inventory item
         */
        Presence(sdbusplus::bus::bus& bus,
                 const std::string& path,
                 const std::string& device,
                 const unsigned int& key,
                 const std::string& name) :
            bus(bus),
            path(path),
            device(device),
            key(key),
            name(name),
            fd(openDevice())
        {
            initEvDev();
            determinePresence();
        }

    private:
        /**
         * @brief Update the present property for the inventory item.
         *
         * @param[in] present - What the present property should be set to.
         */
        void updateInventory(bool present);

        /**
         * @brief Construct the inventory object map for the inventory item.
         *
         * @param[in] present - What the present property should be set to.
         *
         * @return The inventory object map to update inventory
         */
        ObjectMap getObjectMap(bool present);


        /** @brief Connection for sdbusplus bus */
        sdbusplus::bus::bus& bus;

        /**
         * @brief Read the GPIO device to determine initial presence and set
         *        present property at D-Bus path.
         **/
        void determinePresence();

        /** @brief Object path under inventory to display this inventory item */
        const std::string& path;

        /** @brief Device to read for GPIO pin state
                   to determine presence of inventory item */
        const std::string& device;

        /** @brief GPIO key to monitor */
        const unsigned int& key;

        /** @brief Pretty name of the inventory item*/
        const std::string& name;

        /** @brief Event structure */
        EvdevPtr devicePtr;

        /** @brief Opens the device and populates the descriptor */
        int openDevice();

        /** @brief File descriptor manager */
        FileDescriptor fd;

        /** @brief Initializes evdev handle with the fd */
        void initEvDev();
};

/**
 * @brief Get the inventory service name from the mapper object
 *
 * @param[in] bus - The D-Bus bus object
 *
 * @return The inventory manager service name
 */
std::string getInvService(sdbusplus::bus::bus& bus);

/**
 * @brief Get the service name from the mapper for the
 *        interface and path passed in.
 *
 * @param[in] path      - The D-Bus path name
 * @param[in] interface - The D-Bus interface name
 * @param[in] bus       - The D-Bus bus object
 *
 * @return The service name
 */
std::string getService(const std::string& path,
                       const std::string& interface,
                       sdbusplus::bus::bus& bus);

} // namespace presence
} // namespace gpio
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
