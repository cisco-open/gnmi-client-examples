#include "mgbl_api.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "gnmi/mgbl_gnmi_client.h"
#include "mgbl_api_impl.h"
#include "nlohmann/json.hpp"
#include "pbr/mgbl_pbr.h"
//#include "gnmi_mock.grpc.pb.h"

using json = nlohmann::json;
// using ::testing::Return;
using namespace mgbl_api;

/*
 * Unit tests for subscribe_request_helper
 *
 * subscribe_request_helper help set up a SubscribeRequest.
 * It sets the mode (STREAM or ONCE), the encoding and more.
 * It creates a subscription associated with the SubscribeRequest.
 *
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

    std::function<void(std::shared_ptr<GnmiCounters>)> success_handler = [&](std::shared_ptr<CounterInterface> iface)
    {
        std::cout << "Here are the Pbr stats listed: SubscribeResponse "
                  << std::this_thread::get_id() << std::endl;
    };
    std::function<void(grpc::Status)> failure_handler = [&](grpc::Status status)
    { std::cout << "rpc_failed_handler: This rpc failed" << std::endl; };
};

/*
 * Unit tests for subscribe_request_helper
 *
 * subscribe_request_helper help set up a SubscribeRequest.
 * It sets the mode (STREAM or ONCE), the encoding and more.
 * It creates a subscription associated with the SubscribeRequest.
 *
 */

/*
 * We test if subscribe_request_helper accepts STREAM mode and JSON_IETF encoding.
 */
TEST(SubscribeRequestHelperTest, BasicPbrArgumentsTest)
{
    rpc_channel_args channel_args;
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);
    auto impl_instance = instance.get_impl();

    gnmi::SubscribeRequest request;
    rpc_args rpc_args;
    rpc_stream_args info;

    rpc_args.mode = stream_mode::STREAM;
    rpc_args.encoding = gnmi::Encoding::JSON_IETF;
    rpc_args.sample_interval_nsec = 5000;
    info.paths_of_interest.push_back("policy-maps/policy-map[policy-name=key_policy]/");

    internal_error_code err = impl_instance->subscribe_request_helper(&request, rpc_args, info);

    // Should return success
    EXPECT_EQ(err, internal_error_code::SUCCESS);

    // Verify subscription mode
    EXPECT_EQ(request.subscribe().mode(),
              static_cast<gnmi::SubscriptionList_Mode>(stream_mode::STREAM));

    // Verify encoding
    EXPECT_EQ(request.subscribe().encoding(), gnmi::Encoding::JSON_IETF);

    // Verify sample interval and subscription mode
    EXPECT_EQ(request.subscribe().subscription(0).mode(), gnmi::SubscriptionMode::SAMPLE);
    EXPECT_EQ(request.subscribe().subscription(0).sample_interval(), 5000);

    // Verify path
    const gnmi::Path& path = request.subscribe().subscription(0).path();
    EXPECT_EQ(path.elem_size(), 2);  // Should have two elements
    EXPECT_EQ(path.elem(0).name(), "policy-maps");
    EXPECT_EQ(path.elem(1).name(), "policy-map");
    EXPECT_EQ(path.elem(1).key().at("policy-name"), "key_policy");
}

/*
 * We test if subscribe_request_helper accepts STREAM mode and PROTO encoding.
 */
TEST(SubscribeRequestHelperTest, ModeStreamTest)
{
    rpc_channel_args channel_args;
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);
    auto impl_instance = instance.get_impl();

    gnmi::SubscribeRequest request;
    rpc_args rpc_args;
    rpc_stream_args info;

    rpc_args.mode = stream_mode::STREAM;
    rpc_args.encoding = gnmi::Encoding::PROTO;
    rpc_args.sample_interval_nsec = 15000;
    info.paths_of_interest.push_back(
        "policy-maps/policy-map[policy-name=key_policy]/rule-names/rule-name[rule-name=key_rule]/");

    internal_error_code err = impl_instance->subscribe_request_helper(&request, rpc_args, info);

    // Should return success
    EXPECT_EQ(err, internal_error_code::SUCCESS);

    // Verify subscription mode is STREAM
    EXPECT_EQ(request.subscribe().mode(),
              static_cast<gnmi::SubscriptionList_Mode>(stream_mode::STREAM));

    // Verify encoding
    EXPECT_EQ(request.subscribe().encoding(), gnmi::Encoding::PROTO);

    // Verify sample interval and subscription mode
    EXPECT_EQ(request.subscribe().subscription(0).mode(), gnmi::SubscriptionMode::SAMPLE);
    EXPECT_EQ(request.subscribe().subscription(0).sample_interval(), 15000);

    // Verify path
    const gnmi::Path& path = request.subscribe().subscription(0).path();
    EXPECT_EQ(path.elem_size(), 4);  // Should have three elements
    EXPECT_EQ(path.elem(0).name(), "policy-maps");
    EXPECT_EQ(path.elem(1).name(), "policy-map");
    EXPECT_EQ(path.elem(1).key().at("policy-name"), "key_policy");
    EXPECT_EQ(path.elem(2).name(), "rule-names");
    EXPECT_EQ(path.elem(3).name(), "rule-name");
    EXPECT_EQ(path.elem(3).key().at("rule-name"), "key_rule");
}

/*
 * We test if subscribe_request_helper accepts ONCE mode.
 */
TEST(SubscribeRequestHelperTest, ModeOnceTest)
{
    rpc_channel_args channel_args;
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);
    auto impl_instance = instance.get_impl();

    gnmi::SubscribeRequest request;
    rpc_args rpc_args;
    rpc_stream_args info;

    rpc_args.mode = stream_mode::ONCE;
    rpc_args.encoding = gnmi::Encoding::PROTO;
    rpc_args.sample_interval_nsec = 10000;
    info.paths_of_interest.push_back("pbr-stats/policy-maps/rule-names/");

    internal_error_code err = impl_instance->subscribe_request_helper(&request, rpc_args, info);

    // Should return success
    EXPECT_EQ(err, internal_error_code::SUCCESS);

    // Verify subscription mode
    EXPECT_EQ(request.subscribe().mode(),
              static_cast<gnmi::SubscriptionList_Mode>(stream_mode::ONCE));

    // Verify encoding
    EXPECT_EQ(request.subscribe().encoding(), gnmi::Encoding::PROTO);

    // Verify sample interval and subscription mode
    EXPECT_EQ(request.subscribe().subscription(0).mode(), gnmi::SubscriptionMode::SAMPLE);
    EXPECT_EQ(request.subscribe().subscription(0).sample_interval(), 10000);

    // Verify path
    const gnmi::Path& path = request.subscribe().subscription(0).path();
    EXPECT_EQ(path.elem_size(), 3);  // Should have three elements
    EXPECT_EQ(path.elem(0).name(), "pbr-stats");
    EXPECT_EQ(path.elem(1).name(), "policy-maps");
    EXPECT_EQ(path.elem(2).name(), "rule-names");
}

/*
 * Unit tests for check_response
 *
 * check_response verifies if the received SubscribeResponse is a valid object.
 * If it is valid, it will decode the response and populate the pbr_stats object.
 *
 * If the SubscribeResponse has no prefix, NO_PREFIX_IN_RESPONSE error code is returned
 * and the response is not decoded. Else SUCCESS is returned.
 *
 */

/*
 * We test if check_response returns SUCCESS with a valid prefix.
 */
TEST(GnmiResponseTest, CheckResponseValidPrefix)
{
    rpc_channel_args channel_args;
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    instance.set_rpc_success_handler(instance.success_handler);
    instance.set_rpc_failed_handler(instance.failure_handler);
    auto impl_instance = instance.get_impl();

    gnmi::SubscribeResponse response;
    gnmi::Notification* notification = response.mutable_update();

    gnmi::Update* update = notification->add_update();
    gnmi::Path* prefix = notification->mutable_prefix();
    prefix->set_origin("Cisco-IOS-XR-pbr-fwd-stats-oper");

    gnmi::Path* update_path = update->mutable_path();
    *update_path = string_to_gnmipath(
        "pbr-stats/policy-maps/policy-map[policy-name=key_policy]/rule-names/"
        "rule-name[rule-name=key_rule]/");

    gnmi::TypedValue* val = update->mutable_val();
    val->set_json_ietf_val(R"({"byte-count": 1000, "packet-count": 500})");

    pbr_stats_basic stats;
    auto result = impl_instance->check_response(response, *pbr_counters);

    EXPECT_EQ(result.second, internal_error_code::SUCCESS);
}

/*
 * MOCKS NOT WORKING FOR NOW
 */

/*
 * Unit tests for register_pbr_stats_once
 *
 * register_pbr_stats_once is sending a request and waiting for a response.
 * With context_args we open a grpc stub. Then we initiate a grpc write call
 * for the `key_policy` and `key_rule`.
 *
 * We then wait for a response up to the set deadline.
 * (If no deadline is set then the thread will wait until the server responds.)
 * Once the response received it is decoded and we terminate the session.
 *
 * If the transaction is successful return_pbr is filled with the response data.
 * Else a vector is returned with grpc error code and status.
 *
 * register_pbr_stats_once is blocking call.
 *
 */

/*
 * We test if register_pbr_stats_once returns an Error for not getting a response in time.
 */
/*TEST(GnmiStreamImpTest, SuccessfulRequestTest) {
    // Create mock objects
    auto mock_stub = std::shared_ptr<gnmi::MockgNMIStub>();
    auto mock_reader_writer  = std::shared_ptr<grpc::ClientReaderWriter<gnmi::SubscribeRequest,
gnmi::SubscribeResponse>>();

    channel_arguments channel_args = {"server_uri", false, "", "", ""};
    gnmi_client_connection testConnection(channel_args);

    GnmiClient stream_imp(testConnection.get_channel());
    stream_imp.impl_->stub.reset(); // Using FRIEND_TEST to access private members
    stream_imp.impl_->stub = mockStub;

    // Set expectations
    EXPECT_CALL(mock_stub, Subscribe).WillOnce(Return(mock_reader_writer));
    EXPECT_CALL(*mock_reader_writer, Write).WillOnce(Return(true));
    EXPECT_CALL(*mock_reader_writer, Read).WillOnce(Return(false));  // No responses
    EXPECT_CALL(*mock_reader_writer, WritesDone).WillOnce(Return(true));
    EXPECT_CALL(*mock_reader_writer, Finish).WillOnce(Return(Status::OK));

    // Input parameters
    std::vector<pbr_key> pbr_keys;
    pbr_keys.push_back({"key_policy", "key_rule"});
    client_context_arguments context_args = {"user", "password", false,
std::chrono::system_clock::now()}; rpc_arguments rpc_args; std::vector<pbr_stats> return_pbr;

    // Call register_pbr_stats_once
    auto result = stream_imp.register_pbr_stats_once(pbr_keys, context_args, rpc_args, return_pbr);

    // Verify result
    EXPECT_EQ(result.first, error_code::RPC_FAILURE);
    EXPECT_FALSE(result.second.ok());
}
}*/

/*
 * We test if register_pbr_stats_once returns en Error after the deadline exceeded.
 */
/*class DeadlineTestDerived : public GnmiClient
{
    using GnmiClient::GnmiClient;
};
TEST(GnmiStreamImpTest, DeadlineTest) {
    auto console_logger_instance = std::make_shared<console_logger>();
    logger_manager::get_instance().set_logger(console_logger_instance);

    int tolerance = 50; // 50ms
    int expected_delay = 500; // 500ms
    auto delay = std::chrono::system_clock::now() + std::chrono::milliseconds(expected_delay);

    channel_arguments channel_args = {"server_uri", false, "", "", ""};
    std::vector<pbr_key> pbr_keys;
    client_context_arguments context_args = {"user", "password", true, delay};
    rpc_arguments rpc_args;
    std::vector<pbr_stats> test_once;
    std::pair<error_code, grpc::Status> err;

    pbr_keys.push_back({"key_policy", "key_rule"});
    rpc_args.encoding = gnmi::Encoding::JSON_IETF;
    rpc_args.sample_interval_nsec = 5000;

    gnmi_client_connection connection(channel_args);
    DeadlineTestDerived newStream(connection.get_channel());

    auto start_time = std::chrono::steady_clock::now();
    err = newStream.register_pbr_stats_once(pbr_keys, context_args, rpc_args, test_once);
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
start_time).count();

    // Verify result
    EXPECT_EQ(err.first, error_code::RPC_FAILURE);
    EXPECT_FALSE(err.second.ok());
    EXPECT_GE(elapsed_time, expected_delay - tolerance);
    EXPECT_LE(elapsed_time, expected_delay + tolerance);
}*/

/*
 * We test if register_pbr_stats_once returns SUCCESS for a valid request
 */
/*TEST(GnmiStreamImpTest, ReadResponseSuccessTest) {
    // Create mock objects
    auto mock_stub = std::make_shared<MockGnmiStub>();
    auto mock_reader_writer = std::make_shared<MockClientReaderWriter>();
    GnmiClient stream_imp(mock_stub);

    // Set expectations
    EXPECT_CALL(*mock_stub, Subscribe).WillOnce(Return(mock_reader_writer));
    EXPECT_CALL(*mock_reader_writer, Write).WillOnce(Return(true));
    EXPECT_CALL(*mock_reader_writer, Read).WillOnce(Invoke([](gnmi::SubscribeResponse* response) {
        // Simulate a valid response
        gnmi::Notification* notification = response->mutable_update();
        notification->mutable_prefix()->set_origin("Cisco-IOS-XR");
        return true;
    }));
    EXPECT_CALL(*mock_reader_writer, Finish).WillOnce(Return(Status::OK));

    // Input parameters
    std::string key_policy = "test_policy";
    std::string key_rule = "test_rule";
    client_context_arguments context_args = {"user", "password", false,
std::chrono::system_clock::now()}; rpc_arguments rpc_args; std::vector<pbr_stats> return_pbr;

    // Call the function
    auto result = stream_imp.register_pbr_stats_once(key_policy, key_rule, context_args, rpc_args,
return_pbr);

    // Verify result
    EXPECT_EQ(result.first, error_code::SUCCESS);
    EXPECT_TRUE(result.second.ok());
    EXPECT_EQ(return_pbr.size(), 1);  // Verify that one pbr_stats was returned
}*?


/*
 * Unit tests for register_pbr_stats_stream
 *
 * register_pbr_stats_once is sending a request and waiting for a response.
 * With context_args we open a grpc stub.
 *
 * If a receiver thread is active we initiate a grpc write call
 * for the `key_policy` and `key_rule`. If it is not we create it before initiating a write call.
 *
 * If a transaction is successful pbr_function_handler will be called filled with the response data.
 * Else rpc_failed_handler will be called with a grpc status.
 *
 * The session is terminated ONLY if stream_pbr_close is called.
 *
 */

/*
 * We test if register_pbr_stats_stream returns SUCCESS for a valid request and response handling
 */
/*TEST(GnmiStreamImpTest, SuccessfulStreamRequestTest) {
    auto mock_stub = std::make_shared<MockGnmiStub>();
    auto mock_reader_writer = std::make_shared<MockClientReaderWriter>();
    GnmiClient stream_imp(mock_stub);

    // Set expectations for the mock objects
    EXPECT_CALL(*mock_stub, Subscribe).WillOnce(Return(mock_reader_writer));
    EXPECT_CALL(*mock_reader_writer, Write).WillOnce(Return(true));
    EXPECT_CALL(*mock_reader_writer, Read).WillOnce(Invoke([](gnmi::SubscribeResponse* response) {
        // Simulate a valid response
        gnmi::Notification* notification = response->mutable_update();
        notification->mutable_prefix()->set_origin("Cisco-IOS-XR");
        return true;
    }));
    EXPECT_CALL(*mock_reader_writer, Finish).WillOnce(Return(Status::OK));

    // Input parameters
    std::string key_policy = "test_policy";
    std::string key_rule = "test_rule";
    client_context_arguments context_args = {"user", "password", false,
std::chrono::system_clock::now()}; rpc_arguments rpc_args;

    // Call the function
    auto result = stream_imp.register_pbr_stats_stream(key_policy, key_rule, context_args,
rpc_args);

    // Verify result
    EXPECT_EQ(result, error_code::SUCCESS);
}*/

/*
 * We test if register_pbr_stats_stream returns SUCCESS for a successful write but no response
 */
/*TEST(GnmiStreamImpTest, SuccessfulWriteNoResponseTest) {
    auto mock_stub = std::make_shared<MockGnmiStub>();
    auto mock_reader_writer = std::make_shared<MockClientReaderWriter>();
    GnmiClient stream_imp(mock_stub);

    // Set expectations for the mock objects
    EXPECT_CALL(*mock_stub, Subscribe).WillOnce(Return(mock_reader_writer));
    EXPECT_CALL(*mock_reader_writer, Write).WillOnce(Return(true));
    EXPECT_CALL(*mock_reader_writer, Read).WillOnce(Return(false));  // No response to read
    EXPECT_CALL(*mock_reader_writer, Finish).WillOnce(Return(Status::OK));

    // Input parameters
    std::string key_policy = "test_policy";
    std::string key_rule = "test_rule";
    client_context_arguments context_args = {"user", "password", false,
std::chrono::system_clock::now()}; rpc_arguments rpc_args;

    // Call the function
    auto result = stream_imp.register_pbr_stats_stream(key_policy, key_rule, context_args,
rpc_args);

    // Verify result
    EXPECT_EQ(result, error_code::SUCCESS);
}*/