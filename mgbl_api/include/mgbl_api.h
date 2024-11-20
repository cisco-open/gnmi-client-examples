/*
 * Copyright (c) 2024 Cisco Systems, Inc. and its affiliates
 * All rights reserved.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef MGBL_API_H_
#define MGBL_API_H_

#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>
#include <utility>
#include "gnmi/mgbl_gnmi_connection.h"
#include "gnmi/mgbl_gnmi_helper.h"
#include "logger/logger.h"

namespace mgbl_api
{
/**
 * @brief CounterInterface class is the base class for all the counters
 * that are to be implemented.
 */
class CounterInterface
{
   public:
    virtual ~CounterInterface() = default;
    virtual std::string name() = 0;
};

/**
 * @brief GnmiCounters class is the base class for GNMI counters
 * that are to be implemented.
 * get_gnmi_paths() function should return the path to the yang model of the
 * counter.
 */
class GnmiCounters : public CounterInterface
{
   public:
    virtual std::vector<std::string> get_gnmi_paths() const = 0;
    ~GnmiCounters() override = default;
};

/** \addtogroup pbr
 *  @{
 */
/**
 * @brief IPbrStat class is the base class for all the PBR statistics
 */
class IPbrStat
{
   public:
    virtual ~IPbrStat() = default;
};

/**
 * @brief PBRBase class is the base
 * class for all the PBR statistics that are to be implemented.
 * get_gnmi_paths() function should return the path to the yang model of the
 * counter.
 * unordered_map_to_stats() function should convert the unordered_map to the
 * stats object.
 * add_stats() function should add the stats object to the stats vector.
 */
class PBRBase : public GnmiCounters
{
   public:
    using pbr_stat = IPbrStat;
    std::vector<pbr_stat> stats;
    /**
     * @brief Struct to signify the key_policy and key_rule
     * combination the user want to search for,
     * with rpc functions
     */
    struct pbr_key
    {
        std::string key_policy; /**< The policy key */
        std::string key_rule;   /**< The rule key */
    };

    std::vector<pbr_key> keys;
    ~PBRBase() override = default;
    const std::string path_origin = "Cisco-IOS-XR-pbr-fwd-stats-oper"; /**< The path origin */
    virtual std::shared_ptr<IPbrStat> unordered_map_to_stats(
        const std::unordered_map<std::string, std::string>& map) = 0;
    std::vector<std::string> get_gnmi_paths() const override;
    virtual void add_stats(std::shared_ptr<pbr_stat> stat) = 0;
};
/** @} */
}  // namespace mgbl_api

//-----------------------------------------------------------------------------------------------
// The following code is used to generate the documentation for the mgbl_api library.
//-----------------------------------------------------------------------------------------------

/** @defgroup gnmi Gnmi module
 * @brief This module provides functionalities for gNMI (gRPC Network Management Interface)
 * operations.
 *
 * The gnmi module includes classes and functions to establish connections, send requests, and
 * handle responses from gNMI servers. It facilitates network management tasks such as retrieving
 * and setting configuration data, subscribing to updates, and managing telemetry data.
 *
 * The module includes:
 * - mgbl_gnmi_client: A client class to interact with gNMI servers.
 * - mgbl_gnmi_connection: A class to manage gNMI connections.
 * - mgbl_gnmi_helper: Helper functions and utilities for gNMI operations.
 */

/** @defgroup logger Logger module
 *  @brief This module provides functionalities for logging messages.
 * The logger module includes classes and functions to log messages with different log levels.
 * The module includes:
 * - logger: A class to log messages with different log levels.
 * - log_level: An enum class to define different log levels.
 */

/** @defgroup pbr PBR module
 *  @brief This module provides functionalities for Policy-Based Routing (PBR) operations.
 * The PBR module includes classes and functions to manage PBR policies and rules.
 * The module includes:
 * - PBR: A class to manage PBR policies and rules.
 * - PBR::stats_type: A struct to store PBR statistics.
 * - PBR::keys_type: A vector of strings to store policy and rule names.
 */

/** @defgroup rpc RPC module
 *  @brief This module provides functionalities for Remote Procedure Call (RPC) operations.
 * The RPC module includes classes and functions to manage RPC calls.
 * The module includes:
 * - rpc_channel_args: A struct to store channel arguments.
 * - rpc_stream_args: A struct to store stream arguments.
 * - rpc_args: A struct to store RPC arguments.
 * - error_code: An enum class to define error codes.
 * - stream_mode: An enum class to define stream modes.
 */
#endif  // MGBL_API_H_