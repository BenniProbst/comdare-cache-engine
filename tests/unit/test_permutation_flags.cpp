// Test fuer F10-K PermutationFlags (Termin 7 / 04_uml_hardware_isa §2 + REV 5 §3)

#include <cache_engine/concepts/permutation_flags.hpp>

#include <gtest/gtest.h>

namespace ce = comdare::cache_engine;

TEST(PermutationFlags, AnySetTrueWhenAnyBankNonzero) {
    ce::PermutationFlags f{};
    EXPECT_FALSE(f.any_set());

    f.page_bank = ce::flags::page_bank::DENSEBYTE_ART256;
    EXPECT_TRUE(f.any_set());
}

TEST(PermutationFlags, ToIdentifierIncludesAllTenBanks) {
    ce::PermutationFlags f{};
    f.page_bank      = 0x1ULL;
    f.telemetry_bank = 0x2ULL;
    auto id          = f.to_identifier();
    // 10 banks * 16 hex digits + 9 separators = 169 chars
    EXPECT_EQ(id.size(), 10u * 16u + 9u);
    // erste Bank-Stelle: page_bank ist 0x...1
    EXPECT_EQ(id.back(), '2'); // letzte Bank = telemetry_bank
    EXPECT_EQ(id[15], '1');    // page_bank LSB
}

TEST(PermutationFlags, IsValidCombinationDetectsLeafOnlyWithoutRetroactive) {
    // Block AJ Abhaengigkeit: LeafOnlyCounter ohne RetroactiveAggregation = invalid
    ce::PermutationFlags f{};
    f.telemetry_bank = ce::flags::telemetry_bank::LEAFONLY_COUNTER;
    EXPECT_FALSE(f.is_valid_combination());

    f.telemetry_bank |= ce::flags::telemetry_bank::RETROACTIVE_AGGREGATION;
    EXPECT_TRUE(f.is_valid_combination());
}

TEST(PermutationFlags, IsValidCombinationDetectsLeafOnlySampledWithoutRetroactive) {
    ce::PermutationFlags f{};
    f.telemetry_bank = ce::flags::telemetry_bank::LEAFONLY_SAMPLED;
    EXPECT_FALSE(f.is_valid_combination());
}

TEST(PermutationFlags, IsValidCombinationDetectsArtPageWithoutArtNode) {
    ce::PermutationFlags f{};
    f.page_bank = ce::flags::page_bank::DENSEBYTE_ART256;
    EXPECT_FALSE(f.is_valid_combination());

    f.node_bank = ce::flags::node_bank::ART_NODE256;
    EXPECT_TRUE(f.is_valid_combination());
}

TEST(PermutationFlags, IsValidCombinationDetectsB2TreePageWithoutEmbeddedTree) {
    ce::PermutationFlags f{};
    f.page_bank = ce::flags::page_bank::DECISION_B2TREE;
    EXPECT_FALSE(f.is_valid_combination());

    f.memory_layout_bank = ce::flags::memory_layout_bank::EMBEDDED_TREE;
    EXPECT_TRUE(f.is_valid_combination());
}

TEST(PermutationFlags, EmptyFlagsAreValidByDefault) {
    ce::PermutationFlags f{};
    EXPECT_TRUE(f.is_valid_combination());
}

TEST(PermutationFlags, EqualityIsBitwise) {
    ce::PermutationFlags a{};
    ce::PermutationFlags b{};
    a.isa_bank = ce::flags::isa_bank::X86_AVX2;
    b.isa_bank = ce::flags::isa_bank::X86_AVX2;
    EXPECT_EQ(a, b);

    b.isa_bank |= ce::flags::isa_bank::X86_BMI2;
    EXPECT_NE(a, b);
}

TEST(PermutationFlags, TenBanksDistinctMember) {
    // Stelle sicher, dass jede der 10 Banks unabhaengig gesetzt werden kann
    ce::PermutationFlags f{};
    f.page_bank          = 1;
    f.node_bank          = 1;
    f.traversal_bank     = 1;
    f.value_handle_bank  = 1;
    f.memory_layout_bank = 1;
    f.allocator_bank     = 1;
    f.prefetch_bank      = 1;
    f.concurrency_bank   = 1;
    f.isa_bank           = 1;
    f.telemetry_bank     = 1;
    auto id              = f.to_identifier();
    // alle Banks haben '1' als LSB → 10x '1' an festen Positionen
    int ones = 0;
    for (char c : id)
        if (c == '1') ++ones;
    EXPECT_EQ(ones, 10);
}
