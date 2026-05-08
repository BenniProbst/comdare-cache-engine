#include <gtest/gtest.h>
#include <cstring>
#include <fstream>
#include "tree.hpp"

#define trap(expr) if (!!(expr)) raise(SIGTRAP);

using namespace BTree;
#if 0
//---------------------------------------------------------------------------
TEST(TreeTest, insert)
/// test insert()
{
    Tree<uint32_t, uint32_t> tree;
    size_t n = 5e6;
    std::vector<uint32_t> values(n);
    for (uint32_t i = 0; i < n; ++i) {
        values[i] = i;
    }
    std::random_shuffle(values.begin(), values.end());
    for (uint32_t i : values) {
        tree.insert(i, i);
    }
    bool success = tree.validateInvariant();
    ASSERT_EQ(success, true);
}
//---------------------------------------------------------------------------
TEST(TreeTest, lookup)
/// test lookup()
{
    Tree<uint32_t, uint32_t> tree;
    size_t n = 5e6;
    std::vector<uint32_t> values(n);
    for (uint32_t i = 0; i < n; ++i) {
        values[i] = i;
    }
    std::random_shuffle(values.begin(), values.end());
    for (uint32_t i : values) {
        tree.insert(i, i);
    }
    for (uint32_t i : values) {
        uint32_t result;
        bool success = tree.lookup(values[i], result);
        ASSERT_EQ(success, true);
        ASSERT_EQ(result, values[i]);
    }
}
#endif
//---------------------------------------------------------------------------
TEST(TreeTest, lookupWithNestedNode)
/// test lookupWithInterpolation()
{
    Tree<uint32_t, uint32_t> tree;
    size_t n = 5e6;
    std::vector<uint32_t> values(n);
    for (uint32_t i = 0; i < n; ++i) {
        values[i] = i;
    }
    std::random_shuffle(values.begin(), values.end());
    for (uint32_t i : values) {
        tree.insert(i, i);
    }
    for (uint32_t i : values) {
        uint32_t result;
        bool success = tree.lookupWithNestedNode(values[i], result);
        ASSERT_EQ(success, true);
        ASSERT_EQ(result, values[i]);
    }
}
//---------------------------------------------------------------------------
TEST(TreeTest, lookupWithPositionHint)
/// test lookupWithPositionHint()
{
    Tree<uint32_t, uint32_t> tree;
    size_t n = 5e6;
    std::vector<uint32_t> values(n);
    for (uint32_t i = 0; i < n; ++i) {
        values[i] = i;
    }
    std::random_shuffle(values.begin(), values.end());
    for (uint32_t i : values) {
        tree.insert(i, i);
    }
    for (uint32_t i : values) {
        uint32_t result;
        bool success = tree.lookupWithPositionHint(values[i], result);
        ASSERT_EQ(success, true);
        ASSERT_EQ(result, values[i]);
    }
}
//---------------------------------------------------------------------------
