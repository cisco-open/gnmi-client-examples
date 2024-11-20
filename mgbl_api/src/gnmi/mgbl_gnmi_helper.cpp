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

#include "gnmi/mgbl_gnmi_helper.h"
#include <fmt/format.h>
#include "../logger/logger.h"
namespace mgbl_api
{
/** \addtogroup gnmi
 *  @{
 */
/**
 * @brief Converts a gnmi::Path object to a string.
 *
 * @param path Pointer to the gnmi::Path object.
 * @return A string representation of the gnmi path.
 */
std::string gnmipath_to_string(const gnmi::Path& path)
{
    int path_elem_size = path.elem_size();
    std::string element;

    for (int i = 0; i < path_elem_size; i++)
    {
        element.append("/");
        element.append(path.elem(i).name());
        for (const auto& j : path.elem(i).key())
        {
            // Create "key=value" pair
            element.append("[" + j.first + "=" + j.second + "]");
        }
    }
    return element;
}

/**
 * @brief Converts a string to a gnmi::Path object.
 * Throws an exception if the string is not a valid gNMI path.
 *
 * @param path A string representing the gnmi path.
 * @return A gnmi::Path object.
 * @throws std::invalid_argument if the string does not comply with gnmi specification.
 */
gnmi::Path string_to_gnmipath(const std::string& path)
{
    gnmi::Path temp;
    std::string::size_type start = path[0] == '/' ? 1 : 0;  // Skip the leading slash
    std::string::size_type end = path.find('/', start);

    if (end == std::string::npos)
    {
        end = path.size();
    }

    while (start < path.size())
    {
        std::string elem = path.substr(start, end - start);

        if (elem.empty())
        {
            throw std::invalid_argument("Invalid gNMI path: empty path element.");
        }

        // Check if element contains key-value pair in square brackets
        std::string::size_type bracket_start = elem.find('[');
        if (bracket_start != std::string::npos)
        {
            // Ensure there is a closing bracket
            std::string::size_type bracket_end = elem.find(']');
            if (bracket_end == std::string::npos)
            {
                throw std::invalid_argument(
                    "Invalid gNMI path: missing closing bracket in key-value pair.");
            }

            // Extract the name part (before the '[')
            std::string name = elem.substr(0, bracket_start);
            if (name.empty())
            {
                throw std::invalid_argument(
                    "Invalid gNMI path: name before key-value pair cannot be empty.");
            }

            // Extract the key-value part (inside the square brackets)
            std::string key_value = elem.substr(bracket_start + 1, bracket_end - bracket_start - 1);

            // Ensure key-value pair is properly formatted
            std::string::size_type equal_pos = key_value.find('=');
            if (equal_pos == std::string::npos || equal_pos == 0 ||
                equal_pos == key_value.size() - 1)
            {
                throw std::invalid_argument(
                    "Invalid gNMI path: key-value pair must be "
                    "in the format 'key=value'.");
            }

            std::string key = key_value.substr(0, equal_pos);
            std::string value = key_value.substr(equal_pos + 1);

            auto* path_elem = temp.add_elem();
            path_elem->set_name(name);
            (*path_elem->mutable_key())[key] = value;
        }
        else
        {
            // If no key-value pair, just set the name
            temp.add_elem()->set_name(elem);
        }

        start = end + 1;
        end = path.find('/', start);
        if (end == std::string::npos)  // Handle last element
        {
            end = path.size();
        }
    }
    return temp;
}

/**
 * @brief Prints the contents of a gnmi::SubscribeRequest object.
 *
 * @param request Reference to the gnmi::SubscribeRequest object.
 */
void print_subscribe_request(gnmi::SubscribeRequest& request)
{
    int subs_size = 0;
    gnmi::Path prefix;
    gnmi::Path path;
    const auto& subscription = request.subscribe();

    std::cout << std::endl << "--------SubscribeRequest--------" << std::endl;
    if (subscription.has_prefix())
    {
        prefix = subscription.prefix();
        std::cout << "SubscriptionList Prefix: " << gnmipath_to_string(prefix) << std::endl;
    }

    subs_size = subscription.subscription_size();
    for (int j = 0; j < subs_size; j++)
    {
        std::cout << "Subscription[" << j << "]: " << std::endl;
        if (subscription.subscription(j).has_path())
        {
            path = subscription.subscription(j).path();
            std::cout << "Path: " << gnmipath_to_string(path) << std::endl;
        }
        else
        {
            std::cout << "Path: Does not Exist" << std::endl;
        }
    }
}
/** @}*/  // end of gnmi
}  // namespace mgbl_api