// SPDX-License-Identifier: Apache-2.0
// Tests fuer comdare::hbm (Aufgabe #106)

#include <cache_engine/hbm/hbm_hierarchy.hpp>

#include <gtest/gtest.h>

namespace hbm = comdare::hbm;

TEST(HbmHierarchy, TierKindNameMapping) {
    EXPECT_EQ(hbm::tier_kind_name(hbm::TierKind::L1Cache), "L1Cache");
    EXPECT_EQ(hbm::tier_kind_name(hbm::TierKind::HBM),     "HBM");
    EXPECT_EQ(hbm::tier_kind_name(hbm::TierKind::PMEM),    "PMEM");
}

TEST(HbmHierarchy, X86HasHbmAvailable) {
    hbm::X86HbmHierarchy h;
    EXPECT_TRUE(h.hbm_available());
    EXPECT_FALSE(h.pmem_available());
    EXPECT_EQ(h.platform_name(), "x86_64-hbm-numa");
}

TEST(HbmHierarchy, X86HasFiveTiers) {
    hbm::X86HbmHierarchy h;
    EXPECT_EQ(h.tier_count(), 5u);
    EXPECT_EQ(h.tier_info(0).kind, hbm::TierKind::L1Cache);
    EXPECT_EQ(h.tier_info(4).kind, hbm::TierKind::HBM);
    EXPECT_GT(h.tier_info(4).bandwidth_gb_s, h.tier_info(3).bandwidth_gb_s);
}

TEST(HbmHierarchy, ArmDdrHasNoHbm) {
    hbm::ArmDdrHierarchy h;
    EXPECT_FALSE(h.hbm_available());
    EXPECT_EQ(h.tier_count(), 4u);
    EXPECT_EQ(h.platform_name(), "arm64-ddr");
}

TEST(HbmHierarchy, GenericFallbackHasMinimalTiers) {
    hbm::GenericFallback h;
    EXPECT_FALSE(h.hbm_available());
    EXPECT_FALSE(h.pmem_available());
    EXPECT_EQ(h.tier_count(), 3u);
    EXPECT_EQ(h.platform_name(), "generic-fallback");
}

TEST(HbmHierarchy, FactoryCreateGeneric) {
    auto h = hbm::HbmHierarchyFactory::create(hbm::PlatformChoice::Generic);
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(h->platform_name(), "generic-fallback");
}

TEST(HbmHierarchy, FactoryCreateX86Hbm) {
    auto h = hbm::HbmHierarchyFactory::create(hbm::PlatformChoice::X86_Hbm);
    ASSERT_NE(h, nullptr);
    EXPECT_TRUE(h->hbm_available());
}

TEST(HbmHierarchy, FactoryAutoDetectReturnsValidHierarchy) {
    auto h = hbm::HbmHierarchyFactory::create(hbm::PlatformChoice::AutoDetect);
    ASSERT_NE(h, nullptr);
    EXPECT_GT(h->tier_count(), 0u);
    EXPECT_FALSE(h->platform_name().empty());
}

TEST(HbmHierarchy, TierInfoOutOfRangeReturnsDefault) {
    hbm::X86HbmHierarchy h;
    auto t = h.tier_info(100);
    EXPECT_EQ(t.size_bytes, 0u);
}

TEST(HbmHierarchy, LatencyMonotonicallyIncreases) {
    hbm::X86HbmHierarchy h;
    EXPECT_LT(h.tier_info(0).latency_ns, h.tier_info(1).latency_ns);  // L1 < L2
    EXPECT_LT(h.tier_info(1).latency_ns, h.tier_info(2).latency_ns);  // L2 < L3
    EXPECT_LT(h.tier_info(2).latency_ns, h.tier_info(3).latency_ns);  // L3 < DRAM
}

TEST(HbmHierarchy, HbmHasHigherBandwidthThanDram) {
    hbm::X86HbmHierarchy h;
    auto const dram = h.tier_info(3);
    auto const hbm  = h.tier_info(4);
    ASSERT_EQ(dram.kind, hbm::TierKind::DRAM);
    ASSERT_EQ(hbm.kind,  hbm::TierKind::HBM);
    EXPECT_GT(hbm.bandwidth_gb_s, dram.bandwidth_gb_s);
}

TEST(HbmHierarchy, HbmOnDifferentNumaNode) {
    hbm::X86HbmHierarchy h;
    auto const hbm = h.tier_info(4);
    EXPECT_NE(hbm.numa_node, h.tier_info(3).numa_node);
}
