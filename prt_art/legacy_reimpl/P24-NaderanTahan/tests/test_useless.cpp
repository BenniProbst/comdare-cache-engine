// SPDX-License-Identifier: Apache-2.0
#include "useless_prefetch_study.hpp"
#include <gtest/gtest.h>

namespace up = comdare::prt_art::legacy_reimpl::useless_prefetch;

TEST(UselessPrefetch, EmptyMetrics) {
    up::UselessPrefetchTracker t;
    EXPECT_EQ(t.metrics().total_prefetches, 0u);
    EXPECT_DOUBLE_EQ(t.metrics().usefulness_ratio(), 0.0);
}

TEST(UselessPrefetch, UsefulCountsAsUseful) {
    up::UselessPrefetchTracker t;
    t.prefetch(0x1000);
    t.use(0x1000);
    EXPECT_EQ(t.metrics().useful_prefetches, 1u);
    EXPECT_EQ(t.metrics().useless_prefetches, 0u);
    EXPECT_DOUBLE_EQ(t.metrics().usefulness_ratio(), 1.0);
}

TEST(UselessPrefetch, EvictedWithoutUseCountsAsWaste) {
    up::UselessPrefetchTracker t;
    t.prefetch(0x2000);
    t.evict(0x2000);
    EXPECT_EQ(t.metrics().useful_prefetches, 0u);
    EXPECT_EQ(t.metrics().useless_prefetches, 1u);
    EXPECT_DOUBLE_EQ(t.metrics().waste_ratio(), 1.0);
}

TEST(UselessPrefetch, UseAfterEvictCountsAsLate) {
    up::UselessPrefetchTracker t;
    t.prefetch(0x3000);
    t.evict(0x3000);
    t.use(0x3000);
    EXPECT_EQ(t.metrics().late_prefetches, 1u);
}

TEST(UselessPrefetch, MixedRatios) {
    up::UselessPrefetchTracker t;
    for (int i = 0; i < 10; ++i) { t.prefetch(0x1000 + i); t.use(0x1000 + i); }
    for (int i = 0; i < 5; ++i)  { t.prefetch(0x2000 + i); t.evict(0x2000 + i); }
    EXPECT_EQ(t.metrics().total_prefetches, 15u);
    EXPECT_DOUBLE_EQ(t.metrics().usefulness_ratio(), 10.0 / 15.0);
}
