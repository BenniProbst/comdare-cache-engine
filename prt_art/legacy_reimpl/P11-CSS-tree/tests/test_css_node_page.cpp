// SPDX-License-Identifier: Apache-2.0
#include "css_node_page.hpp"
#include <gtest/gtest.h>

namespace css = comdare::prt_art::legacy_reimpl::css_node_page;

TEST(CssNodePage, FitsInCacheLine) {
    EXPECT_LE(sizeof(css::CssNodePage<7>), css::kCacheLineBytes);
}

TEST(CssNodePage, InsertSortedAndLookup) {
    css::CssNodePage<7> node;
    EXPECT_EQ(node.key_count(), 0u);
    EXPECT_EQ(node.insert_sorted(50, 500), 0);
    EXPECT_EQ(node.insert_sorted(20, 200), 0);
    EXPECT_EQ(node.insert_sorted(80, 800), 0);

    EXPECT_EQ(node.keys[0], 20u);
    EXPECT_EQ(node.keys[1], 50u);
    EXPECT_EQ(node.keys[2], 80u);

    auto v = node.lookup(50);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 500u);
}

TEST(CssNodePage, DuplicateInsertRejected) {
    css::CssNodePage<7> node;
    EXPECT_EQ(node.insert_sorted(1, 11), 0);
    EXPECT_EQ(node.insert_sorted(1, 99), 1);  // already exists
    auto v = node.lookup(1);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 11u);
}

TEST(CssNodePage, FullRejectsInsert) {
    css::CssNodePage<3> small;
    EXPECT_EQ(small.insert_sorted(1, 1), 0);
    EXPECT_EQ(small.insert_sorted(2, 2), 0);
    EXPECT_EQ(small.insert_sorted(3, 3), 0);
    EXPECT_TRUE(small.full());
    EXPECT_EQ(small.insert_sorted(4, 4), 5);  // capacity_exceeded
}

TEST(CssTree, IndexArithmeticConsistent) {
    using Tree = css::CssTree<7>;
    EXPECT_EQ(Tree::child_index(0, 0), 1u);
    EXPECT_EQ(Tree::child_index(0, 7), 8u);
    EXPECT_EQ(Tree::child_index(1, 0), 9u);

    EXPECT_EQ(Tree::parent_index(1), 0u);
    EXPECT_EQ(Tree::parent_index(8), 0u);
    EXPECT_EQ(Tree::parent_index(9), 1u);
}

TEST(CssTree, AddRootOnce) {
    css::CssTree<7> tree;
    EXPECT_EQ(tree.node_count(), 0u);
    EXPECT_EQ(tree.add_root(), 0);
    EXPECT_EQ(tree.node_count(), 1u);
    EXPECT_EQ(tree.add_root(), 1);  // already exists
}
