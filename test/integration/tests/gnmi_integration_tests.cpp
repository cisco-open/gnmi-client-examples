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
#include "gnmi/mgbl_gnmi_client.h"
#include "mgbl_api.h"
#include "pbr/mgbl_pbr.h"

// https://github.com/openconfig/reference/blob/master/rpc/gnmi/gnmi-specification.md
using namespace mgbl_api;
std::mutex stream_mtx;
std::condition_variable stream_cv;
bool stream_ended = false;
// Derived class for the gnmi_stream_imp.
class Derived : public GnmiClient
{
   public:
    Derived(const std::shared_ptr<grpc::Channel>& channel, std::shared_ptr<GnmiCounters> interface)
        : GnmiClient(channel, interface)
    {
    }

    GnmiClientDetails* get_impl()
    {
        return this->impl_.get();
    }

    std::function<void(std::shared_ptr<CounterInterface>)> success_handler = [&](std::shared_ptr<CounterInterface> iface)
    {
        std::cout << "Here are the Pbr stats listed: SubscribeResponse "
                  << std::this_thread::get_id() << std::endl;
    };
    std::function<void(grpc::Status)> failure_handler = [&](grpc::Status status)
    { std::cout << "rpc_failed_handler: This rpc failed" << std::endl; };
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
    rpc_channel_args channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);

    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{50};
    EXPECT_TRUE(dummyConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
}

TEST(GRPCTest, StreamInitializationTest)
{
    rpc_channel_args channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);

    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{5};
    EXPECT_TRUE(dummyConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
    const auto channel = dummyConnection.get_channel();
    EXPECT_EQ(instance.rpc_stream_close(), error_code::SUCCESS);
}

TEST(GRPCTest, StreamTLSInitializationTest)
{
    rpc_channel_args channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);
    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{5};

    EXPECT_TRUE(dummyConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
    const auto channel = dummyConnection.get_channel();
    EXPECT_EQ(instance.rpc_stream_close(), error_code::SUCCESS);
}

TEST(GRPCTest, StreamOnceBasicTest)
{
    rpc_channel_args channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);

    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{5};
    EXPECT_TRUE(dummyConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
    const auto channel = dummyConnection.get_channel();

    pbr_counters->keys.push_back({"p1", "r3_p1"});
    client_context_args context_args;
    context_args.username = "foo";
    context_args.password = "bar";
    rpc_args rpc_args;
    rpc_args.encoding = gnmi::Encoding::PROTO;
    std::pair<error_code, grpc::Status> err;

    err = instance.rpc_register_stats_once(context_args, rpc_args);
    EXPECT_EQ(err.first, error_code::SUCCESS);
    EXPECT_EQ(instance.rpc_stream_close(), error_code::SUCCESS);
}

TEST(GRPCTest, StreamBasicTest)
{
    auto console_logger_instance = std::make_shared<console_logger>();
    logger_manager::get_instance().set_logger(console_logger_instance);
    rpc_channel_args channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);

    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{10};
    EXPECT_TRUE(dummyConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));
    const auto channel = dummyConnection.get_channel();
    int num_responses = 1;

    PBRBase::pbr_key pbr_stat{"p1", "r3_p1"};
    pbr_counters->keys.push_back(pbr_stat);
    client_context_args context_args;
    context_args.username = "foo";
    context_args.password = "bar";
    rpc_args rpc_args;
    rpc_args.encoding = gnmi::Encoding::PROTO;

    int count_retry = 0;
    int retry_stream = 5;
    int count = 0;
    bool retry = false;
    bool stream_ended = false;
    instance.set_rpc_success_handler(
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
                std::cout << "       packet-count: " << response_stats.packet_count << std::endl;
                std::cout << "       collection-timestamp seconds:     "
                          << response_stats.collection_timestamp_seconds << std::endl;
                std::cout << "       collection-timestamp nanoseconds: "
                          << response_stats.collection_timestamp_nanoseconds << std::endl;
                std::cout << "       path_grp_name:      " << response_stats.path_grp_name
                          << std::endl;
                std::cout << "       policy_action_type: " << response_stats.policy_action_type
                          << std::endl;
                count++;
                if (count >= num_responses)
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
    instance.set_rpc_failed_handler(
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
                    std::cout << "Stream stopped for other reason. Let's quit this" << std::endl;
                    retry = false;
                }
                std::lock_guard<std::mutex> lock(stream_mtx);
                stream_ended = true;
                stream_cv.notify_one();
            }
        });
    error_code err = instance.rpc_register_stats_stream(context_args, rpc_args);
    EXPECT_EQ(err, error_code::SUCCESS);
    EXPECT_EQ(instance.rpc_stream_close(), error_code::SUCCESS);
}

// That makes sense here if you think about it, but in the beginning I was confused because test
// passes correctly.
TEST(GRPCTest, StreamOnceWrongKeyAndPolicyTest)
{
    rpc_channel_args channel_args;
    channel_args.server_address = "localhost:50051";
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);

    const int GRPC_SERVER_CONNECTION_RETRY_COUNT{1};
    const std::chrono::seconds GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS{5};
    EXPECT_TRUE(dummyConnection.wait_for_grpc_server_connection(
        GRPC_SERVER_CONNECTION_RETRY_COUNT, GRPC_SERVER_CONNECTION_TIMEOUT_SECONDS));

    std::string policy = "fdassafdhusdfahjfsakdfh";
    std::string rule = "vxjackhsduihsdfjksfad";
    pbr_counters->keys.push_back({policy, rule});
    client_context_args context_args;
    context_args.username = "foo";
    context_args.password = "bar";
    rpc_args rpc_args;
    rpc_args.encoding = gnmi::Encoding::PROTO;
    std::pair<error_code, grpc::Status> err;

    err = instance.rpc_register_stats_once(context_args, rpc_args);
    EXPECT_EQ(err.first, error_code::SUCCESS);
    EXPECT_EQ(instance.rpc_stream_close(), error_code::SUCCESS);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}