#include "pbr/mgbl_pbr.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "gnmi/mgbl_gnmi_client.h"

using namespace mgbl_api;

/*
 * Unit tests for map_to_stats
 *
 * map_to_stats should correctly convert a map of strings to a pbr_stats object.
 * If a key is missing, the corresponding field should be set to a default value.
 *
 */

/*
 * We are testing if map_to_stats correctly converts a valid map to a pbr_stats object.
 */
TEST(MapToPbrStatsTest, ValidMap)
{
    auto pbr_counters = std::make_shared<PBRBasic>();

    std::unordered_map<std::string, std::string> input_map = {
        {"policy_name", "test_policy"},
        {"rule_name", "test_rule"},
        {"/fib-stats/byte-count", "12345"},
        {"/fib-stats/packet-count", "67890"},
        {"/fib-stats/collection-timestamp/seconds", "161718"},
        {"/fib-stats/collection-timestamp/nano-seconds", "192021"},
        {"/paction/policy-rule-action/act-un/path-grp-name", "test_path_grp"},
        {"/paction/policy-rule-action/act-un/type", "test_type"}};
    // TODO bug: json.flatten() returns /paction/policy-rule-action/act-un/type because
    // policy-rule-action is an array.

    auto result = pbr_counters->unordered_map_to_stats(input_map);
    auto pbr_basic_stat = std::dynamic_pointer_cast<PbrBasicStat>(result);

    // Check that the fields were populated correctly
    EXPECT_EQ(pbr_basic_stat->policy_name, "test_policy");
    EXPECT_EQ(pbr_basic_stat->rule_name, "test_rule");
    EXPECT_EQ(pbr_basic_stat->byte_count, 12345);
    EXPECT_EQ(pbr_basic_stat->packet_count, 67890);
    EXPECT_EQ(pbr_basic_stat->collection_timestamp_seconds, 161718);
    EXPECT_EQ(pbr_basic_stat->collection_timestamp_nanoseconds, 192021);
    EXPECT_EQ(pbr_basic_stat->path_grp_name, "test_path_grp");
    EXPECT_EQ(pbr_basic_stat->policy_action_type, "test_type");
}

/*
 * Unit tests for get_gnmi_path
 *
 * get_gnmi_path should return a list of paths corresponding to a set of policy and rules from
 * member keys. If the member attribute keys is empty then the path should be empty.
 *
 */

/*
 * We are testing if get_gnmi_path generate a valid string for one key.
 */
TEST(GnmiPathTest, GnmiPathSingleKey)
{
    PBRBasic instance;
    instance.keys.push_back({"key_policy", "key_rule"});

    std::vector<std::string> result = instance.get_gnmi_paths();

    // Check that the fields were populated correctly
    EXPECT_EQ(result[0],
              "Cisco-IOS-XR-pbr-fwd-stats-oper:pbr-stats/policy-maps/"
              "policy-map[policy-name=key_policy]/rule-names/rule-name[rule-name=key_rule]/");
}

/*
 * We are testing if get_gnmi_path generate a valid set of strings for multiple keys.
 */
TEST(GnmiPathTest, GnmiPathMultipleKey)
{
    PBRBasic instance;
    instance.keys.push_back({"key_policy1", "key_rule1"});
    instance.keys.push_back({"key_policy2", "key_rule2"});
    instance.keys.push_back({"key_policy3", "key_rule3"});

    std::vector<std::string> result = instance.get_gnmi_paths();

    // Check that the fields were populated correctly
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0],
              "Cisco-IOS-XR-pbr-fwd-stats-oper:pbr-stats/policy-maps/"
              "policy-map[policy-name=key_policy1]/rule-names/rule-name[rule-name=key_rule1]/");
    EXPECT_EQ(result[1],
              "Cisco-IOS-XR-pbr-fwd-stats-oper:pbr-stats/policy-maps/"
              "policy-map[policy-name=key_policy2]/rule-names/rule-name[rule-name=key_rule2]/");
    EXPECT_EQ(result[2],
              "Cisco-IOS-XR-pbr-fwd-stats-oper:pbr-stats/policy-maps/"
              "policy-map[policy-name=key_policy3]/rule-names/rule-name[rule-name=key_rule3]/");
}

/*
 * Unit tests for addStats
 *
 * addStats should add a pbr_stats object to the stats vector.
 * If the pbr_stats object is empty, the fields should be set to default values.
 *
 */

/*
 * We are testing if add_stats correctly adds a pbr_stats object to the stats vector.
 */
TEST(AddStatsTest, AddStats)
{
    PBRBasic instance;
    std::shared_ptr<PbrBasicStat> stat = std::make_shared<PbrBasicStat>();
    stat->policy_name = "test_policy";
    stat->rule_name = "test_rule";
    stat->byte_count = 12345;
    stat->packet_count = 67890;
    stat->collection_timestamp_seconds = 161718;
    stat->collection_timestamp_nanoseconds = 192021;
    stat->path_grp_name = "test_path_grp";
    stat->policy_action_type = "test_type";

    instance.add_stats(stat);

    // Check that the fields were populated correctly
    EXPECT_EQ(instance.stats.size(), 1);
    EXPECT_EQ(instance.stats[0].policy_name, "test_policy");
    EXPECT_EQ(instance.stats[0].rule_name, "test_rule");
    EXPECT_EQ(instance.stats[0].byte_count, 12345);
    EXPECT_EQ(instance.stats[0].packet_count, 67890);
    EXPECT_EQ(instance.stats[0].collection_timestamp_seconds, 161718);
    EXPECT_EQ(instance.stats[0].collection_timestamp_nanoseconds, 192021);
    EXPECT_EQ(instance.stats[0].path_grp_name, "test_path_grp");
    EXPECT_EQ(instance.stats[0].policy_action_type, "test_type");
}

/*
 * We are testing if add_stats correctly adds multiple pbr_stats objects to the stats vector.
 */
TEST(AddStatsTest, AddMultipleStats)
{
    PBRBasic instance;
    std::shared_ptr<PbrBasicStat> stat1 = std::make_shared<PbrBasicStat>();
    stat1->policy_name = "test_policy1";
    stat1->rule_name = "test_rule1";
    stat1->byte_count = 12345;
    stat1->packet_count = 67890;
    stat1->collection_timestamp_seconds = 161718;
    stat1->collection_timestamp_nanoseconds = 192021;
    stat1->path_grp_name = "test_path_grp1";
    stat1->policy_action_type = "test_type1";

    std::shared_ptr<PbrBasicStat> stat2 = std::make_shared<PbrBasicStat>();
    stat2->policy_name = "test_policy2";
    stat2->rule_name = "test_rule2";
    stat2->byte_count = 54321;
    stat2->packet_count = 9876;
    stat2->collection_timestamp_seconds = 181716;
    stat2->collection_timestamp_nanoseconds = 210192;
    stat2->path_grp_name = "test_path_grp2";
    stat2->policy_action_type = "test_type2";

    instance.add_stats(stat1);
    instance.add_stats(stat2);

    // Check that the fields were populated correctly
    EXPECT_EQ(instance.stats.size(), 2);
    EXPECT_EQ(instance.stats[0].policy_name, "test_policy1");
    EXPECT_EQ(instance.stats[0].rule_name, "test_rule1");
    EXPECT_EQ(instance.stats[0].byte_count, 12345);
    EXPECT_EQ(instance.stats[0].packet_count, 67890);
    EXPECT_EQ(instance.stats[0].collection_timestamp_seconds, 161718);
    EXPECT_EQ(instance.stats[0].collection_timestamp_nanoseconds, 192021);
    EXPECT_EQ(instance.stats[0].path_grp_name, "test_path_grp1");
    EXPECT_EQ(instance.stats[0].policy_action_type, "test_type1");

    EXPECT_EQ(instance.stats[1].policy_name, "test_policy2");
    EXPECT_EQ(instance.stats[1].rule_name, "test_rule2");
    EXPECT_EQ(instance.stats[1].byte_count, 54321);
    EXPECT_EQ(instance.stats[1].packet_count, 9876);
    EXPECT_EQ(instance.stats[1].collection_timestamp_seconds, 181716);
    EXPECT_EQ(instance.stats[1].collection_timestamp_nanoseconds, 210192);
    EXPECT_EQ(instance.stats[1].path_grp_name, "test_path_grp2");
    EXPECT_EQ(instance.stats[1].policy_action_type, "test_type2");
}

/*
 * We are testing if add_stats correctly handles an empty pbr_stats object.
 */
TEST(AddStatsTest, AddEmptyStats)
{
    PBRBasic instance;
    std::shared_ptr<PbrBasicStat> stat = std::make_shared<PbrBasicStat>();

    instance.add_stats(stat);

    // Check that the fields were populated correctly
    EXPECT_EQ(instance.stats.size(), 1);
    EXPECT_EQ(instance.stats[0].policy_name, "");
    EXPECT_EQ(instance.stats[0].rule_name, "");
    EXPECT_EQ(instance.stats[0].byte_count, 0);
    EXPECT_EQ(instance.stats[0].packet_count, 0);
    EXPECT_EQ(instance.stats[0].collection_timestamp_seconds, 0);
    EXPECT_EQ(instance.stats[0].collection_timestamp_nanoseconds, 0);
    EXPECT_EQ(instance.stats[0].path_grp_name, "");
    EXPECT_EQ(instance.stats[0].policy_action_type, "");
}