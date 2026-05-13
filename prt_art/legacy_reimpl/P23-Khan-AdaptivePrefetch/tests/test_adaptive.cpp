// SPDX-License-Identifier: Apache-2.0
#include "adaptive_prefetch_distance.hpp"
#include <gtest/gtest.h>

namespace ap = comdare::prt_art::legacy_reimpl::adaptive_prefetch;

TEST(AdaptiveDistance, InitialDistance) {
    ap::AdaptivePrefetchDistance d{4};
    EXPECT_EQ(d.current_distance(), 4u);
}

TEST(AdaptiveDistance, ManyMissesIncreaseDistance) {
    ap::AdaptivePrefetchDistance d{4};
    for (int i = 0; i < 100; ++i) d.record_miss();
    EXPECT_EQ(d.current_distance(), 5u);
}

TEST(AdaptiveDistance, ManyHitsDecreaseDistance) {
    ap::AdaptivePrefetchDistance d{10};
    for (int i = 0; i < 100; ++i) d.record_hit();
    EXPECT_EQ(d.current_distance(), 9u);
}

TEST(AdaptiveDistance, DistanceClampedAtBounds) {
    ap::AdaptivePrefetchDistance d{1};
    for (int i = 0; i < 100; ++i) d.record_hit();
    EXPECT_EQ(d.current_distance(), 1u);

    ap::AdaptivePrefetchDistance d2{32};
    for (int i = 0; i < 100; ++i) d2.record_miss();
    EXPECT_EQ(d2.current_distance(), 32u);
}
