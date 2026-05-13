// SPDX-License-Identifier: Apache-2.0
#include "multi_level_reloc.hpp"
#include <gtest/gtest.h>

namespace mlr = comdare::prt_art::legacy_reimpl::multi_level_reloc;

TEST(MultiLevelReloc, EmptyAfterConstruction) {
    mlr::MultiLevelRelocation r;
    EXPECT_EQ(r.block_count(), 0u);
    EXPECT_EQ(r.migrations(), 0u);
}

TEST(MultiLevelReloc, AddBlockRejectsInvalidLevel) {
    mlr::MultiLevelRelocation r{4};
    mlr::Block bad{1, 99, 0, 0};
    EXPECT_EQ(r.add_block(bad), 4);
    EXPECT_EQ(r.block_count(), 0u);
}

TEST(MultiLevelReloc, HotBlockMigratesUp) {
    mlr::MultiLevelRelocation r{4};
    r.add_block({1, 3, 1000, 100});   // hot, deep level
    r.reorganize_bfs(/*hot_threshold=*/100);
    EXPECT_EQ(r.blocks()[0].level, 2u);  // promoted one level up
    EXPECT_GE(r.migrations(), 1u);
}

TEST(MultiLevelReloc, ColdBlockMigratesDown) {
    mlr::MultiLevelRelocation r{4};
    r.add_block({1, 0, 0, 50});   // cold, at L1
    r.reorganize_bfs(100);
    EXPECT_EQ(r.blocks()[0].level, 1u);  // demoted
}
