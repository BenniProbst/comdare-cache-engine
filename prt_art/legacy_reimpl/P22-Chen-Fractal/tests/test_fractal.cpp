// SPDX-License-Identifier: Apache-2.0
#include "fractal_prefetch_bplus.hpp"
#include <gtest/gtest.h>

namespace fr = comdare::prt_art::legacy_reimpl::fractal_bplus;

TEST(DiskPage, InnerNodeFitsInCacheLine) {
    EXPECT_LE(sizeof(fr::InnerCacheNode), fr::kCacheLineBytes);
}

TEST(DiskPage, CapacityMatchesPageSize) {
    EXPECT_EQ(fr::DiskPageWithInnerBPlus::capacity(), 64u);  // 4096/64
}

TEST(DiskPage, AllocateInnerNodesUntilFull) {
    fr::DiskPageWithInnerBPlus page;
    EXPECT_EQ(page.inner_count(), 0u);
    int first = page.allocate_inner_node();
    EXPECT_EQ(first, 0);
    EXPECT_EQ(page.inner_count(), 1u);

    for (std::size_t i = 1; i < fr::kInnerNodesPerPage; ++i) {
        EXPECT_GE(page.allocate_inner_node(), 0);
    }
    EXPECT_EQ(page.allocate_inner_node(), 5);  // capacity_exceeded
}

TEST(DiskPage, FractalLookupOnEmpty) {
    fr::DiskPageWithInnerBPlus page;
    EXPECT_EQ(page.lookup_fractal(42), 0u);
}
