// SPDX-License-Identifier: Apache-2.0
#include "layout_invariant.hpp"
#include <gtest/gtest.h>

namespace li = comdare::prt_art::legacy_reimpl::layout_invariant;

TEST(LayoutInvariantBuffer, EmptyOnInit) {
    li::LayoutInvariantBuffer<64> b;
    EXPECT_EQ(b.size(), 0u);
    EXPECT_TRUE(b.check_invariant());
}

TEST(LayoutInvariantBuffer, InsertAndAt) {
    li::LayoutInvariantBuffer<64> b;
    EXPECT_EQ(b.insert(100), 0);
    EXPECT_EQ(b.insert(200), 0);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_TRUE(b.check_invariant());

    auto v0 = b.at(0);
    ASSERT_TRUE(v0.has_value());
    EXPECT_EQ(*v0, 100u);
}

TEST(LayoutInvariantBuffer, EraseMaintainsInvariant) {
    li::LayoutInvariantBuffer<16> b;
    b.insert(11);
    b.insert(22);
    b.insert(33);
    EXPECT_EQ(b.erase_at(1), 0);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_TRUE(b.check_invariant());

    EXPECT_FALSE(b.at(1).has_value());
    EXPECT_EQ(*b.at(0), 11u);
    EXPECT_EQ(*b.at(2), 33u);
}

TEST(LayoutInvariantBuffer, EraseNonExistentReturnsNotFound) {
    li::LayoutInvariantBuffer<16> b;
    EXPECT_EQ(b.erase_at(5), 2);
}

TEST(LayoutInvariantBuffer, OutOfRangeReturnsError) {
    li::LayoutInvariantBuffer<16> b;
    EXPECT_EQ(b.erase_at(100), 7);
    EXPECT_FALSE(b.at(100).has_value());
}

TEST(LayoutInvariantBuffer, CapacityRejectsOverflow) {
    li::LayoutInvariantBuffer<4> b;
    EXPECT_EQ(b.insert(1), 0);
    EXPECT_EQ(b.insert(2), 0);
    EXPECT_EQ(b.insert(3), 0);
    EXPECT_EQ(b.insert(4), 0);
    EXPECT_EQ(b.insert(5), 5);  // capacity_exceeded
}
