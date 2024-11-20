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
#ifndef MGBL_API_LOGGER_H_
#define MGBL_API_LOGGER_H_

#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace mgbl_api
{
/** \addtogroup logger
 *  @{
 */
/**
 * @brief Enum used to specify the severity of the log messages.
 */
enum class log_level
{
    VERBOSE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

/**
 * @brief Abstract base class for logging.
 */
class logger
{
   public:
    virtual ~logger() = default;

    /**
     * @brief Logs a message with a specified log level.
     *
     * @param message The message to log.
     * @param level The severity level of the log message.
     */
    virtual void log(const std::string& message, log_level level) = 0;

    /**
     * @brief Converts a log level to its string representation.
     *
     * @param level The log level to convert.
     * @return The string representation of the log level.
     */
    static const char* log_level_to_string(log_level level);
};

/**
 * @brief Manages the logger instance.
 */
class logger_manager
{
   public:
    /**
     * @brief Gets the singleton instance of the logger_manager.
     *
     * @return The singleton instance of the logger_manager.
     */
    static logger_manager& get_instance()
    {
        static logger_manager instance;
        return instance;
    }

    /**
     * @brief Sets the logger instance.
     *
     * @param logger_instance The logger instance to set.
     */
    void set_logger(std::shared_ptr<logger> logger_instance)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        this->logger_instance = std::move(logger_instance);
    }

    /**
     * @brief Logs a message with a specified log level using the current logger instance.
     *
     * @param message The message to log.
     * @param level The severity level of the log message.
     */
    void log(const std::string& message, log_level level)
    {
        std::shared_ptr<logger> temp_logger;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            temp_logger = logger_instance;
        }
        if (logger_instance)
        {
            logger_instance->log(message, level);
        }
    }

    logger_manager(const logger_manager&) = delete;
    logger_manager& operator=(const logger_manager&) = delete;
    logger_manager(logger_manager&&) = delete;
    logger_manager& operator=(logger_manager&&) = delete;

   private:
    logger_manager() = default;
    ~logger_manager() = default;

    std::shared_ptr<logger> logger_instance; /**< The current logger instance */
    std::mutex mutex_; /**< Mutex for thread-safe access to the logger instance */
};
/** @} */  // end of logger
}  // namespace mgbl_api
#endif  // MGBL_API_LOGGER_H_