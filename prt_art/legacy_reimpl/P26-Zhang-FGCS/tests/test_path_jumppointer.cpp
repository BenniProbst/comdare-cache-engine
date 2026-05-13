// SPDX-License-Identifier: Apache-2.0
#include "path_jumppointer_prefetch.hpp"
#include <gtest/gtest.h>

namespace zf = comdare::prt_art::legacy_reimpl::zhang_fgcs;

TEST(PathJumpPointer, EmptyOnInit) {
    zf::PathJumpPointerPrefetcher p;
    EXPECT_EQ(p.tracked_blocks(), 0u);
    EXPECT_EQ(p.recommend(99), zf::PrefetchKind::Path);
}

TEST(PathJumpPointer, ReadCountIncreases) {
    zf::PathJumpPointerPrefetcher p;
    p.record_read(1);
    p.record_read(1);
    p.record_read(1);
    auto* s = p.stats_for(1);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->read_count, 3u);
}

TEST(PathJumpPointer, RecommendsPathWhenBetter) {
    zf::PathJumpPointerPrefetcher p;
    for (int i = 0; i < 8; ++i) p.record_path_hit(1);
    for (int i = 0; i < 2; ++i) p.record_path_miss(1);
    for (int i = 0; i < 2; ++i) p.record_jump_hit(1);
    for (int i = 0; i < 8; ++i) p.record_jump_miss(1);
    EXPECT_EQ(p.recommend(1), zf::PrefetchKind::Path);
}

TEST(PathJumpPointer, RecommendsJumpWhenBetter) {
    zf::PathJumpPointerPrefetcher p;
    for (int i = 0; i < 2; ++i) p.record_path_hit(2);
    for (int i = 0; i < 8; ++i) p.record_path_miss(2);
    for (int i = 0; i < 9; ++i) p.record_jump_hit(2);
    for (int i = 0; i < 1; ++i) p.record_jump_miss(2);
    EXPECT_EQ(p.recommend(2), zf::PrefetchKind::JumpPointer);
}
