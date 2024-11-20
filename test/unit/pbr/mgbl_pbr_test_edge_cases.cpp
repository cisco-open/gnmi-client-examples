#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include "gnmi/mgbl_gnmi_client.h"
#include "mgbl_api.h"
#include "nlohmann/json.hpp"
#include "pbr/mgbl_pbr.h"

using json = nlohmann::json;
using namespace mgbl_api;

/*
 * Edge cases unit tests for map_to_stats
 *
 * See mgbl_pbr_test.cpp
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
 * We are testing if map_to_stats correctly handles a missing key by returning an empty field.
 */
TEST(MapToPbrStatsTest, MissingKey)
{
    auto pbr_counters = std::make_shared<PBRBasic>();

    std::unordered_map<std::string, std::string> input_map = {
        {"policy_name", "test_policy"},
        {"rule_name", "test_rule"},
        // Missing "fib-stats.byte-count"
        {"/fib-stats/packet-count", "67890"},
        {"/fib-stats/collection-timestamp/seconds", "161718"},
        {"/fib-stats/collection-timestamp/nano-seconds", "192021"},
        {"/paction/policy-rule-action/act-un/path-grp-name", "test_path_grp"},
        {"/paction/policy-rule-action/act-un/type", "test_type"}};

    auto result = pbr_counters->unordered_map_to_stats(input_map);
    auto pbr_basic_stat = std::dynamic_pointer_cast<PbrBasicStat>(result);

    // Check that the fields were populated correctly
    EXPECT_EQ(pbr_basic_stat->policy_name, "test_policy");
    EXPECT_EQ(pbr_basic_stat->rule_name, "test_rule");
    EXPECT_EQ(pbr_basic_stat->byte_count, 0);  // Default value for missing key
    EXPECT_EQ(pbr_basic_stat->packet_count, 67890);
    EXPECT_EQ(pbr_basic_stat->collection_timestamp_seconds, 161718);
    EXPECT_EQ(pbr_basic_stat->collection_timestamp_nanoseconds, 192021);
    EXPECT_EQ(pbr_basic_stat->path_grp_name, "test_path_grp");
    EXPECT_EQ(pbr_basic_stat->policy_action_type, "test_type");
}
/*
 * We test if get_gnmi_path returns an empty list of strings if member keys is empty.
 */
TEST(GnmiPathTest, GnmiPathNoKeys)
{
    PBRBasic instance;

    // No keys

    std::vector<std::string> result = instance.get_gnmi_paths();

    // Check that the fields were populated correctly
    EXPECT_EQ(result.empty(), true);
}

/*
 * Edge cases unit tests for stream_args_pbr
 *
 * See mgbl_pbr_test.cpp
 */

/*
 * We test if stream_args_pbr returns an incomplete string if the key is partial.
 */
TEST(GnmiStreamArgsTest, GnmiPathPartialKey)
{
    PBRBasic instance;
    instance.keys.push_back({"key_policy", ""});  // Partial key

    std::vector<std::string> result = instance.get_gnmi_paths();

    // Check that the fields were populated correctly
    EXPECT_EQ(result[0],
              "Cisco-IOS-XR-pbr-fwd-stats-oper:pbr-stats/policy-maps/"
              "policy-map[policy-name=key_policy]/rule-names/rule-name[rule-name=]/");
}

/*
 * Edge cases unit tests for addStats
 *
 * See mgbl_pbr_test.cpp
 */

/*
 * We are testing if add_stats correctly handles a pbr_stats object with only some fields populated.
 */
TEST(AddStatsTest, AddPartialStats)
{
    PBRBasic instance;
    std::shared_ptr<PbrBasicStat> stat = std::make_shared<PbrBasicStat>();

    stat->policy_name = "partial_policy";
    stat->byte_count = 12345;

    instance.add_stats(stat);

    // Check that the fields were populated correctly
    EXPECT_EQ(instance.stats.size(), 1);
    EXPECT_EQ(instance.stats[0].policy_name, "partial_policy");
    EXPECT_EQ(instance.stats[0].rule_name, "");
    EXPECT_EQ(instance.stats[0].byte_count, 12345);
    EXPECT_EQ(instance.stats[0].packet_count, 0);
    EXPECT_EQ(instance.stats[0].collection_timestamp_seconds, 0);
    EXPECT_EQ(instance.stats[0].collection_timestamp_nanoseconds, 0);
    EXPECT_EQ(instance.stats[0].path_grp_name, "");
    EXPECT_EQ(instance.stats[0].policy_action_type, "");
}