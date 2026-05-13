// SPDX-License-Identifier: Apache-2.0
#include "probability_layout.hpp"
#include <gtest/gtest.h>

namespace pl = comdare::prt_art::legacy_reimpl::probability_layout;

TEST(ProbabilityLayout, EmptyOnInit) {
    pl::ProbabilityLayout layout{4};
    EXPECT_EQ(layout.node_count(), 0u);
    EXPECT_EQ(layout.block_capacity(), 4u);
    EXPECT_TRUE(layout.compute_layout().empty());
}

TEST(ProbabilityLayout, GreedyPacksByProbability) {
    pl::ProbabilityLayout layout{2};
    layout.add_node({1, 0.10, 0});
    layout.add_node({2, 0.50, 0});
    layout.add_node({3, 0.30, 0});
    layout.add_node({4, 0.05, 0});

    auto blocks = layout.compute_layout();
    EXPECT_EQ(blocks.size(), 2u);
    // Erster Block: hoechste Wahrscheinlichkeiten (0.50 + 0.30)
    EXPECT_EQ(blocks[0].node_ids[0], 2u);
    EXPECT_EQ(blocks[0].node_ids[1], 3u);
    EXPECT_DOUBLE_EQ(blocks[0].density, 0.80);
    // Zweiter Block
    EXPECT_EQ(blocks[1].node_ids[0], 1u);
    EXPECT_EQ(blocks[1].node_ids[1], 4u);
}

TEST(ProbabilityLayout, BlockCapacityOne) {
    pl::ProbabilityLayout layout{1};
    layout.add_node({1, 0.5, 0});
    layout.add_node({2, 0.3, 0});
    auto blocks = layout.compute_layout();
    EXPECT_EQ(blocks.size(), 2u);
}
