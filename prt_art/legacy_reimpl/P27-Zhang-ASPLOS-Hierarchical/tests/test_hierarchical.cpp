// SPDX-License-Identifier: Apache-2.0
#include "hierarchical_bundle_prefetch.hpp"
#include <gtest/gtest.h>

namespace za = comdare::prt_art::legacy_reimpl::zhang_asplos;

TEST(HierarchicalBundle, EmptyOnInit) {
    za::HierarchicalBundlePrefetcher p;
    EXPECT_EQ(p.bundle_count(), 0u);
    EXPECT_EQ(p.completed_bundles(), 0u);
}

TEST(HierarchicalBundle, AddAddressesToL1) {
    za::HierarchicalBundlePrefetcher p;
    EXPECT_EQ(p.add_address_to_bundle(0x1000, 1), 0);
    EXPECT_EQ(p.add_address_to_bundle(0x2000, 1), 0);
    EXPECT_EQ(p.add_address_to_bundle(0x3000, 1), 0);
    EXPECT_EQ(p.bundle_count(), 1u);
    EXPECT_EQ(p.completed_bundles(), 0u);
    // 4. Adresse -> Bundle ist voll
    EXPECT_EQ(p.add_address_to_bundle(0x4000, 1), 0);
    EXPECT_EQ(p.completed_bundles(), 1u);
}

TEST(HierarchicalBundle, AdditionalAddressOpensNewBundle) {
    za::HierarchicalBundlePrefetcher p;
    for (int i = 0; i < 5; ++i) p.add_address_to_bundle(0x1000 + i, 1);
    EXPECT_EQ(p.bundle_count(), 2u);  // 1 voll, 1 mit 1 Adresse
}

TEST(HierarchicalBundle, BundleLevelTracked) {
    za::HierarchicalBundlePrefetcher p;
    p.add_address_to_bundle(0x1000, 1);
    p.add_address_to_bundle(0x2000, 2);
    p.add_address_to_bundle(0x3000, 3);
    EXPECT_EQ(p.bundles_at_level(1), 1u);
    EXPECT_EQ(p.bundles_at_level(2), 1u);
    EXPECT_EQ(p.bundles_at_level(3), 1u);
}

TEST(HierarchicalBundle, InvalidLevelRejected) {
    za::HierarchicalBundlePrefetcher p;
    EXPECT_EQ(p.add_address_to_bundle(0x1000, 0),  4);
    EXPECT_EQ(p.add_address_to_bundle(0x2000, 99), 4);
    EXPECT_EQ(p.bundle_count(), 0u);
}
