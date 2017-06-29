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

struct FreeEvDev
{
    void operator()(struct libevdev* device) const
    {
        libevdev_free(device);
    }
};

/** @class Presence
 *  @brief Responsible for determining and monitoring presence of
 *  inventory items and updating Dbus accordingly.
 */
class Presence
{

    public:
        Presence() = delete;
        ~Presence() = default;
        Presence(const Presence&) = delete;
        Presence& operator=(const Presence&) = delete;
        Presence(Presence&&) = delete;
        Presence& operator=(Presence&&) = delete;

        /** @brief Constructs Presence object.
         *
         *  @param[in] path     - Object path under inventory
                                  to display this inventory item
         *  @param[in] device   - Device to read for GPIO pin state
                                  to determine presence of inventory item
         *  @param[in] key      - GPIO key to monitor
         *  @param[in] name     - Pretty name of the inventory item
         */
        Presence(const std::string& path,
                 const std::string& device,
                 const unsigned int& key,
                 const std::string& name) :
            path(path),
            device(device),
            key(key),
            name(name)
        {
            determinePresence();
        }

    private:
        /**
         * @brief Read the GPIO device to determine presence and set 
         *        presence at Dbus path.
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
};

} // namespace presence
} // namespace gpio
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
