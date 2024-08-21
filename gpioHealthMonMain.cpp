/**
 * Copyright © 2024 Google
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

#include "gpioHealthMon.hpp"

#include <CLI/CLI.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/async.hpp>
#include <xyz/openbmc_project/HealthMonitor/GetStatus/server.hpp>

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>

namespace phosphor
{
namespace gpio
{

std::map<std::string, int> polarityMap = {
    /**< Only watch falling edge events. */
    {"FALLING", GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE},
    /**< Only watch rising edge events. */
    {"RISING", GPIOD_LINE_REQUEST_EVENT_RISING_EDGE},
    /**< Monitor both types of events. */
    {"BOTH", GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES}};

}
} // namespace phosphor

/* Object mapper service name */
const std::string objMapper = "xyz.openbmc_project.ObjectMapper";
const std::string objMapperPath = "/xyz/openbmc_project/object_mapper";
const std::string objMapperInterface = "xyz.openbmc_project.ObjectMapper";
const std::string getSubTreeMethod = "GetSubTreePaths";
const std::string entityManager = "xyz.openbmc_project.EntityManager";
const std::string getAllInterface = "org.freedesktop.DBus.Properties";
const std::string targetInterface = "xyz.openbmc_project.Configuration.HealthMonitor";
std::set<std::unique_ptr<phosphor::gpio::GpioMonitor>> gpios;

using PropertyType = std::variant<std::string, std::vector<std::string>, uint64_t>;
std::map<std::string, PropertyType> properties;

using GetStatus_inherit = sdbusplus::server::object_t<
    sdbusplus::server::xyz::openbmc_project::health_monitor::GetStatus>;

class GetStatus : GetStatus_inherit
{
  public:
    explicit GetStatus(sdbusplus::bus_t& bus, auto path, bool healthy,
                       std::string entityObjPath, uint64_t limit) :
        GetStatus_inherit(bus, path), entityObjPath(entityObjPath), limit(limit)
    {
        GetStatus::healthy(healthy);
        GetStatus::toggleCount(0);
    }

    void setHealthy() override
    {
        healthy(!healthy());
        toggleCount(toggleCount() + 1);
        healthyPropertyChanged(entityObjPath);
        if(toggleCount() == limit)
        {
            for (auto it = gpios.begin(); it != gpios.end();)
            {
                // Check if the entity path matches
                if ((*it)->getEntityPath() == entityObjPath)
                {
                    // Reset the unique_ptr to release the resource and remove it from the set
                    it = gpios.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            delete this;
        }
        return;
    }
    std::string entityObjPath;
    uint64_t limit;
};

std::vector<std::unique_ptr<GetStatus>> dbusObjects;
std::set<std::string> exist;
std::mutex handlerMutex;

bool createHealthObject(sdbusplus::asio::connection& conn,
                        const std::string& objPath)
{
    /* Call GetAll method call */
    std::vector<std::string> targetInterfaces = {
        "xyz.openbmc_project.Configuration.HealthMonitor"};
    sdbusplus::message_t getProperties =
        conn.new_method_call(entityManager.c_str(), objPath.c_str(),
                             getAllInterface.c_str(), "GetAll");
    getProperties.append(targetInterface.c_str());
    sdbusplus::message_t resp = conn.call(getProperties);
    if (resp.is_method_error())
    {
        lg2::error("Error calling GetAll method");
        return false;
    }
    std::map<std::string, PropertyType> properties;
    resp.read(properties);

    /* GPIO Line message */
    std::string lineMsg = "GPIO Line ";

    /* GPIO line */
    gpiod_line* line = NULL;

    std::string lineName = "";

    /* Either falling or rising is healthy */
    bool healthyPolarity;

    /* Status for current GPIO */
    int currentHealthy = 1;

    uint64_t limit = 1;

    /* D-Bus object path */
    std::string gpioObjectPath = "/xyz/openbmc_project/HealthMonitor/";

    std::string errMsg;

    /* GPIO line configuration, default to monitor both edge */
    struct gpiod_line_request_config config
    {
        "gpio_monitor", GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, 0
    };

    if (properties.find("GpioLineName") == properties.end())
    {
        /* Gpio Line Name should be defined */
        lg2::error("Failed to find gpio line name.");
        return false;
    }
    else
    {
        /* Find the GPIO line */
        lineName = std::get<std::string>(properties["GpioLineName"]);
        lineMsg += lineName;
        line = gpiod_line_find(lineName.c_str());
    }

    if (line == NULL)
    {
        lg2::error("Failed to find the {GPIO}", "GPIO", errMsg);
        return false;
    }
    gpioObjectPath += lineName;

    // Request the line for input
    if (gpiod_line_request_input(line, "gpioHealthMonitor") < 0)
    {
        lg2::error("Failed to request line as input.");
        return false;
    }

    currentHealthy = gpiod_line_get_value(line);
    if (currentHealthy < 0)
    {
        lg2::error("Failed to read current GPIO value");
        return false;
    }
    gpiod_line_release(line);

    /* Get event to be monitored, if it is not defined then
     * Both rising falling edge will be monitored.
     */
    if (properties.find("GpioHealthyPolarity") == properties.end())
    {
        lg2::error("Failed to find HealthyPolarity.");
        return false;
    }
    /* Rising is true, Falling is flase */
    healthyPolarity =
        std::get<std::string>(properties["GpioHealthyPolarity"]) == "Rising";

    if (properties.find("LimitMonitoring") == properties.end())
    {
        lg2::error("Failed to find LimitMonitoring.");
        return false;
    }

    limit = std::get<uint64_t>(properties["LimitMonitoring"]);

    /* Create a monitor object and let it do all the rest */
    gpios.insert(std::make_unique<phosphor::gpio::GpioMonitor>(
        line, config, conn.get_io_context(), lineMsg, gpioObjectPath,
        healthyPolarity, currentHealthy, objPath));
    dbusObjects.push_back(std::make_unique<GetStatus>(
        conn, gpioObjectPath.c_str(), currentHealthy, objPath, limit));
    return true;
}

std::unique_ptr<sdbusplus::bus::match_t>
    setupPropertiesChangedMatches(sdbusplus::asio::connection& conn)
{
    lg2::info("Begin to listen");
    std::string match_expression =
        "type='signal',member='PropertiesChanged',path_namespace='/xyz/openbmc_project/inventory',arg0namespace='xyz.openbmc_project.Configuration.HealthMonitor'";
    auto match = std::make_unique<sdbusplus::bus::match_t>(
        static_cast<sdbusplus::bus_t&>(conn), match_expression,
        [&conn](sdbusplus::message_t& message) {
        std::lock_guard<std::mutex> lock(handlerMutex);
        const auto* path = message.get_path();
        if (exist.find(path) != exist.end())
            return;
        exist.insert(path);
        lg2::info("Insert {PATH}", "PATH", path);
        if (createHealthObject(conn, path) == false)
        {
            lg2::error("Failed to create health object for {NAME}.", "NAME",
                       path);
        }
    });
    return match;
}

int main()
{
    lg2::info("Start app");
    boost::asio::io_context io;
    auto bus = sdbusplus::bus::new_default();
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    std::string instance_path = "/xyz/openbmc_project/HealthMonitor";

    /* Request D-bus service name and path */
    conn->request_name("xyz.openbmc_project.HealthMonitor");
    sdbusplus::server::manager_t manager{bus, instance_path.c_str()};

    std::vector<std::string> objPaths;

    /* Find object under this path */
    std::string subtree = "/xyz/openbmc_project/inventory";
    int32_t depth = 0;
    std::vector<std::string> targetInterfaces = {
        "xyz.openbmc_project.Configuration.HealthMonitor"};
    bool ready = false;
    size_t prev = 0;

    // Polling to test the entity manager is ready
    while (!ready)
    {
        // Create a new method call
        sdbusplus::message_t method_call = conn->new_method_call(
            objMapper.c_str(), objMapperPath.c_str(),
            objMapperInterface.c_str(), getSubTreeMethod.c_str());

        // Append the arguments to the method call
        method_call.append(subtree.c_str());
        method_call.append(depth);
        method_call.append(targetInterfaces);
        try
        {
            prev = objPaths.size();
            // Call the method and get the reply
            sdbusplus::message_t reply = conn->call(method_call);
            // Read the object paths from the reply
            objPaths.clear();
            reply.read(objPaths);
            if (objPaths.size() != 0 && prev == objPaths.size())
            {
                ready = true;
            }
            else
            {
                sleep(2);
            }
        }
        catch (const sdbusplus::exception::exception& e)
        {
            ready = false;
            // pause to give the server a chance to start up
            sleep(2);
        }
    }

    if (objPaths.size() == 0)
    {
        lg2::error("Failed to get the gpio object path.");
        return -1;
    }

    for (const auto& objPath : objPaths)
    {
        if (exist.find(objPath) != exist.end())
            continue;
        exist.insert(objPath);
        if (createHealthObject(*conn, objPath) == false)
        {
            lg2::error("Failed to create health object for {NAME}.", "NAME",
                       objPath);
            continue;
        }
    }
    auto match = setupPropertiesChangedMatches(*conn);
    io.run();

    return 0;
}
