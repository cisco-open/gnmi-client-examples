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
#include <getopt.h>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include "gnmi/mgbl_gnmi_client.h"
#include "mgbl_api.h"
#include "pbr/mgbl_pbr.h"

// https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-specification.md
/*
 * This tutorial code showcases how to use the mgbl_api library.
 * It will specifically go over examples on how to perform a gnmi
 * subscribe stream pbr request through the use of our api,
 * and showcase common error handling scenarios and how the user
 * can perform basic retries.
 */

using namespace mgbl_api;

// Derived class of the the logger implementation
class console_logger : public logger
{
   public:
    ~console_logger() override = default;

    // Log function is overriden to print the log to stdout
    void log(const std::string& message, log_level level) override
    {
        std::cout << logger::log_level_to_string(level) << ": " << message << '\n';
    }
};

int main(int argc, char** argv)
{
    auto pbr_counters = std::make_shared<PBRBasic>();
    // User needs to set up the channel_arguments into gnmi_client_connection
    rpc_channel_args channel_args;

    // Set server address. Can set unix sockets by setting as unix:/socket_file.sock
    channel_args.server_address = "111.111.111.111:11111";

    // User can add channel credentials to the channel_args
    // channel_args.pem_roots_certs_path
    // channel_args.pem_private_key_path
    // channel_args.pem_cert_chain_path

    // need to set up the client conetxt arguments
    client_context_args context_args;

    context_args.username = "username";
    context_args.password = "password";

    /* Typically it is recommended that the user set a deadline for the rpc.
     *We will set a deadline of 10 seconds in this example.
     */
    context_args.set_deadline = true;
    context_args.deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);

    // Configure the rpc arguments
    rpc_args rpc_args;

    // Set the encoding type for the subscription rpc. Default is gnmi::Encoding::PROTO
    // rpc_args.encoding = gnmi::Encoding::JSON_IETF;

    // Set the sample interval for a grpc stream. Default is 1 second
    const uint32_t SAMPLE_INTERVAL_SEC_DEFAULT = 2;
    uint32_t sample_interval_sec = SAMPLE_INTERVAL_SEC_DEFAULT;
    rpc_args.sample_interval_nsec *= sample_interval_sec;

    // Our vector of pbr_keys which will be passed into the pbr register functions
    PBRBase::pbr_key pbr_stat{"p1", "r3_p1"};
    pbr_counters->keys.push_back(pbr_stat);

    // Example of pushing two policy_rule combinations into the one request

    PBRBase::pbr_key pbr_stat2;
    pbr_stat2 = {"p1", "r2_p1"};
    pbr_counters->keys.push_back(pbr_stat2);

    // Example of how to implement own logging
    auto console_logger_instance = std::make_shared<console_logger>();
    logger_manager::get_instance().set_logger(console_logger_instance);

    // Create the gnmi_client_connection instance
    gnmi_client_connection testConnection(channel_args);

    // Optional: User can try to establish the connection before trying to get any pbr stats data.
    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{5};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{10};
    const std::chrono::seconds STREAM_SLEEP_DURATION_SECONDS{10};

    if (testConnection.wait_for_grpc_server_connection(GRPC_SERVER_CONNECTION_RETRY_COUNT,
                                                       GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS))
    {
        GnmiClient newStream(testConnection.get_channel(), pbr_counters);
        std::deque<std::pair<bool, grpc::Status>> my_deque;
        std::mutex queue_mtx;
        std::condition_variable queue_cv;

        /*
         * In this example, we start the stream with an inital request.
         * Depending on the rpc_failed_handler and the rpc_success_handler
         * above, we can exit or retry.
         * Errors or Success pair put into a the queue by the rpc_success_handler
         * and rpc_failed_handler
         * According to them, we decide what we want to do.
         * In this example, after we get 10 responses successfully, we will exit
         * or if we reach 5 retries
         */
        int number_of_responses = 10;
        int count_retry = 0;
        int retry_stream = 5;
        int count = 0;
        bool retry = false;
        bool stream_ended = false;
        newStream.set_rpc_success_handler(
            [&](std::shared_ptr<CounterInterface> iface)
            {
                auto derived_iface = std::dynamic_pointer_cast<PBRBasic>(iface);
                if (!derived_iface->stats.empty())
                {
                    const auto& response_stats = derived_iface->stats.back();
                    std::cout << "Here are the Pbr stats listed: SubscribeResponse "
                              << std::this_thread::get_id() << std::endl;
                    std::cout << "       policy-name:  " << response_stats.policy_name << std::endl;
                    std::cout << "       rule-name:    " << response_stats.rule_name << std::endl;
                    std::cout << "       byte-count:   " << response_stats.byte_count << std::endl;
                    std::cout << "       packet-count: " << response_stats.packet_count
                              << std::endl;
                    std::cout << "       collection-timestamp seconds:     "
                              << response_stats.collection_timestamp_seconds << std::endl;
                    std::cout << "       collection-timestamp nanoseconds: "
                              << response_stats.collection_timestamp_nanoseconds << std::endl;
                    std::cout << "       path_grp_name:      " << response_stats.path_grp_name
                              << std::endl;
                    std::cout << "       policy_action_type: " << response_stats.policy_action_type
                              << std::endl;
                    count++;
                    if (count >= number_of_responses)
                    {
                        // If we reached number of responses we want to exit
                        logger_manager::get_instance().log("Response limit reached. Let's stop the stream",
                                                        log_level::INFO);
                        std::pair<bool, grpc::Status> temp;
                        temp.first = false;
                        std::unique_lock<std::mutex> lock(queue_mtx);
                        my_deque.push_back(temp);
                        queue_cv.notify_one();
                    }
                }
            });

        // Set lambda for rpc_failed_handler
        newStream.set_rpc_failed_handler(
            [&](grpc::Status status)
            {
                logger_manager::get_instance().log("rpc_failed_handler: This rpc failed", log_level::ERROR);
                if (!status.ok())
                {
                    std::pair<bool, grpc::Status> temp;
                    temp.first = true;
                    temp.second = status;
                    std::unique_lock<std::mutex> lock(queue_mtx);
                    my_deque.push_back(temp);
                    queue_cv.notify_one();
                }
            });

        while (count_retry < retry_stream)
        {
            // Upon retry, the context deadline should be updated if used
            if (context_args.set_deadline)
            {
                context_args.deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
            }

            // The error_code for the stream case can be used as an extra check,
            // but is not required for the stream case.
            // Instead we implemented a deque to handle rpc failures.
            error_code err;

            err = newStream.rpc_register_stats_stream(context_args, rpc_args);
            if (err != error_code::CLIENT_TYPE_FAILURE)
            {
                // All error_code return type which is not CLIENT_TYPE_FAILURE
                // is related to a grpc::Status. Must be handled through rpc_failed_handler.
                // SUCCESS handled through pbr_function_handler

                // Here is an example of adding another seperate request on the same stream.
                // The user can do this by calling the register function again.
                err = newStream.rpc_register_stats_stream(context_args, rpc_args);

                std::unique_lock<std::mutex> lock(queue_mtx);
                queue_cv.wait(lock, [&] { return !my_deque.empty(); });
                std::pair<bool, grpc::Status> rpc_err_status = my_deque.front();
                my_deque.pop_front();
                lock.unlock();

                // If the rpc_err_status.first is true, it indicates that an error occured.
                // grpc::Status exists only when error occured
                if (rpc_err_status.first)
                {
                    // https://grpc.github.io/grpc/cpp/md_doc_statuscodes.html
                    if (rpc_err_status.second.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
                    {
                        std::string msg = fmt::format(
                            "Stream stopped due to deadline exceeded\n"
                            "Error Code: {} \n"
                            "Error message: {} \n",
                            static_cast<int>(rpc_err_status.second.error_code()),
                            rpc_err_status.second.error_message());
                        logger_manager::get_instance().log(msg, log_level::ERROR);
                        // Upon Deadline Exceed in stream case we choose to not retry.
                        // Close the stream and join the receive thread
                        newStream.rpc_stream_close();
                        break;
                    }
                    else if (rpc_err_status.second.error_code() == grpc::StatusCode::UNAVAILABLE)
                    {
                        // If Unavailable, it is best to retry after a certain backoff period
                        // How the user signals to do that, is up to them
                        std::string msg = fmt::format(
                            "Stream retry due to Unavailable error\n"
                            "Error Code: {} \n"
                            "Error message: {} \n",
                            static_cast<int>(rpc_err_status.second.error_code()),
                            rpc_err_status.second.error_message());
                        logger_manager::get_instance().log(msg, log_level::ERROR);

                        // Close the stream and try again after backoff
                        newStream.rpc_stream_close();
                        count_retry++;

                        // Simulating retry backoff period by making thread sleep
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                    }
                    else if (rpc_err_status.second.error_code() ==
                             grpc::StatusCode::UNAUTHENTICATED)
                    {
                        // User can preform reauthentication if they wish to
                        // In this example we just retry with the same credentials set
                        std::string msg = fmt::format(
                            "Stream stopped due to Unauthentication error\n"
                            "Error Code: {} \n"
                            "Error message: {} \n",
                            static_cast<int>(rpc_err_status.second.error_code()),
                            rpc_err_status.second.error_message());
                        logger_manager::get_instance().log(msg, log_level::ERROR);

                        // Close the stream and try again
                        newStream.rpc_stream_close();
                        count_retry++;
                    }
                    else
                    {
                        // In our example, we do not want to retry on any other error codes
                        logger_manager::get_instance().log(
                            "Stream stopped for other reason. Let's quit this", log_level::ERROR);
                        // Close the stream and try again
                        newStream.rpc_stream_close();
                        break;
                    }
                }
                else
                {
                    // Signifies no error occured and all responses received
                    break;
                }
            }
            else
            {
                logger_manager::get_instance().log("Error: Using unsupported client type",
                                                   log_level::ERROR);
                // Even if the stream was not successful, it is okay to call close
                newStream.rpc_stream_close();

                // Since CLIENT_TYPE_FAILURE is not in relation to a grpc::Status
                // we would not want to retry
                break;
            }
        }
    }
    else
    {
        logger_manager::get_instance().log("Initial connection attempts failed", log_level::ERROR);
    }
    return 0;
}