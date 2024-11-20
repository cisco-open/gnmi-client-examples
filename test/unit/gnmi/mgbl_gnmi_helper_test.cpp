#include "gnmi/mgbl_gnmi_helper.h"
#include <gtest/gtest.h>
#include "gnmi/mgbl_gnmi_client.h"
#include "nlohmann/json.hpp"
#include "pbr/mgbl_pbr.h"

using json = nlohmann::json;
using namespace mgbl_api;

/*
 * Unit tests for gnmipath_to_string
 *
 * gnmipath_to_string converts a gnmi path structure into a linear string.
 * The path should be as the gnmi specification.
 * The return string is of the form
 * "/policy-maps/policy-map[policy-name=key_policy]/rule-names/rule-name[rule-name=key_rule]"
 *
 */

/*
 * We test if gnmipath_to_string returns a valid string
 * from a valid path.
 */
TEST(GnmiPathTest, PathConversionToString)
{
    gnmi::Path path;
    path.add_elem()->set_name("interfaces");
    path.add_elem()->set_name("interface");

    std::string result = gnmipath_to_string(path);
    EXPECT_EQ(result, "/interfaces/interface");
}

/*
 * We test if gnmipath_to_string returns a valid string
 * from a valid path with multiple elements.
 */
TEST(GnmiPathTest, MultiElementPathConversion)
{
    gnmi::Path path;
    path.add_elem()->set_name("interfaces");
    path.add_elem()->set_name("interface");
    path.add_elem()->set_name("name");

    std::string result = gnmipath_to_string(path);
    EXPECT_EQ(result, "/interfaces/interface/name");
}

/*
 * We test if gnmipath_to_string returns a valid string
 * from a valid path with keys.
 */
TEST(GnmiPathTest, PathWithKeysConversion)
{
    gnmi::Path path;
    auto* elem = path.add_elem();
    elem->set_name("interface");
    (*elem->mutable_key())["name"] = "eth0";

    std::string result = gnmipath_to_string(path);
    EXPECT_EQ(result, "/interface[name=eth0]");
}

/*
 * Unit tests for string_to_gnmipath
 *
 * string_to_gnmipath converts a string into a gnmi path.
 * The string should be a valid path representing a valid gnmi structure.
 *
 */

/*
 * We test if string_to_gnmipath returns a valid path
 * from a valid string.
 */
TEST(StringToGnmiPathTest, SimplePathTest)
{
    std::string path = "/interfaces/interface";

    gnmi::Path result = string_to_gnmipath(path);

    // Check the number of path elements
    EXPECT_EQ(result.elem_size(), 2);

    // Verify each element
    EXPECT_EQ(result.elem(0).name(), "interfaces");
    EXPECT_EQ(result.elem(1).name(), "interface");
}

/*
 * We test string_to_gnmipath returns a correct path
 * with no leading slash.
 */
TEST(StringToGnmiPathTest, NoLeadingSlashTest)
{
    std::string path = "system/interfaces";

    gnmi::Path result = string_to_gnmipath(path);

    // Check the number of path elements
    EXPECT_EQ(result.elem_size(), 2);

    // Verify each element
    EXPECT_EQ(result.elem(0).name(), "system");
    EXPECT_EQ(result.elem(1).name(), "interfaces");
}

/*
 * We test if string_to_gnmipath returns a valid path
 * from a valid string with multiple elements.
 */
TEST(StringToGnmiPathTest, MultipleSlashesTest)
{
    std::string path = "/system/interfaces/interface/name";

    gnmi::Path result = string_to_gnmipath(path);

    // Check the number of path elements
    EXPECT_EQ(result.elem_size(), 4);

    // Verify each element
    EXPECT_EQ(result.elem(0).name(), "system");
    EXPECT_EQ(result.elem(1).name(), "interfaces");
    EXPECT_EQ(result.elem(2).name(), "interface");
    EXPECT_EQ(result.elem(3).name(), "name");
}

/*
 * We test if string_to_gnmipath returns a valid path
 * from a valid string with keys.
 */
TEST(StringToGnmiPathTest, PathWithKeysConversion)
{
    std::string path = "/interface[name=eth0]";

    gnmi::Path result = string_to_gnmipath(path);

    // Check the number of path elements
    EXPECT_EQ(result.elem_size(), 1);

    // Verify each element
    EXPECT_EQ(result.elem(0).name(), "interface");
    EXPECT_EQ(result.elem(0).key().at("name"), "eth0");
}

/*
 * Unit tests for gnmi_decode_json_ietf
 *
 * gnmi_decode_json_ietf has to decode a grpc notification update encoded with
 * JSON_IETF. It is looking for the fields 'byte-count', 'packet-count',
 * 'collection-timestamp.seconds', 'collection-timestamp.nano-seconds' and 'paction'.
 *
 * The function will try to parse the JSON string. If it doesn't succeed an error will be logged and
 * the returned map string will be empty. Else the map string will be populated with the decoded
 * fields.
 *
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
 * We are testing if gnmi_decode_json_ietf correctly parses simple data.
 */
TEST(GnmiJsonTest, DecodeNotificationUpdateJsonIetfFibStats)
{
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);

    gnmi::Update update;
    // Create a dummy JSON update
    std::string json_value = R"({
        "fib-stats": {
            "byte-count": 1000,
            "packet-count": 500,
            "collection-timestamp": {
                "seconds": 1633000000,
                "nano-seconds": 123456789
            }
        }
    })";

    update.mutable_val()->set_json_ietf_val(json_value);

    std::shared_ptr<std::unordered_map<std::string, std::string>> raw_pbr_stat =
        std::make_shared<std::unordered_map<std::string, std::string>>();
    EXPECT_NO_THROW(instance.get_impl()->gnmi_decode_json_ietf(update, raw_pbr_stat));

    // Check that the stats were populated correctly
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/byte-count"], "1000");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/packet-count"], "500");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/seconds"], "1633000000");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/nano-seconds"], "123456789");
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/path-grp-name"],
              "");  // We expect empty for those
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/type"],
              "");  // We expect empty for those
}

/*
 * We are testing if gnmi_decode_json_ietf correctly parses array data.
 */
TEST(GnmiJsonTest, DecodeNotificationUpdateJsonIetfPaction)
{
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);

    gnmi::Update update;
    // Create a dummy JSON update
    std::string json_value = R"({
        "paction": {
            "policy-rule-action": [
            {
                "act-un": {
                    "path-grp-name": "group1",
                    "type": "group2"
                }
            }
            ]
        }
    })";

    update.mutable_val()->set_json_ietf_val(json_value);

    std::shared_ptr<std::unordered_map<std::string, std::string>> raw_pbr_stat =
        std::make_shared<std::unordered_map<std::string, std::string>>();
    EXPECT_NO_THROW(instance.get_impl()->gnmi_decode_json_ietf(update, raw_pbr_stat));

    // Check that the stats were populated correctly
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/byte-count"], "");    // We expect default value for those
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/packet-count"], "");  // We expect default value for those
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/seconds"],
              "");  // We expect default value for those
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/nano-seconds"],
              "");  // We expect default value for those
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/path-grp-name"], "group1");
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/type"], "group2");
}

/*
 * Unit tests for gnmi_decode_proto
 *
 * gnmi_decode_proto has to decode a grpc notification update encoded with PROTO.
 * It is looking for the fields 'byte-count', 'packet-count', 'collection-timestamp.seconds',
 * 'collection-timestamp.nano-seconds' and 'paction'.
 *
 * The function will parse the PROTO object and populate the map string.
 * If the path doesn't contain one of the field or if the value in the fields is of the wrong
 type,
 * it will be discared.
 *
 */

/*
 * We are testing if gnmi_decode_proto correctly parses the input data.
 */
TEST(DecodeProtoTest, MultipleElementsTest)
{
    gnmi::Update update_byte_count;
    gnmi::Update update_packet_count;
    gnmi::Update update_timestamp_seconds;
    gnmi::Update update_timestamp_nanoseconds;
    gnmi::Update update_paction_path_rgp_name;
    gnmi::Update update_paction_type;
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);

    // Set path and value for fib-stats/byte-count
    gnmi::Path* path_byte_count = update_byte_count.mutable_path();
    path_byte_count->add_elem()->set_name("fib-stats");
    path_byte_count->add_elem()->set_name("byte-count");
    update_byte_count.mutable_val()->set_uint_val(1000);

    // Set path and value for fib-stats/packet-count
    gnmi::Path* path_packet_count = update_packet_count.mutable_path();
    path_packet_count->add_elem()->set_name("fib-stats");
    path_packet_count->add_elem()->set_name("packet-count");
    update_packet_count.mutable_val()->set_uint_val(500);

    // Set path and value for fib-stats/collection-timestamp/seconds
    gnmi::Path* path_seconds = update_timestamp_seconds.mutable_path();
    path_seconds->add_elem()->set_name("fib-stats");
    path_seconds->add_elem()->set_name("collection-timestamp");
    path_seconds->add_elem()->set_name("seconds");
    update_timestamp_seconds.mutable_val()->set_int_val(1633000000);

    // Set path and value for fib-stats/collection-timestamp/nano-seconds
    gnmi::Path* path_nanoseconds = update_timestamp_nanoseconds.mutable_path();
    path_nanoseconds->add_elem()->set_name("fib-stats");
    path_nanoseconds->add_elem()->set_name("collection-timestamp");
    path_nanoseconds->add_elem()->set_name("nano-seconds");
    update_timestamp_nanoseconds.mutable_val()->set_int_val(123456789);

    // Set path and value for paction/policy-rule-action/act-un/path-grp-name/
    gnmi::Path* path_rgp_name = update_paction_path_rgp_name.mutable_path();
    path_rgp_name->add_elem()->set_name("paction");
    path_rgp_name->add_elem()->set_name("policy-rule-action");
    path_rgp_name->add_elem()->set_name("act-un");
    path_rgp_name->add_elem()->set_name("path-grp-name");
    update_paction_path_rgp_name.mutable_val()->set_string_val("rgp_name");

    // Set path and value for paction/policy-rule-action/act-un/type/
    gnmi::Path* path_rgp_type = update_paction_type.mutable_path();
    path_rgp_type->add_elem()->set_name("paction");
    path_rgp_type->add_elem()->set_name("policy-rule-action");
    path_rgp_type->add_elem()->set_name("act-un");
    path_rgp_type->add_elem()->set_name("type");
    update_paction_type.mutable_val()->set_string_val("rgp_type");

    // Call the function for each update (simulating multiple notifications)
    std::shared_ptr<std::unordered_map<std::string, std::string>> raw_pbr_stat =
        std::make_shared<std::unordered_map<std::string, std::string>>();
    instance.get_impl()->gnmi_decode_proto(update_byte_count, raw_pbr_stat);
    instance.get_impl()->gnmi_decode_proto(update_packet_count, raw_pbr_stat);
    instance.get_impl()->gnmi_decode_proto(update_timestamp_seconds, raw_pbr_stat);
    instance.get_impl()->gnmi_decode_proto(update_timestamp_nanoseconds, raw_pbr_stat);
    instance.get_impl()->gnmi_decode_proto(update_paction_path_rgp_name, raw_pbr_stat);
    instance.get_impl()->gnmi_decode_proto(update_paction_type, raw_pbr_stat);

    // Verify that all fields were correctly populated
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/byte-count"], "1000");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/packet-count"], "500");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/seconds"], "1633000000");
    EXPECT_EQ((*raw_pbr_stat)["/fib-stats/collection-timestamp/nano-seconds"], "123456789");
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/path-grp-name"], "rgp_name");
    EXPECT_EQ((*raw_pbr_stat)["/paction/policy-rule-action/act-un/type"], "rgp_type");
}

/*
 * Unit tests for gnmi_parse_response
 *
 * gnmi_parse_response decode all updates from a notification and populates a map string object.
 * From a notification, it will check for a prefix path, and notifications to decode.
 * From the prefix the policy_name and rule_name will be deduced.
 *
 * If an error occurs it will be logged and returned. Then the data should not be trusted.
 * Else SUCCESS is returned and the mapped string can be read.
 *
 */

/*
 * We test if gnmi_parse_response is able to parse one update.
 */
TEST(GnmiParseResponseTest, CreateParseResponseValidNotification)
{
    rpc_channel_args channel_args("localhost:50051", false, "", "", "");
    gnmi_client_connection dummyConnection(channel_args);
    auto pbr_counters = std::make_shared<PBRBasic>();
    Derived instance(dummyConnection.get_channel(), pbr_counters);

    gnmi::Notification notification;
    gnmi::Update* update = notification.add_update();
    gnmi::Path* prefix = notification.mutable_prefix();
    prefix->set_origin("Cisco-IOS-XR-pbr-fwd-stats-oper");

    gnmi::Path* update_path = update->mutable_path();
    *update_path = string_to_gnmipath(
        "pbr-stats/policy-maps/policy-map[policy-name=key_policy]/rule-names/"
        "rule-name[rule-name=key_rule]/");

    gnmi::TypedValue* val = update->mutable_val();
    val->set_json_ietf_val(R"({
        "fib-stats": {
            "byte-count": 1000,
            "packet-count": 500
        }
    })");

    auto result =
        instance.get_impl()->gnmi_parse_response(notification, "Cisco-IOS-XR-pbr-fwd-stats-oper");

    EXPECT_EQ(result.second, internal_error_code::SUCCESS);
    EXPECT_EQ((*result.first)["policy_name"], "key_policy");
    EXPECT_EQ((*result.first)["rule_name"], "key_rule");
    EXPECT_EQ((*result.first)["/fib-stats/byte-count"], "1000");
    EXPECT_EQ((*result.first)["/fib-stats/packet-count"], "500");
}
