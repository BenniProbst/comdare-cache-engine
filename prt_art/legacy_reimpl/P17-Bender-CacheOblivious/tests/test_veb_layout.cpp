// SPDX-License-Identifier: Apache-2.0
#include "cache_oblivious_layout.hpp"
#include <gtest/gtest.h>
#include <set>

namespace co = comdare::prt_art::legacy_reimpl::cache_oblivious_layout;

TEST(VebLayout, HeightOneSingleNode) {
    co::VebLayout veb{1};
    EXPECT_EQ(veb.height(), 1u);
    EXPECT_EQ(veb.node_count(), 1u);
    EXPECT_EQ(veb.veb_index(0), 0u);
}

TEST(VebLayout, HeightThreeSevenNodes) {
    co::VebLayout veb{3};
    EXPECT_EQ(veb.node_count(), 7u);
    // Alle 7 Knoten muessen eindeutige Indizes in [0..6] haben
    std::set<std::size_t> indices;
    for (std::size_t i = 0; i < 7; ++i) indices.insert(veb.veb_index(i));
    EXPECT_EQ(indices.size(), 7u);
    EXPECT_EQ(*indices.begin(),  0u);
    EXPECT_EQ(*indices.rbegin(), 6u);
}

TEST(VebLayout, HeightThreeMatchesExpectedVebOrder) {
    // H=3, top_h=2, bot_h=1
    // Erwartete vEB-Indizes:
    //   node 0 -> 0  (root)
    //   node 1 -> 1  (top-left)
    //   node 2 -> 2  (top-right)
    //   node 3 -> 3  (bottom-left-of-1)
    //   node 4 -> 4  (bottom-right-of-1)
    //   node 5 -> 5  (bottom-left-of-2)
    //   node 6 -> 6  (bottom-right-of-2)
    co::VebLayout veb{3};
    EXPECT_EQ(veb.veb_index(0), 0u);
    EXPECT_EQ(veb.veb_index(1), 1u);
    EXPECT_EQ(veb.veb_index(2), 2u);
    EXPECT_EQ(veb.veb_index(3), 3u);
    EXPECT_EQ(veb.veb_index(4), 4u);
    EXPECT_EQ(veb.veb_index(5), 5u);
    EXPECT_EQ(veb.veb_index(6), 6u);
}

TEST(VebLayout, HeightFourDiffersFromBfs) {
    // H=4, top_h=2, bot_h=2: vEB unterscheidet sich von BFS!
    // Top-Subtree (Hoehe 2): node 0,1,2 -> Index 0,1,2  (gleich wie BFS)
    // Bottom-Subtrees (Hoehe 2):
    //   B1 (root=node 3): node 3,7,8 -> Index 3,4,5    (BFS: 3,7,8)
    //   B2 (root=node 4): node 4,9,10 -> Index 6,7,8   (BFS: 4,7,8)
    //   B3 (root=node 5): node 5,11,12 -> Index 9,10,11
    //   B4 (root=node 6): node 6,13,14 -> Index 12,13,14
    // -> node 7 hat vEB-Index 4, BFS-Index ware 7.
    co::VebLayout veb{4};
    EXPECT_EQ(veb.node_count(), 15u);
    // Top-Subtree (BFS-konform)
    EXPECT_EQ(veb.veb_index(0), 0u);
    EXPECT_EQ(veb.veb_index(1), 1u);
    EXPECT_EQ(veb.veb_index(2), 2u);
    // Bottom-Subtree 1: enthaelt node 3 (Wurzel) + node 7,8 (Kinder von 3)
    EXPECT_EQ(veb.veb_index(3), 3u);
    EXPECT_EQ(veb.veb_index(7), 4u);  // <- vEB-spezifisch! BFS waere 7.
    EXPECT_EQ(veb.veb_index(8), 5u);  // <- vEB-spezifisch! BFS waere 8.
    // Bottom-Subtree 2: enthaelt node 4 + node 9,10
    EXPECT_EQ(veb.veb_index(4), 6u);
    EXPECT_EQ(veb.veb_index(9), 7u);
    EXPECT_EQ(veb.veb_index(10), 8u);
    // Bottom-Subtree 3: node 5 + node 11,12
    EXPECT_EQ(veb.veb_index(5), 9u);
    EXPECT_EQ(veb.veb_index(11), 10u);
    EXPECT_EQ(veb.veb_index(12), 11u);
    // Bottom-Subtree 4: node 6 + node 13,14
    EXPECT_EQ(veb.veb_index(6), 12u);
    EXPECT_EQ(veb.veb_index(13), 13u);
    EXPECT_EQ(veb.veb_index(14), 14u);
}

TEST(VebLayout, HeightFourAllIndicesUnique) {
    co::VebLayout veb{4};
    std::set<std::size_t> indices;
    for (std::size_t i = 0; i < 15; ++i) indices.insert(veb.veb_index(i));
    EXPECT_EQ(indices.size(), 15u);
    EXPECT_EQ(*indices.begin(),  0u);
    EXPECT_EQ(*indices.rbegin(), 14u);
}

TEST(VebLayout, RootIsAtIndexZero) {
    co::VebLayout veb{5};
    EXPECT_EQ(veb.veb_index(0), 0u);
}

TEST(VebLayout, HeightFiveAllIndicesUnique) {
    co::VebLayout veb{5};
    EXPECT_EQ(veb.node_count(), 31u);
    std::set<std::size_t> indices;
    for (std::size_t i = 0; i < 31; ++i) indices.insert(veb.veb_index(i));
    EXPECT_EQ(indices.size(), 31u);
    EXPECT_EQ(*indices.begin(),  0u);
    EXPECT_EQ(*indices.rbegin(), 30u);
}
