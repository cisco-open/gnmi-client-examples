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
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include "gnmi/mgbl_gnmi_client.h"
#include "mgbl_api.h"
#include "pbr/mgbl_pbr.h"

// https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-specification.md

/*
 * This tutorial code showcases how to use the mgbl_api library.
 * It will specifically go over examples on how to perform a gnmi
 * subscribe once pbr request through the use of our api,
 * and showcase common error handling scenarios and how the user
 * can perform retries.
 * This example behaves atomically, from sending the request,
 * to recieving all responses. So if an error occures within that
 * time it will retry on the entire request in certain error cases.
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

// Helper function to print successful pbr_stats
void print_pbr(std::vector<PbrBasicStat> pbr_vector)
{
    for (const auto& single_test_once : pbr_vector)
    {
        // We want to print the resposnes on Success
        std::cout << "Here are the Pbr stats listed: SubscribeResponse " << std::endl;
        std::cout << "       policy-name:  " << single_test_once.policy_name << std::endl;
        std::cout << "       rule-name:    " << single_test_once.rule_name << std::endl;
        std::cout << "       byte-count:   " << single_test_once.byte_count << std::endl;
        std::cout << "       packet-count: " << single_test_once.packet_count << std::endl;
        std::cout << "       collection-timestamp seconds:     "
                  << single_test_once.collection_timestamp_seconds << std::endl;
        std::cout << "       collection-timestamp nanoseconds: "
                  << single_test_once.collection_timestamp_nanoseconds << std::endl;
        std::cout << "       path_grp_name:      " << single_test_once.path_grp_name << std::endl;
        std::cout << "       policy_action_type: " << single_test_once.policy_action_type
                  << std::endl;
    }
}

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
     * We will set a deadline of 3 seconds in this example.
     */
    context_args.set_deadline = true;
    context_args.deadline = std::chrono::system_clock::now() + std::chrono::seconds(3);

    // Configure the rpc arguments
    rpc_args rpc_args;

    // Set the encoding type for the subscription rpc. Default is gnmi::Encoding::PROTO
    rpc_args.encoding = gnmi::Encoding::JSON_IETF;

    // Our vector of pbr_keys which will be passed into the pbr register functions
    pbr_counters->keys.push_back({"p1", "r3_p1"});
    // Can push multiple policies rule combinations into the one request
    // pbr_counters->keys.push_back({"p1", "r2_p1"});

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
        /* In order to use the gnmi_imp_stream class, the use must provide a channel object
         * the user can get the grpc::channel object from the gnmi_client_connection method
         */
        GnmiClient newStream(testConnection.get_channel(), pbr_counters);

        // The user can choose to run register_pbr_once as many times as they would like
        std::vector<PbrBasicStat> test_once;

        // Error structure returned by register function
        std::pair<error_code, grpc::Status> err;

        err = newStream.rpc_register_stats_once(context_args, rpc_args);

        // Lets go over simple error handling examples

        if (err.first == error_code::SUCCESS)
        {
            auto test_once = pbr_counters->stats;
            print_pbr(test_once);
        }
        else if (err.first == error_code::CLIENT_TYPE_FAILURE)
        {
            // A client type indicator error indicates the user tried to make different client-type
            // requests within the same instance of gnmi_stream_imp. grpc::Status will not be
            // filled.
            std::string msg =
                fmt::format("Stream once error code: {}\n", static_cast<int>(err.first));
            logger_manager::get_instance().log(msg, log_level::ERROR);

            // User can build a new instace of the class and try again
            std::pair<error_code, grpc::Status> retry_err;
            std::vector<PbrBasicStat> retry_once;
            GnmiClient newClient(testConnection.get_channel(), pbr_counters);
            retry_err = newClient.rpc_register_stats_once(context_args, rpc_args);

            // Should do the checks and act acoordingly
            if (retry_err.first == error_code::SUCCESS)
            {
                print_pbr(retry_once);
            }
            else
            {
                std::string retry_msg =
                    fmt::format("Failed on second, Stream once. Error code: {}\n",
                                static_cast<int>(retry_err.first));
                logger_manager::get_instance().log(retry_msg, log_level::ERROR);
            }
        }
        else
        {
            // Every other error_code indicates a rpc failure status code exists
            // In our example these retries do not check for every error code, to
            // keep the example simple and clean. But the user most likely would.

            // https://grpc.github.io/grpc/cpp/md_doc_statuscodes.html
            std::string msg = fmt::format(
                "Stream once error code: {} \n"
                "grpcStatus error_code: {} \n"
                "error message: {} \n",
                static_cast<int>(err.first), static_cast<int>(err.second.error_code()), err.second.error_message());
            logger_manager::get_instance().log(msg, log_level::ERROR);

            // Upon retry, the context deadline should be updated if used
            if (context_args.set_deadline)
            {
                context_args.deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
            }

            /* User will need to check which responses were returned before rpc failure occured
             * In our case, we will assume it all failed and redo all requests
             */
            std::vector<PbrBasicStat> retry_once;
            std::pair<error_code, grpc::Status> retry_err;

            if (err.second.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
            {
                // If a timeout occurs we could try again after extending the deadline
                // by 5 seconds
                context_args.deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
                retry_err = newStream.rpc_register_stats_once(context_args, rpc_args);

                if (retry_err.first == error_code::SUCCESS)
                {
                    print_pbr(retry_once);
                }
                else
                {
                    std::string retry_msg = fmt::format(
                        "Retry of Stream once error code: {}\n"
                        "grpcStatus error_code: {}\n"
                        "error message: {}\n",
                        static_cast<int>(retry_err.first), static_cast<int>(retry_err.second.error_code()),
                        retry_err.second.error_message());
                    logger_manager::get_instance().log(retry_msg, log_level::ERROR);
                }
            }
            else if (err.second.error_code() == grpc::StatusCode::UNAVAILABLE)
            {
                // If Unavailable, it is best to retry after a certain backoff period

                // Simulating retry backoff period by making thread sleep
                std::this_thread::sleep_for(std::chrono::seconds(5));
                retry_err = newStream.rpc_register_stats_once(context_args, rpc_args);

                if (retry_err.first == error_code::SUCCESS)
                {
                    print_pbr(retry_once);
                }
                else
                {
                    std::string retry_msg = fmt::format(
                        "Retry of Stream once error code: {}\n"
                        "grpcStatus error_code: {}\n"
                        "error message: {}\n",
                        static_cast<int>(retry_err.first), static_cast<int>(retry_err.second.error_code()),
                        retry_err.second.error_message());
                    logger_manager::get_instance().log(retry_msg, log_level::ERROR);
                }
            }
            else if (err.second.error_code() == grpc::StatusCode::UNAUTHENTICATED)
            {
                /* The request does not have the valid authentication credentials.
                 * Can provide new credentials and try again.
                 */

                // We just try to establish connection when the rpc call is made and check on the
                // status
                retry_err = newStream.rpc_register_stats_once(context_args, rpc_args);

                if (retry_err.first == error_code::SUCCESS)
                {
                    print_pbr(retry_once);
                }
                else
                {
                    std::string retry_msg = fmt::format(
                        "Retry of Stream once error code: {}\n"
                        "grpcStatus error_code: {}\n"
                        "error message: {}\n",
                        static_cast<int>(retry_err.first), static_cast<int>(retry_err.second.error_code()),
                        retry_err.second.error_message());
                    logger_manager::get_instance().log(retry_msg, log_level::ERROR);
                }
            }
            else
            {
                // User can decide how they handle these cases based off the error codes proivded by
                // grpc
            }
        }
    }
    else
    {
        logger_manager::get_instance().log("Initial connection attempts failed", log_level::ERROR);
    }
    return 0;
}