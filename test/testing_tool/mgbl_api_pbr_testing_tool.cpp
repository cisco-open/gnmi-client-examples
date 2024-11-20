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
#include <time.h>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include "gnmi/mgbl_gnmi_client.h"
#include "mgbl_api.h"
#include "pbr/mgbl_pbr.h"

// https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-specification.md
using namespace mgbl_api;

const int GRPC_SERVER_CONNECTION_RETRY_COUNT{5};
const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{10};
const std::chrono::seconds STREAM_SLEEP_DURATION_SECONDS{10};

bool stream_ended = false;
bool stream_case = false;
// Derived class for the gnmi_stream_imp.

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
    rpc_channel_args channel_args;
    client_context_args context_args;
    rpc_args rpc_args;

    int option_char = 0;
    int option_long = 0;

    auto pbr_counters = std::make_shared<PBRBasic>();

    const struct option longopts[] = {
        {"stream_case", no_argument, nullptr, 'z'},
        {"retry_stream", required_argument, nullptr, 'y'},
        {"number_of_stream_responses", required_argument, nullptr, 'v'},
        {"server_ip", required_argument, nullptr, 'w'},
        {"ssl_tls", no_argument, nullptr, 'a'},
        {"pem_roots_certs_path", required_argument, nullptr, 'b'},
        {"pem_private_key_path", required_argument, nullptr, 'c'},
        {"pem_cert_chain_path", required_argument, nullptr, 'd'},
        {"username", required_argument, nullptr, 'u'},
        {"password", required_argument, nullptr, 'p'},
        {"deadline", required_argument, nullptr, 'g'},
        {"sample_interval", required_argument, nullptr, 'f'},
        {"encoding", required_argument, nullptr, 'e'},
        {"policy_rule", required_argument, nullptr, 'j'},

        {"help", no_argument, nullptr, 'h'},

        {nullptr, 0, nullptr, 0}};
    bool ssl_tls = false;
    bool stream_case = false;
    int retry_stream = 1;
    int number_of_stream_responses = 1;
    const uint32_t SAMPLE_INTERVAL_SEC_DEFAULT = 10;
    uint32_t sample_interval_sec = SAMPLE_INTERVAL_SEC_DEFAULT;
    std::string policy_rule;
    std::string policy;
    std::string rule;
    size_t pos = 0;
    std::chrono::milliseconds deadline_var;
    int response_number = 0;

    while ((option_long =
                getopt_long(argc, argv, "y:v:w:ab:c:d:u:p:g:f:e:j:hz", longopts, nullptr)) != -1)
    {
        switch (option_long)
        {
            case 'z':
                stream_case = true;
                break;
            case 'y':
                retry_stream = std::stoi(optarg);
                break;
            case 'v':
                number_of_stream_responses = std::stoi(optarg);
                break;
            case 'w':
                channel_args.server_address = optarg;
                break;
            case 'a':
                ssl_tls = true;
                break;
            case 'b':
                channel_args.pem_roots_certs_path = optarg;
                break;
            case 'c':
                channel_args.pem_private_key_path = optarg;
                break;
            case 'd':
                channel_args.pem_cert_chain_path = optarg;
                break;
            case 'u':
                context_args.username = optarg;
                break;
            case 'p':
                context_args.password = optarg;
                break;
            case 'g':
                context_args.set_deadline = true;
                context_args.deadline =
                    std::chrono::system_clock::now() + std::chrono::seconds(std::stoi(optarg));
                break;
            case 'f':
                sample_interval_sec = std::stoi(optarg);
                break;
            case 'e':
                if (std::string(optarg) == "JSON")
                {
                    rpc_args.encoding = gnmi::Encoding::JSON_IETF;
                }
                else if (std::string(optarg) == "PROTO")
                {
                    rpc_args.encoding = gnmi::Encoding::PROTO;
                }
                else
                {
                    std::cout << "Invalid encoding. Please use JSON or PROTO" << std::endl;
                    return 0;
                }
                break;
            case 'j':
                policy_rule = optarg;
                pos = policy_rule.find(':');

                if (pos != std::string::npos)
                {
                    policy = policy_rule.substr(0, pos);
                    rule = policy_rule.substr(pos + 1);
                    pbr_counters->keys.push_back({policy, rule});
                }
                break;
            case 'h':
                std::cout << "----------------------------------------- Arguments that Have to "
                             "be Set ----------------------------------------------\n";
                std::cout << "-w, --server_ip <server_ip:server-port>\t\t(Default: "
                             "empty string)\n";
                std::cout << "-u, --username <username>\t\t\t(Requires "
                             "argument. Default: empty string)\n";
                std::cout << "-p, --password <password>\t\t\t(Requires "
                             "argument. Default: empty string)\n";
                std::cout << "-j, --policy_rule <policy_key>:<rule_key>\t(Requires "
                             "argument: Used for PBR. Can repeat it to add multiple "
                             "paths in one SubscribeRequest. Default: empty string)\n";
                std::cout << "-z, --stream_case\t\t\t\t(No argument "
                             "required, if not used once case is the default)\n";
                std::cout << "-v, --number_of_stream_responses <number>\t(Requires "
                             "argument: Calls number_of_stream_responses after certain amount of "
                             "seconds. Default 1 second. Only used with stream_case)\n";
                std::cout << "----------------------------------------- Arguments which are "
                             "optional ----------------------------------------------\n";
                std::cout
                    << "-y, --retry_stream <number>\t\t\t(Requires "
                       "argument. Specifies how many times the program should attempt to connect "
                       "to the server. Default: 1)\n";
                std::cout
                    << "-a, --ssl_tls\t\t\t\t\t(No argument required: "
                       "Used for tls secure connection. If not, set uses insecure connection)\n";
                std::cout << "-b, --pem_roots_certs_path <path>\t\t"
                             "(Requires argument:"
                             "path to the root certification file. Used in conjuction with ssl_tls "
                             "set. Default: empty string)\n";
                std::cout << "-c, --pem_private_key_path <path>\t\t"
                             "(Requires argument: path to the pem private key file"
                             "Used in conjuction with ssl_tls set. Default: empty string)\n";
                std::cout << "-d, --pem_cert_chain_path <path>\t\t"
                             "(Requires argument. "
                             "Used in conjuction with ssl_tls set. Default: empty string)\n";
                std::cout
                    << "-g, --deadline <deadline>\t\t\t(Requires argument: "
                       "Enter any amount of seconds to set timeout. Default: no timeout set)\n";
                std::cout << "-f, --sample_interval <sample_interval>\t\t(Requires "
                             "argument: Enter any amount of seconds for sample interval. Only used "
                             "in stream subscription case. Default: 10)\n";
                std::cout << "-e, --encoding <encoding>\t\t\t(Requires "
                             "argument: Enter PROTO, or JSON. Default: PROTO)\n";
                std::cout << "-h, --help\t\t\t\t\t(This message)\n\n";
                std::cout
                    << "Example: ./gnmi_client --stream_case --server_ip "
                       "111.111.111.111:11111 --username username --password password "
                       "--deadline 10 --policy_rule p1:r3_p1 --number_of_stream_responses 10\n";
                return 0;
            default:
                return 0;
        }
    }

    // Example of how to implement own logging
    auto console_logger_instance = std::make_shared<console_logger>();
    logger_manager::get_instance().set_logger(console_logger_instance);

    // Create the gnmi_client_connection instance
    gnmi_client_connection testConnection(channel_args);

    // We try to establish the connection before trying to get any pbr stats data.
    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{5};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{10};
    const std::chrono::seconds STREAM_SLEEP_DURATION_SECONDS{10};

    rpc_args.sample_interval_nsec *= sample_interval_sec;
    if (testConnection.wait_for_grpc_server_connection(GRPC_SERVER_CONNECTION_RETRY_COUNT,
                                                       GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS))
    {
        // Pass in channel object into the defined class instance
        GnmiClient newStream(testConnection.get_channel(), pbr_counters);
        std::mutex stream_mtx;
        std::condition_variable stream_cv;
        bool retry = false;
        int count = 0;

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
                    if (count >= number_of_stream_responses)
                    {
                        std::cout << "Response limit reached. Let's stop the stream" << std::endl;
                        retry = false;
                        std::lock_guard<std::mutex> lock(stream_mtx);
                        stream_ended = true;
                        stream_cv.notify_one();
                    }
                }
            });

        // Set lambda for rpc_failed_handler
        newStream.set_rpc_failed_handler(
            [&](grpc::Status status)
            {
                std::cout << "rpc_failed_handler: This rpc failed" << std::endl;
                if (!status.ok())
                {
                    if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED)
                    {
                        std::cout << "Stream stopped due to deadline exceeded" << std::endl;
                        retry = true;
                    }
                    else
                    {
                        // Any other error, we do not want to retry
                        // https://grpc.github.io/grpc/cpp/md_doc_statuscodes.html
                        std::cout << "Stream stopped for other reason. Let's quit this"
                                  << std::endl;
                        retry = false;
                    }
                    std::lock_guard<std::mutex> lock(stream_mtx);
                    stream_ended = true;
                    stream_cv.notify_one();
                }
            });

        if (stream_case)
        {
            /*
             * In this example, we start the stream with an inital request.
             * Depending on the pbr_function_handler and the rpc_handler_function
             * above, we can exit or retry.
             */
            int count_retry = 0;
            while (count_retry < retry_stream)
            {
                if (context_args.set_deadline)
                {
                    // reset the context deadline
                    context_args.deadline = std::chrono::system_clock::now() + deadline_var;
                }
                response_number = number_of_stream_responses;

                auto check_retry = [&retry, &count, &stream_mtx, &stream_ended]()
                {
                    bool try_retry = retry;
                    retry = false;
                    count = 0;
                    std::lock_guard<std::mutex> lock(stream_mtx);
                    stream_ended = false;
                    return try_retry;
                };
                error_code err = newStream.rpc_register_stats_stream(context_args, rpc_args);
                if (err == error_code::SUCCESS)
                {
                    std::unique_lock<std::mutex> lock(stream_mtx);
                    stream_cv.wait(lock, [&stream_ended] { return stream_ended; });
                    lock.unlock();
                    newStream.rpc_stream_close();
                    if (check_retry())
                    {
                        std::cout << "We will attempt a retry" << std::endl;
                        count_retry++;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    count_retry++;
                }
            }
        }
        else
        {
            // The user can choose to run register_pbr_once as many times as they would like
            std::pair<error_code, grpc::Status> err;

            err = newStream.rpc_register_stats_once(context_args, rpc_args);
            auto test_once = pbr_counters->stats;
            if (err.first == error_code::SUCCESS)
            {
                for (const auto& single_test_once : test_once)
                {
                    std::cout << "Here are the Pbr stats listed: SubscribeResponse " << std::endl;
                    std::cout << "       policy-name:  " << single_test_once.policy_name
                              << std::endl;
                    std::cout << "       rule-name:    " << single_test_once.rule_name << std::endl;
                    std::cout << "       byte-count:   " << single_test_once.byte_count
                              << std::endl;
                    std::cout << "       packet-count: " << single_test_once.packet_count
                              << std::endl;
                    std::cout << "       collection-timestamp seconds:     "
                              << single_test_once.collection_timestamp_seconds << std::endl;
                    std::cout << "       collection-timestamp nanoseconds: "
                              << single_test_once.collection_timestamp_nanoseconds << std::endl;
                    std::cout << "       path_grp_name:      " << single_test_once.path_grp_name
                              << std::endl;
                    std::cout << "       policy_action_type: "
                              << single_test_once.policy_action_type << std::endl;
                }
            }
            else
            {
                // User can choose what to do next based off the error code return and the grpc
                // status object
                std::cout << "Stream once error code: " << (int)err.first
                          << " grpcStatus error_code: " << err.second.error_code() << std::endl;
            }
        }
    }
    return 0;
}