// SPDX-License-Identifier: Apache-2.0
#include "csb_node_group_page.hpp"
#include <gtest/gtest.h>

namespace csb = comdare::prt_art::legacy_reimpl::csb;

TEST(CsbGroupPage, EmptyOnInit) {
    csb::CsbNodeGroupPage<14> g;
    EXPECT_EQ(g.size(), 0u);
}

TEST(CsbGroupPage, InsertSortedAndLookup) {
    csb::CsbNodeGroupPage<14> g;
    EXPECT_EQ(g.insert_separator(50, 500), 0);
    EXPECT_EQ(g.insert_separator(10, 100), 0);
    EXPECT_EQ(g.insert_separator(30, 300), 0);
    EXPECT_EQ(g.separators[0], 10u);
    EXPECT_EQ(g.separators[1], 30u);
    EXPECT_EQ(g.separators[2], 50u);

    auto v = g.lookup(30);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 300u);
}

TEST(CsbGroupPage, DuplicateInsertRejected) {
    csb::CsbNodeGroupPage<14> g;
    EXPECT_EQ(g.insert_separator(1, 11), 0);
    EXPECT_EQ(g.insert_separator(1, 99), 1);
}

TEST(CsbGroupPage, FullCapacity) {
    csb::CsbNodeGroupPage<3> g;
    EXPECT_EQ(g.insert_separator(1, 1), 0);
    EXPECT_EQ(g.insert_separator(2, 2), 0);
    EXPECT_EQ(g.insert_separator(3, 3), 0);
    EXPECT_EQ(g.insert_separator(4, 4), 5);
}
