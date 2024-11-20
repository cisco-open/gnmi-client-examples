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

#include "pbr/mgbl_pbr.h"
#include <fmt/format.h>
#include <regex>
#include "gnmi/mgbl_gnmi_helper.h"
#include "logger/logger.h"
#include "mgbl_api.h"

namespace mgbl_api
{
/** \addtogroup pbr
 *  @{
 */

/**
 * @brief Converts the given map to a pbr_stats object.
 *
 * @param map A map containing string key-value pairs representing PBR statistics.
 */
std::shared_ptr<PBRBase::pbr_stat> PBRBasic::unordered_map_to_stats(
    const std::unordered_map<std::string, std::string>& map)
{
    auto stats = std::make_shared<PbrBasicStat>();
    auto it = map.find("policy_name");
    if (it != map.end())
    {
        stats->policy_name = it->second;
    }

    it = map.find("rule_name");
    if (it != map.end())
    {
        stats->rule_name = it->second;
    }

    it = map.find("/fib-stats/byte-count");
    if (it != map.end())
    {
        stats->byte_count = std::stoull(it->second);
    }

    it = map.find("/fib-stats/packet-count");
    if (it != map.end())
    {
        stats->packet_count = std::stoull(it->second);
    }

    it = map.find("/fib-stats/collection-timestamp/seconds");
    if (it != map.end())
    {
        stats->collection_timestamp_seconds = std::stoull(it->second);
    }

    it = map.find("/fib-stats/collection-timestamp/nano-seconds");
    if (it != map.end())
    {
        stats->collection_timestamp_nanoseconds = std::stoull(it->second);
    }

    it = map.find("/paction/policy-rule-action/act-un/path-grp-name");
    if (it != map.end())
    {
        stats->path_grp_name = it->second;
    }
    it = map.find("/paction/policy-rule-action/act-un/type");
    if (it != map.end())
    {
        stats->policy_action_type = it->second;
    }
    return stats;
}

std::vector<std::string> PBRBase::get_gnmi_paths() const
{
    std::vector<std::string> paths;
    for (const auto& key : keys)
    {
        std::stringstream path;

        path << path_origin + ":pbr-stats/policy-maps/policy-map[policy-name=" << key.key_policy
             << "]/rule-names/rule-name[rule-name=" << key.key_rule << "]/";
        paths.push_back(path.str());
    }
    return paths;
}

}  // namespace mgbl_api