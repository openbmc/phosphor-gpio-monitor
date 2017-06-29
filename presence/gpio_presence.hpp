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
            name(name),
            fd(openDevice())
        {
            initEvDev();
            determinePresence();
        }

    private:
        /**
         * @brief Read the GPIO device to determine initial presence and set
         *        present property at D-Bus path.
         **/
        void determinePresence();

        /** @brief Object path under inventory to display this inventory item */
        const std::string path;

        /** @brief Device to read for GPIO pin state
                   to determine presence of inventory item */
        const std::string device;

        /** @brief GPIO key to monitor */
        const unsigned int key;

        /** @brief Pretty name of the inventory item*/
        const std::string name;

        /** @brief Event structure */
        EvdevPtr devicePtr;

        /** @brief Opens the device and populates the descriptor */
        int openDevice();

        /** @brief File descriptor manager */
        FileDescriptor fd;

        /** @brief Initializes evdev handle with the fd */
        void initEvDev();
};

} // namespace presence
} // namespace gpio
} // namespace phosphor

