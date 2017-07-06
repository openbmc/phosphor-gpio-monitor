#pragma once
#include <string>
#include <systemd/sd-event.h>
#include <libevdev/libevdev.h>
#include "file.hpp"

namespace phosphor
{
namespace gpio
{
namespace presence
{

/* Need a custom deleter for freeing up sd_event */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        event = sd_event_unref(event);
    }
};
using EventPtr = std::unique_ptr<sd_event, EventDeleter>;

/* Need a custom deleter for freeing up sd_event_source */
struct EventSourceDeleter
{
    void operator()(sd_event_source* eventSource) const
    {
        eventSource = sd_event_source_unref(eventSource);
    }
};
using EventSourcePtr = std::unique_ptr<sd_event_source, EventSourceDeleter>;

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
         *  @param[in] event    - sd_event handler
         *  @param[in] handler  - IO callback handler. Defaults to one in this
         *                        class
         */
        Presence(sdbusplus::bus::bus& bus,
                 const std::string& path,
                 const std::string& device,
                 const unsigned int& key,
                 const std::string& name,
                 EventPtr& event,
                 sd_event_io_handler_t handler = Presence::processEvents) :
            bus(bus),
            path(path),
            device(device),
            key(key),
            name(name),
            event(event),
            callbackHandler(handler),
            fd(openDevice())

        {
            initEvDev();
            determinePresence();
            // Register callback handler when FD has some data
            registerCallback();
        }

        /** @brief Callback handler when the FD has some activity on it
         *
         *  @param[in] es       - Populated event source
         *  @param[in] fd       - Associated File descriptor
         *  @param[in] revents  - Type of event
         *  @param[in] userData - User data that was passed during registration
         *
         *  @return             - 0 or positive number on success and negative
         *                        errno otherwise
         */
        static int processEvents(sd_event_source* es, int fd,
                                 uint32_t revents, void* userData);

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

        /** @brief Monitor to sd_event */
        EventPtr& event;

        /** @brief Callback handler when the FD has some data */
        sd_event_io_handler_t callbackHandler;

        /** @brief event source */
        EventSourcePtr eventSource;

        /** @brief Opens the device and populates the descriptor */
        int openDevice();

        /** @brief attaches FD to events and sets up callback handler */
        void registerCallback();

        /** @brief File descriptor manager */
        FileDescriptor fd;

        /** @brief Analyzes the GPIO event and update present property*/
        void analyzeEvent();

        /** @brief Initializes evdev handle with the fd */
        void initEvDev();
};

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

