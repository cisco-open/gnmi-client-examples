#include <gtest/gtest.h>
#include "gnmi/mgbl_gnmi_client.h"
#include "gnmi/mgbl_gnmi_helper.h"
#include "nlohmann/json.hpp"
#include "pbr/mgbl_pbr.h"

using json = nlohmann::json;
using namespace mgbl_api;

/*
 * Edge cases unit tests for gnmipath_to_string
 *
 * See mgbl_api_helper_test.cpp
 */

/*
 * We test if gnmipath_to_string returns an empty path
 * from an empty string.
 */
TEST(GnmiPathTest, EmptyPathConversion)
{
    gnmi::Path path;
    std::string result = gnmipath_to_string(path);
    EXPECT_EQ(result, "");  // Expected format for empty path
}

/*
 * We test if gnmipath_to_string return a path
 * with an empty key value.
 */
TEST(GnmiPathTest, PathWithEmptyKeys)
{
    gnmi::Path path;
    auto* elem = path.add_elem();
    elem->set_name("interface");
    (*elem->mutable_key())["name"] = "";

    std::string result = gnmipath_to_string(path);
    EXPECT_EQ(result, "/interface[name=]");
}

/*
 * Unit tests for string_to_gnmipath
 *
 * See mgbl_api_helper_test.cpp
 */

/*
 * We test string_to_gnmipath returns an empty string
 * with an empty path.
 */
TEST(StringToGnmiPathTest, EmptyPathTest)
{
    std::string path = "";

    gnmi::Path result = string_to_gnmipath(path);

    // Check that no elements were added
    EXPECT_EQ(result.elem_size(), 0);
}

/*
 * We test if string_to_gnmipath returns a valid path
 * from a valid string with a trailing slash ('/').
 */
TEST(StringToGnmiPathTest, TrailingSlashTest)
{
    std::string path = "system/interfaces/";

    gnmi::Path result = string_to_gnmipath(path);

    // Check the number of path elements
    EXPECT_EQ(result.elem_size(), 2);

    // Verify each element
    EXPECT_EQ(result.elem(0).name(), "system");
    EXPECT_EQ(result.elem(1).name(), "interfaces");
}

/*
 * We test string_to_gnmipath returns a valid path
 * with multiple consecutive slashes.
 */
TEST(StringToGnmiPathTest, ConsecutiveSlashesTest)
{
    std::string path = "/system//interfaces///interface";

    /*
     * string_to_gnmipath should throw an exception
     * if the path is not compliant with gnmi standard
     */
    EXPECT_THROW(string_to_gnmipath(path), std::invalid_argument);
}

/*
 * Edge cases unit tests for gnmi_decode_json_ietf
 *
 * See mgbl_api_helper_test.cpp
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
};

/*
 * We test if gnmi_decode_json_ietf returns an empty struct
 * if the JSON string is incomplete.
 */
TEST(GnmiJsonTest, DecodeMissingFieldsJsonIetf)
{
    gnmi::Update update;
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);

    // JSON with missing fields
    std::string json_value = R"({
        "fib-stats": {
            "byte-count": 1000
        }
    })";

    update.mutable_val()->set_json_ietf_val(json_value);

    // Call the function
    std::shared_ptr<std::unordered_map<std::string, std::string>> raw_pbr_stat =
        std::make_shared<std::unordered_map<std::string, std::string>>();
    EXPECT_NO_THROW(instance.get_impl()->gnmi_decode_json_ietf(update, raw_pbr_stat));

    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/byte-count"], "1000");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/packet-count"], "");  // Should be default value
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/seconds"],
              "");  // Should be default value
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/nano-seconds"],
              "");  // Should be default value
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/path-grp-name"],
              "");                                                                // Should be empty
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/type"], "");  // Should be empty
}

// /*
//  * We test if gnmi_decode_json_ietf returns an empty struct
//  * if the JSON string is invalid. (A.k.a a field is of wrong type)
//  */
TEST(GnmiJsonTest, DecodeInvalidJsonIetf)
{
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    gnmi::Update update;
    // Invalid JSON
    std::string json_value = R"({ "fib-stats": {"byte-count": "abc" })";

    update.mutable_val()->set_json_ietf_val(json_value);

    std::shared_ptr<std::unordered_map<std::string, std::string>> raw_pbr_stat =
        std::make_shared<std::unordered_map<std::string, std::string>>();
    EXPECT_THROW(instance.get_impl()->gnmi_decode_json_ietf(update, raw_pbr_stat), json::exception);

    // We expect an empty struct
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/byte-count"], "");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/packet-count"], "");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/seconds"], "");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/nano-seconds"], "");
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/path-grp-name"], "");
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/type"], "");
}

// /*
//  * Edge cases unit tests for gnmi_decode_proto
//  *
//  * See mgbl_api_helper_test.cpp
//  */

/*
 * We test if gnmi_decode_proto returns an empty struct
 * if the path doesn't match.
 */
TEST(DecodeProtoTest, NonMatchingPathTest)
{
    gnmi::Update update;
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);

    gnmi::Path* path = update.mutable_path();
    path->add_elem()->set_name("random-path");

    update.mutable_val()->set_uint_val(1000);
    std::shared_ptr<std::unordered_map<std::string, std::string>> raw_pbr_stat =
        std::make_shared<std::unordered_map<std::string, std::string>>();
    instance.get_impl()->gnmi_decode_proto(update, raw_pbr_stat);

    // Verify no updates were made to pbr_stats
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/byte-count"], "");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/packet-count"], "");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/seconds"], "");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/nano-seconds"], "");
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/path-grp-name"], "");
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/type"], "");
}

/*
 * We test if gnmi_decode_proto returns a default value
 * if fields are missing.
 */
TEST(GnmiProtoTest, DecodeMissingFieldsProto)
{
    gnmi::Update update;
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);

    // Proto update with no value set
    gnmi::Path* path = update.mutable_path();
    path->add_elem()->set_name("byte-count");

    std::shared_ptr<std::unordered_map<std::string, std::string>> raw_pbr_stat =
        std::make_shared<std::unordered_map<std::string, std::string>>();
    instance.get_impl()->gnmi_decode_proto(update, raw_pbr_stat);

    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/byte-count"], "");  // Should be default value
}

/*
 * Edge cases unit tests for gnmi_parse_response
 *
 * See mgbl_api_helper_test.cpp
 */

/*
 * We test if gnmi_parse_response returns a NO_UPDATE_IN_NOTIFICATION error
 * if the notification is incomplete.
 */
TEST(GnmiParseResponseTest, CreateParseResponseIncompleteNotification)
{
    gnmi::Notification notification;
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);

    gnmi::Path* prefix = notification.mutable_prefix();
    prefix->set_origin("Cisco-IOS-XR-pbr-fwd-stats-oper");

    auto result =
        instance.get_impl()->gnmi_parse_response(notification, "Cisco-IOS-XR-pbr-fwd-stats-oper");

    EXPECT_EQ(result.second, internal_error_code::NO_UPDATE_IN_NOTIFICATION);
}

/*
 * We test if gnmi_parse_response returns a NO_POLICY_NAME_IN_RESPONSE error
 * if the notification is incomplete.
 */
TEST(GnmiParseResponseTest, CreateParseResponseNoPolicyNotification)
{
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);

    gnmi::Notification notification;
    gnmi::Path* prefix = notification.mutable_prefix();
    prefix->set_origin("Cisco-IOS-XR-pbr-fwd-stats-oper");

    gnmi::Update* update = notification.add_update();
    gnmi::Path* update_path = update->mutable_path();

    std::string json_value = R"({
        "fib-stats": {
            "byte-count": 1000
        }
    })";

    update->mutable_val()->set_json_ietf_val(json_value);

    auto result =
        instance.get_impl()->gnmi_parse_response(notification, "Cisco-IOS-XR-pbr-fwd-stats-oper");

    EXPECT_EQ(result.second, internal_error_code::NO_POLICY_NAME_IN_RESPONSE);
}

/*
 * We test if gnmi_parse_response returns a NO_RULE_NAME_IN_RESPONSE error
 * if the notification is incomplete.
 */
TEST(GnmiParseResponseTest, CreateParseResponseNoRuleNotification)
{
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);
    gnmi::Notification notification;

    gnmi::Path* prefix = notification.mutable_prefix();
    prefix->set_origin("Cisco-IOS-XR-pbr-fwd-stats-oper");

    gnmi::Update* update = notification.add_update();
    gnmi::Path* update_path = update->mutable_path();

    /* Add valid policy_name to not trigger NO_POLICY_NAME_IN_RESPONSE */
    auto* elem = update_path->add_elem();
    elem->set_name("policy-map");
    (*elem->mutable_key())["policy-name"] = "key_policy";

    std::string json_value = R"({
        "fib-stats": {
            "byte-count": 1000
        }
    })";

    update->mutable_val()->set_json_ietf_val(json_value);

    auto result =
        instance.get_impl()->gnmi_parse_response(notification, "Cisco-IOS-XR-pbr-fwd-stats-oper");

    EXPECT_EQ(result.second, internal_error_code::NO_RULE_NAME_IN_RESPONSE);
}

/*
 * We test if gnmi_parse_response returns a NO_PREFIX_IN_RESPONSE error
 * if the notification doesn't have anu prefix.
 */
TEST(GnmiParseResponseTest, CreateParseResponseNoPrefixNotification)
{
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);

    gnmi::Notification notification;
    gnmi::Update* update = notification.add_update();
    gnmi::Path* update_path = update->mutable_path();

    /* Add valid policy_name to not trigger NO_POLICY_NAME_IN_RESPONSE */
    auto* elem = update_path->add_elem();
    elem->set_name("policy-map");
    (*elem->mutable_key())["policy-name"] = "key_policy";

    std::string json_value = R"({
        "fib-stats": {
            "byte-count": 1000
        }
    })";

    update->mutable_val()->set_json_ietf_val(json_value);

    auto result = instance.get_impl()->gnmi_parse_response(notification, "");

    EXPECT_EQ(result.second, internal_error_code::NO_PREFIX_IN_RESPONSE);
}
