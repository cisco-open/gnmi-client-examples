#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include "gnmi.grpc.pb.h"
#include "gnmi.pb.h"
#include "mgbl_api.h"

// https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-specification.md
using namespace mgbl_api;
std::mutex stream_mtx;
std::condition_variable stream_cv;
bool stream_ended = false;
// Derived class for the gnmi_stream_imp.
class StreamExample : public gnmi_stream_imp
{
   public:
    bool retry;
    int number_of_responses;
    int count;

    StreamExample(std::shared_ptr<grpc::Channel> channel)
        : gnmi_stream_imp(std::move(channel)), retry(false), number_of_responses(1), count(0)
    {
    }

    void pbr_function_handler(const pbr_stats& response_stats) override
    {
        std::cout << "Here are the Pbr stats listed: SubscribeResponse "
                  << std::this_thread::get_id() << std::endl;
        std::cout << "       policy-name:  " << response_stats.policy_name << std::endl;
        std::cout << "       rule-name:    " << response_stats.rule_name << std::endl;
        std::cout << "       byte-count:   " << response_stats.byte_count << std::endl;
        std::cout << "       packet-count: " << response_stats.packet_count << std::endl;
        std::cout << "       collection-timestamp seconds:     "
                  << response_stats.collection_timestamp_seconds << std::endl;
        std::cout << "       collection-timestamp nanoseconds: "
                  << response_stats.collection_timestamp_nanoseconds << std::endl;
        std::cout << "       path_grp_name:      " << response_stats.path_grp_name << std::endl;
        std::cout << "       policy_action_type: " << response_stats.policy_action_type
                  << std::endl;
        count++;
        if (count >= number_of_responses)
        {
            std::cout << "Response limit reached. Let's stop the stream" << std::endl;
            retry = false;
            std::lock_guard<std::mutex> lock(stream_mtx);
            stream_ended = true;
            stream_cv.notify_one();
        }
    }

    void rpc_failed_handler(const grpc::Status& status) override
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
                std::cout << "Stream stopped for other reason. Let's quit this" << std::endl;
                retry = false;
            }
            std::lock_guard<std::mutex> lock(stream_mtx);
            stream_ended = true;
            stream_cv.notify_one();
        }
    }

    bool check_retry()
    {
        bool try_retry = retry;
        retry = false;
        count = 0;
        std::lock_guard<std::mutex> lock(stream_mtx);
        stream_ended = false;
        return try_retry;
    }

    /* Sets the number of responses we want to look for */
    void set_num_responses(int respones_amount)
    {
        number_of_responses = respones_amount;
    }
};

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

TEST(GRPCTest, ConnectivityTest)
{
    channel_arguments channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection testConnection(channel_args);

    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{50};
    EXPECT_TRUE(testConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
}

TEST(GRPCTest, StreamInitializationTest)
{
    channel_arguments channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection testConnection(channel_args);

    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{5};
    EXPECT_TRUE(testConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
    const auto channel = testConnection.get_channel();
    StreamExample streamExample(channel);
    EXPECT_EQ(streamExample.stream_pbr_close(), error_code::SUCCESS);
}

TEST(GRPCTest, StreamTLSInitializationTest)
{
    channel_arguments channel_args;
    channel_args.server_address = "localhost:50051";
    channel_args.ssl_tls = true;
    channel_args.pem_roots_certs_path = "test/integration/tests/example_tls/root.crt";
    gnmi_client_connection testConnection(channel_args);
    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{5};
    EXPECT_TRUE(testConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
    const auto channel = testConnection.get_channel();
    StreamExample streamExample(channel);
    EXPECT_EQ(streamExample.stream_pbr_close(), error_code::SUCCESS);
}

TEST(GRPCTest, StreamOnceBasicTest)
{
    channel_arguments channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection testConnection(channel_args);

    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{5};
    EXPECT_TRUE(testConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
    const auto channel = testConnection.get_channel();
    StreamExample streamExample(channel);

    std::string policy = "p1";
    std::string rule = "r3_p1";
    std::vector<pbr_key> pbr_keys;
    pbr_keys.push_back({policy, rule});
    client_context_arguments context_args;
    context_args.username = "foo";
    context_args.password = "bar";
    rpc_arguments rpc_args;
    rpc_args.encoding = gnmi::Encoding::PROTO;
    std::vector<pbr_stats> test_once;
    std::pair<error_code, grpc::Status> err;

    err = streamExample.register_pbr_stats_once(pbr_keys, context_args, rpc_args, test_once);
    EXPECT_EQ(err.first, error_code::SUCCESS);
    EXPECT_EQ(streamExample.stream_pbr_close(), error_code::SUCCESS);
}

TEST(GRPCTest, StreamBasicTest)
{
    auto console_logger_instance = std::make_shared<console_logger>();
    logger_manager::get_instance().set_logger(console_logger_instance);
    channel_arguments channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection testConnection(channel_args);

    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{10};
    EXPECT_TRUE(testConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
    const auto channel = testConnection.get_channel();
    StreamExample streamExample(channel);
    streamExample.set_num_responses(1);

    std::string policy = "p1";
    std::string rule = "r3_p1";
    std::vector<pbr_key> pbr_keys;
    pbr_keys.push_back({policy, rule});
    client_context_arguments context_args;
    context_args.username = "foo";
    context_args.password = "bar";
    rpc_arguments rpc_args;
    rpc_args.encoding = gnmi::Encoding::PROTO;
    std::vector<pbr_stats> test_once;

    error_code err = streamExample.register_pbr_stats_stream(pbr_keys, context_args, rpc_args);
    EXPECT_EQ(err, error_code::SUCCESS);
    EXPECT_EQ(streamExample.stream_pbr_close(), error_code::SUCCESS);
}

// That makes sense here if you think about it, but in the beginning I was confused because test
// passes correctly.
TEST(GRPCTest, StreamOnceWrongKeyAndPolicyTest)
{
    channel_arguments channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection testConnection(channel_args);

    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{5};
    EXPECT_TRUE(testConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
    const auto channel = testConnection.get_channel();
    StreamExample streamExample(channel);

    std::string policy = "fdassafdhusdfahjfsakdfh";
    std::string rule = "vxjackhsduihsdfjksfad";
    std::vector<pbr_key> pbr_keys;
    pbr_keys.push_back({policy, rule});
    client_context_arguments context_args;
    context_args.username = "foo";
    context_args.password = "bar";
    rpc_arguments rpc_args;
    rpc_args.encoding = gnmi::Encoding::PROTO;
    std::vector<pbr_stats> test_once;
    std::pair<error_code, grpc::Status> err;

    err = streamExample.register_pbr_stats_once(pbr_keys, context_args, rpc_args, test_once);
    EXPECT_EQ(err.first, error_code::SUCCESS);
    EXPECT_EQ(streamExample.stream_pbr_close(), error_code::SUCCESS);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}