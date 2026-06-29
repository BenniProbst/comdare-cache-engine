// test_cache_recommendation.cpp - REV 5.2 CacheRecommendation Defaults + Factory

#include "cache_engine/concepts/cache_recommendation.hpp"

#include <gtest/gtest.h>

namespace ce = comdare::cache_engine;

TEST(CacheRecommendation, DefaultIsDoNothing) {
    auto rec = ce::CacheRecommendation::DoNothing();
    EXPECT_EQ(rec.verdict, ce::CacheRecommendation::Verdict::DoNothing);
    EXPECT_FALSE(rec.alloc_hint.has_value());
    EXPECT_FALSE(rec.layout_proposal.has_value());
    EXPECT_FALSE(rec.prefetch_advice.has_value());
    EXPECT_EQ(rec.confidence, 0.0);
}

TEST(CacheRecommendation, AllocateHintRoundTrip) {
    ce::CacheRecommendation rec{};
    rec.verdict    = ce::CacheRecommendation::Verdict::Allocate;
    rec.alloc_hint = ce::MemoryAllocationHint{
        .tier = ce::MemoryAllocationHint::TierKind::Hbm, .alignment_bytes = 64, .size_bytes = 4096};

    ASSERT_TRUE(rec.alloc_hint.has_value());
    EXPECT_EQ(rec.alloc_hint->tier, ce::MemoryAllocationHint::TierKind::Hbm);
    EXPECT_EQ(rec.alloc_hint->size_bytes, 4096u);
}

TEST(CacheRecommendation, MigrationDirectiveRoundTrip) {
    ce::CacheRecommendation rec{};
    rec.verdict             = ce::CacheRecommendation::Verdict::Migrate;
    int dummy               = 0;
    rec.migration_directive = ce::MigrationDirective{
        .source_addr = &dummy, .target_addr = nullptr, .target_tier = ce::MemoryAllocationHint::TierKind::Nvram};

    ASSERT_TRUE(rec.migration_directive.has_value());
    EXPECT_EQ(rec.migration_directive->source_addr, &dummy);
    EXPECT_EQ(rec.migration_directive->target_tier, ce::MemoryAllocationHint::TierKind::Nvram);
}
