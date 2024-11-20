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
#include "logger/logger.h"

namespace mgbl_api
{
/** \addtogroup logger
 *  @{
 */
/**
 * @brief Converts a log level to its string representation.
 *
 * @param level The log level to convert.
 * @return The string representation of the log level.
 */
const char* logger::log_level_to_string(log_level level)
{
    switch (level)
    {
        case log_level::VERBOSE:
            return "VERBOSE";
        case log_level::DEBUG:
            return "DEBUG";
        case log_level::INFO:
            return "INFO";
        case log_level::WARNING:
            return "WARNING";
        case log_level::ERROR:
            return "ERROR";
        case log_level::CRITICAL:
            return "CRITICAL";
    }
    return "NOTDEFINED";
}
/** @} */  // end of logger
}  // namespace mgbl_api