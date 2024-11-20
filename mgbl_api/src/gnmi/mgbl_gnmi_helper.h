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
#ifndef MGBL_API_HELPER_H_
#define MGBL_API_HELPER_H_

#include <grpcpp/grpcpp.h>
#include <cstring>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <vector>
#include "gnmi.grpc.pb.h"

namespace mgbl_api
{
/** \addtogroup gnmi
 *  @{
 */
/**
 * @enum internal_error_code
 * @brief Used internally to specify type of error that occurred.
 */
enum class internal_error_code
{
    SUCCESS = 0,
    NO_NOTIFICATION,
    NO_PREFIX_IN_RESPONSE,
    NO_UPDATE_IN_NOTIFICATION,
    NO_POLICY_NAME_IN_RESPONSE,
    NO_RULE_NAME_IN_RESPONSE,
    UNSUPPORTED_ENCODING,
    UNKNOWN_ERROR
};

/**
 * @brief Struct for configuring the subscription paths.
 */
struct rpc_stream_args
{
    std::string prefix; /**< Prefix of the gnmi path */
    std::vector<std::string>
        paths_of_interest; /**< Vector of strings representing the paths to get data from */
};

/**
 * @brief Converts the gnmi path type to a string.
 *
 * gnmipath_to_string converts a gnmi path structure into a linear string.
 * The path should be as the gnmi specification.
 * The return string is of the form
 * "/policy-maps/policy-map[policy-name=key_policy]/rule-names/rule-name[rule-name=key_rule]"
 *
 * @param path A gnmi::Path object.
 * @return A string representation of the gnmi path.
 */
std::string gnmipath_to_string(const gnmi::Path& path);

/**
 * @brief Converts a string which contains the subscription path into a gnmi::Path.
 *
 * string_to_gnmipath converts a string into a gnmi path.
 * The string should be a valid path representing a valid gnmi structure.
 *
 * string_to_gnmipath raises a std::invalid_argument if the string does
 * not comply with gnmi specification:
 * (https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-path-conventions.md)
 *
 * @param path A string representing the gnmi path.
 * @return A gnmi::Path object.
 * @throws std::invalid_argument if the string does not comply with gnmi specification.
 */
gnmi::Path string_to_gnmipath(const std::string& path);

/**
 * @brief Prints the subscribe request.
 *
 * @param request Reference to the gnmi::SubscribeRequest object.
 */
void print_subscribe_request(gnmi::SubscribeRequest& request);

/** @}*/  // end of gnmi
}  // namespace mgbl_api
#endif  // MGBL_API_HELPER_H_