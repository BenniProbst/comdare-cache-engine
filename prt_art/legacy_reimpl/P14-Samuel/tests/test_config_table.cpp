// SPDX-License-Identifier: Apache-2.0
#include "config_table_bplus_page.hpp"
#include <gtest/gtest.h>

namespace sm = comdare::prt_art::legacy_reimpl::samuel;

TEST(ConfigurationTable, EmptyLookupReturnsNullopt) {
    sm::ConfigurationTable t;
    EXPECT_EQ(t.size(), 0u);
    EXPECT_FALSE(t.lookup({0, 0, sm::UpdateRatioBin::ReadDominant}).has_value());
}

TEST(ConfigurationTable, InsertAndLookup) {
    sm::ConfigurationTable t;
    sm::ConfigRecommendation r{4, 31, true, true};
    t.insert({1, 1, sm::UpdateRatioBin::Mixed}, r);

    auto found = t.lookup({1, 1, sm::UpdateRatioBin::Mixed});
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->recommended_cache_lines_per_node, 4u);
    EXPECT_EQ(found->recommended_fanout, 31u);
    EXPECT_TRUE(found->enable_sibling_clusters);
    EXPECT_TRUE(found->enable_prefetch);
}

TEST(ConfigurationTable, KeyMismatchReturnsNullopt) {
    sm::ConfigurationTable t;
    t.insert({0, 0, sm::UpdateRatioBin::ReadDominant}, {});
    EXPECT_FALSE(t.lookup({2, 2, sm::UpdateRatioBin::WriteHeavy}).has_value());
}

TEST(ConfigKey, EqualityIsExact) {
    sm::ConfigKey a{1, 2, sm::UpdateRatioBin::Mixed};
    sm::ConfigKey b{1, 2, sm::UpdateRatioBin::Mixed};
    sm::ConfigKey c{1, 2, sm::UpdateRatioBin::WriteHeavy};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}
