// SPDX-License-Identifier: Apache-2.0
#include "wider_bplus_page.hpp"
#include <gtest/gtest.h>

namespace hk = comdare::prt_art::legacy_reimpl::hankins;

TEST(WiderBPlus, FanoutScalesWithCacheLines) {
    EXPECT_LT(hk::WiderBPlusPage<2>::fanout(), hk::WiderBPlusPage<8>::fanout());
    EXPECT_LT(hk::WiderBPlusPage<8>::fanout(), hk::WiderBPlusPage<16>::fanout());
}

TEST(WiderBPlus, InsertAndLookup) {
    hk::WiderBPlusPage<4> node;
    EXPECT_EQ(node.insert(50, 500), 0);
    EXPECT_EQ(node.insert(20, 200), 0);
    EXPECT_EQ(node.insert(80, 800), 0);

    auto v = node.lookup(20);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 200u);
}

TEST(CostModel, TotalWeighted) {
    hk::CostModel m;
    double t = m.total(100, 10, 5, 2);
    EXPECT_DOUBLE_EQ(t, 100*1.0 + 10*4.0 + 5*0.5 + 2*2.0);
}

TEST(CostModel, CustomWeights) {
    hk::CostModel m{2.0, 8.0, 1.0, 4.0};
    EXPECT_DOUBLE_EQ(m.total(1, 1, 1, 1), 2.0 + 8.0 + 1.0 + 4.0);
}
