// SPDX-License-Identifier: Apache-2.0
#include "prefetch_bplus.hpp"
#include <gtest/gtest.h>

namespace pf = comdare::prt_art::legacy_reimpl::prefetch_bplus;

TEST(PrefetchTracker, EmitAndCount) {
    pf::PrefetchTracker t;
    t.emit(0x1000, 3, false);
    t.emit(0x2000, 2, true);
    EXPECT_EQ(t.hint_count(), 2u);
    EXPECT_EQ(t.hints()[0].address, 0x1000u);
    EXPECT_TRUE(t.hints()[1].is_write);
}

TEST(PrefetchTracker, ResetClears) {
    pf::PrefetchTracker t;
    t.emit(0x1000);
    t.reset();
    EXPECT_EQ(t.hint_count(), 0u);
}

TEST(WiderNode, ScanEmitsPrefetchHints) {
    pf::WiderNodeWithPrefetch<4> node;
    for (std::uint32_t i = 0; i < 32; ++i) {
        node.insert(i, i * 10);
    }
    pf::PrefetchTracker t;
    auto v = node.scan_with_prefetch(20, t);
    EXPECT_EQ(v, 200u);
    EXPECT_GT(t.hint_count(), 0u);
}

TEST(WiderNode, InsertAndScanCorrectness) {
    pf::WiderNodeWithPrefetch<2> node;
    EXPECT_EQ(node.insert(5, 50), 0);
    EXPECT_EQ(node.insert(3, 30), 0);
    EXPECT_EQ(node.insert(7, 70), 0);
    pf::PrefetchTracker t;
    EXPECT_EQ(node.scan_with_prefetch(3, t), 30u);
    EXPECT_EQ(node.scan_with_prefetch(7, t), 70u);
}
