// SPDX-License-Identifier: Apache-2.0
// Tests fuer comdare::succinct (Aufgabe #105)

#include <comdare/succinct/bit_vector.hpp>
#include <comdare/succinct/louds.hpp>

#include <gtest/gtest.h>

namespace su = comdare::succinct;

// ─────────────────────────────────────────────────────────────────────────────
// BitVector
// ─────────────────────────────────────────────────────────────────────────────

TEST(BitVector, EmptyOnInit) {
    su::BitVector b;
    EXPECT_EQ(b.size(), 0u);
    EXPECT_TRUE(b.empty());
}

TEST(BitVector, ResizeAndSet) {
    su::BitVector b{128};
    EXPECT_EQ(b.size(), 128u);
    EXPECT_FALSE(b.get(5));
    b.set(5, true);
    EXPECT_TRUE(b.get(5));
    b.set(5, false);
    EXPECT_FALSE(b.get(5));
}

TEST(BitVector, SetOutOfRangeReturnsError) {
    su::BitVector b{8};
    EXPECT_EQ(b.set(100, true), 7); // status_out_of_range
}

TEST(BitVector, Rank1CountsCorrectly) {
    su::BitVector b{16};
    b.set(0, true);
    b.set(3, true);
    b.set(7, true);
    b.set(15, true);
    b.build_rank_index();
    EXPECT_EQ(b.rank1(0), 0u); // Anzahl gesetzter in [0,0) = 0
    EXPECT_EQ(b.rank1(1), 1u);
    EXPECT_EQ(b.rank1(4), 2u);
    EXPECT_EQ(b.rank1(8), 3u);
    EXPECT_EQ(b.rank1(16), 4u);
}

TEST(BitVector, Rank0IsComplement) {
    su::BitVector b{10};
    for (std::size_t i = 0; i < 10; ++i) b.set(i, (i % 2 == 0));
    b.build_rank_index();
    EXPECT_EQ(b.rank1(10) + b.rank0(10), 10u);
}

TEST(BitVector, TotalOnes) {
    su::BitVector b{256};
    for (std::size_t i = 0; i < 256; ++i) b.set(i, (i % 3 == 0));
    b.build_rank_index();
    EXPECT_EQ(b.total_ones(), 86u); // 256/3 = 85 + 1 (i=0)
}

TEST(BitVector, Select1FindsKthOne) {
    su::BitVector b{32};
    b.set(2, true);
    b.set(7, true);
    b.set(15, true);
    b.set(31, true);
    b.build_rank_index();
    EXPECT_EQ(b.select1(1), 2u);
    EXPECT_EQ(b.select1(2), 7u);
    EXPECT_EQ(b.select1(3), 15u);
    EXPECT_EQ(b.select1(4), 31u);
}

TEST(BitVector, Select1OutOfRangeReturnsSize) {
    su::BitVector b{8};
    b.set(0, true);
    b.build_rank_index();
    EXPECT_EQ(b.select1(5), b.size());
}

TEST(BitVector, MultiBlockRank) {
    // 1024 Bits = 4 Bloecke (Block = 256 Bits)
    su::BitVector b{1024};
    for (std::size_t i = 0; i < 1024; ++i) b.set(i, (i % 7 == 0));
    b.build_rank_index();
    EXPECT_EQ(b.rank1(1024), 147u); // ceil(1024/7) = 147
    EXPECT_EQ(b.rank1(7), 1u);      // nur i=0
    EXPECT_EQ(b.rank1(8), 2u);      // i=0, i=7
}

TEST(BitVector, ClearResetsAllBits) {
    su::BitVector b{64};
    for (std::size_t i = 0; i < 64; ++i) b.set(i, true);
    b.build_rank_index();
    EXPECT_EQ(b.total_ones(), 64u);
    b.clear();
    b.build_rank_index();
    EXPECT_EQ(b.total_ones(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// LOUDS
// ─────────────────────────────────────────────────────────────────────────────

TEST(Louds, EmptyOnInit) {
    su::Louds l;
    EXPECT_EQ(l.node_count(), 0u);
}

TEST(Louds, BuildFromBfsDegreesSetsNodeCount) {
    // Baum:    A (2 Kinder)
    //         / \
    //        B   C  (0,0)
    su::Louds                  l;
    std::vector<std::uint32_t> degrees{2, 0, 0};
    l.build_from_bfs_degrees(degrees);
    EXPECT_EQ(l.node_count(), 3u);
}

TEST(Louds, DegreeMatchesInput) {
    // Baum:       A (2 Kinder)
    //            / \
    //           B   C
    //          / \
    //         D   E   (B hat 2 Kinder, C/D/E je 0)
    su::Louds                  l;
    std::vector<std::uint32_t> degrees{2, 2, 0, 0, 0};
    l.build_from_bfs_degrees(degrees);
    EXPECT_EQ(l.degree(0), 2u);
    EXPECT_EQ(l.degree(1), 2u);
    EXPECT_EQ(l.degree(2), 0u);
    EXPECT_EQ(l.degree(3), 0u);
    EXPECT_EQ(l.degree(4), 0u);
}

TEST(Louds, BitsStructureForSimpleTree) {
    // Erwartetes LOUDS-Encoding fuer 5-Knoten-Baum mit degrees {2,2,0,0,0}:
    //  N0=A (d=2):  "1 1 0"
    //  N1=B (d=2):  "1 1 0"
    //  N2=C (d=0):  "0"
    //  N3=D (d=0):  "0"
    //  N4=E (d=0):  "0"
    //  Total: 11011000 0 0 = "110110000"
    su::Louds l;
    l.build_from_bfs_degrees({2, 2, 0, 0, 0});
    auto const& b = l.bits();
    EXPECT_EQ(b.size(), 9u); // 2 + 1 + 2 + 1 + 1 + 1 + 1 + 1 = 9
    EXPECT_TRUE(b.get(0));
    EXPECT_TRUE(b.get(1));
    EXPECT_FALSE(b.get(2));
    EXPECT_TRUE(b.get(3));
    EXPECT_TRUE(b.get(4));
    EXPECT_FALSE(b.get(5));
}

TEST(Louds, LeafNodeHasNoFirstChild) {
    su::Louds l;
    l.build_from_bfs_degrees({2, 0, 0});
    EXPECT_EQ(l.first_child(1), 0u); // B ist leaf
    EXPECT_EQ(l.first_child(2), 0u); // C ist leaf
}
