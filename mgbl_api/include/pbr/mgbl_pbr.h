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
#ifndef MGBL_PBR_H_
#define MGBL_PBR_H_

#include <grpcpp/grpcpp.h>
#include <nlohmann/json.hpp>
#include <string>
#include "gnmi/mgbl_gnmi_helper.h"
#include "mgbl_api.h"

namespace mgbl_api
{
using json = nlohmann::json;

/** \addtogroup pbr
 *  @{
 */
struct pbr_stats_basic
{
};

/**
 * @brief Struct to hold the statistics of a pbr rule and policy combination.
 *
 * `policy_name` and `rule_name` corresponds to the initial request.
 */
class PbrBasicStat : public IPbrStat
{
   public:
    PbrBasicStat() = default;
    PbrBasicStat(const PbrBasicStat& other) = default;

    std::string policy_name;                       /**< The policy name */
    std::string rule_name;                         /**< The rule name */
    uint64_t byte_count = 0;                       /**< The byte count */
    uint64_t packet_count = 0;                     /**< The packet count */
    uint64_t collection_timestamp_seconds = 0;     /**< The collection timestamp in seconds */
    uint64_t collection_timestamp_nanoseconds = 0; /**< The collection timestamp in nanoseconds */
    std::string path_grp_name;                     /**< The path group name */
    std::string policy_action_type;                /**< The policy action type */
};

/**
 * @brief PBRBasic class is the concrete implementation to get PBR counters.
 */
class PBRBasic final : public PBRBase
{
   public:
    using pbr_stats = PbrBasicStat;
    std::vector<pbr_stats> stats; /**< The vector of pbr_stats */

    ~PBRBasic() final = default;

    /**
     * @brief Returns the name of the module.
     */
    std::string name() final
    {
        return "pbr";
    }

    /**
     * @brief Adds the given pbr_stats to the stats vector.
     *
     * @param stats The pbr_stats to add.
     */
    void add_stats(std::shared_ptr<pbr_stat> stat) final
    {
        const auto* derived_ptr = dynamic_cast<const PbrBasicStat*>(stat.get());
        if (derived_ptr != nullptr)
        {
            stats.push_back(*derived_ptr);
        }
        else
        {
            logger_manager::get_instance().log("Failed to add stat: incompatible type.",
                                               log_level::ERROR);
        }
    }

    /**
     * @brief Converts the given map to a pbr_stats object.
     */
    std::shared_ptr<IPbrStat> unordered_map_to_stats(
        const std::unordered_map<std::string, std::string>& map) final;

    // internal_error_code set_specific_data(std::string printed_path, IPbrStat& pbr_stat) final;
};
/** @} */  // end of pbr
}  // namespace mgbl_api
#endif  // MGBL_PRB_H_