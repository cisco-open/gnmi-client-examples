#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mgbl_api.h"
#include "mgbl_api_impl.h"
#include "nlohmann/json.hpp"
//#include "mock_gnmi.h"
#include "gnmi.grpc.pb.h"
#include "gnmi/mgbl_gnmi_client.h"
#include "pbr/mgbl_pbr.h"

using json = nlohmann::json;
using namespace mgbl_api;

/*
 * Edge cases unit tests for subscribe_request_pbr_helper
 *
 * See mgbl_api_helper_test.cc
 */

/*
Dummy class to test the subscribe_request_helper
*/
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

/*
 * Edge cases unit tests for subscribe_request_helper
 *
 * See mgbl_api_test.cpp
 */

/*
 * We test subscribe_request_helper with incomplete pbr_arguments.
 */
TEST(SubscribeRequestHelperTest, EmptyPathTest)
{
    rpc_channel_args channel_args;
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);
    auto* impl_instance = instance.get_impl();

    gnmi::SubscribeRequest request;
    rpc_args rpc_args;
    rpc_stream_args info;

    rpc_args.mode = stream_mode::STREAM;
    rpc_args.encoding = gnmi::Encoding::JSON_IETF;
    rpc_args.sample_interval_nsec = 5000;
    info.paths_of_interest.push_back("");  // Empty path

    internal_error_code err = impl_instance->subscribe_request_helper(&request, rpc_args, info);

    // Should return success
    EXPECT_EQ(err, internal_error_code::SUCCESS);

    // Verify the subscription is set even with empty path
    EXPECT_EQ(request.subscribe().mode(),
              static_cast<gnmi::SubscriptionList_Mode>(stream_mode::STREAM));
    EXPECT_EQ(request.subscribe().encoding(), gnmi::Encoding::JSON_IETF);

    // Path should be empty
    const gnmi::Path& path = request.subscribe().subscription(0).path();
    EXPECT_EQ(path.elem_size(), 0);  // No elements in path
}

/*
 * We test if subscribe_request_helper does not throw any exceptions if the path is invalid
 */
TEST(SubscribeRequestHelperTest, InvalidPathTest)
{
    rpc_channel_args channel_args;
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);
    auto* impl_instance = instance.get_impl();

    gnmi::SubscribeRequest request;
    rpc_args rpc_args;
    rpc_stream_args info;

    rpc_args.mode = stream_mode::STREAM;
    rpc_args.encoding = gnmi::Encoding::JSON_IETF;
    rpc_args.sample_interval_nsec = 5000;
    info.paths_of_interest.push_back("system//interfaces///interface");  // Invalid path

    internal_error_code err = impl_instance->subscribe_request_helper(&request, rpc_args, info);

    // Should return success
    EXPECT_EQ(err, internal_error_code::SUCCESS);

    // Verify the subscription is set even with empty path
    EXPECT_EQ(request.subscribe().mode(),
              static_cast<gnmi::SubscriptionList_Mode>(stream_mode::STREAM));
    EXPECT_EQ(request.subscribe().encoding(), gnmi::Encoding::JSON_IETF);

    // Path should be empty
    const gnmi::Path& path = request.subscribe().subscription(0).path();
    EXPECT_EQ(path.elem_size(), 0);  // No elements in path
}

/*
 * Edge cases unit tests for check_response
 *
 * See mgbl_api_test.cpp
 */

/*
 * We test if check_response returns a NO_PREFIX_IN_RESPONSE error
 * if the notification has no prefix.
 */
TEST(GnmiResponseTest, CheckResponseMissingPrefix)
{
    rpc_channel_args channel_args;
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);
    auto* impl_instance = instance.get_impl();
    gnmi::SubscribeResponse response;
    pbr_stats_basic stats;

    response.mutable_update();  // Creates an empty update

    auto result = impl_instance->check_response(response, *pbr_counters);

    EXPECT_EQ(result.second, internal_error_code::NO_PREFIX_IN_RESPONSE);
}

/*
 * We test if check_response returns a NO_NOTIFICATION error
 * if the notification has no update.
 */
TEST(GnmiResponseTest, CheckResponseNoUpdate)
{
    rpc_channel_args channel_args;
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);
    auto* impl_instance = instance.get_impl();
    gnmi::SubscribeResponse response;  // With no update
    pbr_stats_basic stats;

    auto result = impl_instance->check_response(response, *pbr_counters);

    EXPECT_EQ(result.second, internal_error_code::NO_NOTIFICATION);
}

/*
 * We test if check_response forward an error if the notification update is invalid.
 */
TEST(GnmiResponseTest, CheckResponseInvalidUpdate)
{
    rpc_channel_args channel_args;
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);
    auto impl_instance = instance.get_impl();
    gnmi::SubscribeResponse response;
    pbr_stats_basic stats;

    gnmi::Notification* notification = response.mutable_update();

    /* Add an update that should trigger NO_POLICY_NAME_IN_RESPONSE */
    gnmi::Path prefix;
    prefix.set_origin("Cisco-IOS-XR-pbr-fwd-stats-oper");
    *notification->mutable_prefix() = prefix;
    gnmi::Update* update = notification->add_update();
    gnmi::Path* update_path = update->mutable_path();
    std::string json_value = R"({
        "fib-stats": {
            "byte-count": 1000
        }
    })";
    update->mutable_val()->set_json_ietf_val(json_value);

    auto result = impl_instance->check_response(response, *pbr_counters);

    EXPECT_EQ(result.second, internal_error_code::NO_POLICY_NAME_IN_RESPONSE);
}

/*
 * Edge cases unit tests for register_pbr_stats_once
 *
 * See mgbl_api_text.cc
 */

// Test case for write failure
/*TEST(GnmiStreamImpTest, WriteFailureTest) {
    // Create mock objects
    auto mock_stub = std::make_shared<MockGnmiStub>();
    auto mock_reader_writer = std::make_shared<MockClientReaderWriter>();
    GnmiClient stream_imp(mock_stub);

    // Set expectations
    EXPECT_CALL(*mock_stub, Subscribe).WillOnce(Return(mock_reader_writer));
    EXPECT_CALL(*mock_reader_writer, Write).WillOnce(Return(false));  // Simulate Write failure
    EXPECT_CALL(*mock_reader_writer, Finish).WillOnce(Return(Status::CANCELLED));

    // Input parameters
    std::string key_policy = "test_policy";
    std::string key_rule = "test_rule";
    client_context_arguments context_args = {"user", "password", false,
std::chrono::system_clock::now()}; rpc_arguments rpc_args; std::vector<pbr_stats> return_pbr;

    // Call the function
    auto result = stream_imp.register_pbr_stats_once(key_policy, key_rule, context_args, rpc_args,
return_pbr);

    // Verify result
    EXPECT_EQ(result.first, error_code::RPC_FAILURE);
    EXPECT_EQ(result.second.error_code(), grpc::StatusCode::CANCELLED);
}*/

// Test case for client type failure
/*TEST(GnmiStreamImpTest, ClientTypeFailureTest) {
    // Create mock objects
    auto mock_stub = std::make_shared<MockGnmiStub>();
    GnmiClient stream_imp(mock_stub);

    // Manually set client_type to something other than "pbr" or ""
    stream_imp.impl_->client_type = "other";

    // Input parameters
    std::string key_policy = "test_policy";
    std::string key_rule = "test_rule";
    client_context_arguments context_args = {"user", "password", false,
std::chrono::system_clock::now()}; rpc_arguments rpc_args; std::vector<pbr_stats> return_pbr;

    // Call the function
    auto result = stream_imp.register_pbr_stats_once(key_policy, key_rule, context_args, rpc_args,
return_pbr);

    // Verify result
    EXPECT_EQ(result.first, error_code::CLIENT_TYPE_FAILURE);
}*/

// Test register_pbr_stats_once with invalid policy and rule
/*TEST(GnmiStreamImpTest, RegisterPbrStatsOnceInvalidKeys) {
    GnmiClient stream(std::make_shared<grpc::Channel>(nullptr));  // Mock a channel

    std::vector<pbr_stats> pbr_data;
    client_context_arguments context_args;
    rpc_arguments rpc_args;

    auto result = stream.register_pbr_stats_once("invalid_policy", "invalid_rule", context_args,
rpc_args, pbr_data);

    EXPECT_EQ(result.first, error_code::RPC_FAILURE);
}*/

/*
 * Edge cases unit tests for register_pbr_stats_stream
 *
 * See mgbl_api_text.cc
 */

// Test case for client type failure
/*TEST(GnmiStreamImpTest, ClientTypeFailureTest) {
    auto mock_stub = std::make_shared<MockGnmiStub>();
    GnmiClient stream_imp(mock_stub);

    // Manually set client_type to something other than "pbr" or ""
    stream_imp.impl_->client_type = "other";

    // Input parameters
    std::string key_policy = "test_policy";
    std::string key_rule = "test_rule";
    client_context_arguments context_args = {"user", "password", false,
std::chrono::system_clock::now()}; rpc_arguments rpc_args;

    // Call the function
    auto result = stream_imp.register_pbr_stats_stream(key_policy, key_rule, context_args,
rpc_args);

    // Verify result
    EXPECT_EQ(result, error_code::CLIENT_TYPE_FAILURE);
}*/

// Test case for failed write operation
/*TEST(GnmiStreamImpTest, WriteFailureTest) {
    auto mock_stub = std::make_shared<MockGnmiStub>();
    auto mock_reader_writer = std::make_shared<MockClientReaderWriter>();
    GnmiClient stream_imp(mock_stub);

    // Set expectations for the mock objects
    EXPECT_CALL(*mock_stub, Subscribe).WillOnce(Return(mock_reader_writer));
    EXPECT_CALL(*mock_reader_writer, Write).WillOnce(Return(false));  // Simulate Write failure
    EXPECT_CALL(*mock_reader_writer, Finish).WillOnce(Return(Status::CANCELLED));

    // Input parameters
    std::string key_policy = "test_policy";
    std::string key_rule = "test_rule";
    client_context_arguments context_args = {"user", "password", false,
std::chrono::system_clock::now()}; rpc_arguments rpc_args;

    // Call the function
    auto result = stream_imp.register_pbr_stats_stream(key_policy, key_rule, context_args,
rpc_args);

    // Verify result
    EXPECT_EQ(result, error_code::WRITES_FAILED);
}*/

// Test case for RPC failure
/*TEST(GnmiStreamImpTest, RpcFailureTest) {
    auto mock_stub = std::make_shared<MockGnmiStub>();
    auto mock_reader_writer = std::make_shared<MockClientReaderWriter>();
    GnmiClient stream_imp(mock_stub);

    // Set expectations for the mock objects
    EXPECT_CALL(*mock_stub, Subscribe).WillOnce(Return(mock_reader_writer));
    EXPECT_CALL(*mock_reader_writer, Write).WillOnce(Return(true));
    EXPECT_CALL(*mock_reader_writer, Read).WillOnce(Return(false));  // No responses
    EXPECT_CALL(*mock_reader_writer, Finish).WillOnce(Return(Status(grpc::StatusCode::INTERNAL, "RPC
failure")));

    // Input parameters
    std::string key_policy = "test_policy";
    std::string key_rule = "test_rule";
    client_context_arguments context_args = {"user", "password", false,
std::chrono::system_clock::now()}; rpc_arguments rpc_args;

    // Call the function
    auto result = stream_imp.register_pbr_stats_stream(key_policy, key_rule, context_args,
rpc_args);

    // Verify result
    EXPECT_EQ(result, error_code::RPC_FAILURE);
}*/