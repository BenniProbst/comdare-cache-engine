// V32.II.1 (2026-05-18 spaet) - PermutationFlagsV32 Roundtrip + Sub-Bank-Tests
//
// @subsystem CE
// @phase_owner CEB

#include "cache_engine/concepts/permutation_flags_v32.hpp"

#include <gtest/gtest.h>

using comdare::cache_engine::concepts::PermutationFlagsV32;

TEST(PermutationFlagsV32, DefaultInit) {
    PermutationFlagsV32 p {};
    EXPECT_EQ(p.page_bank, 0);
    EXPECT_EQ(p.node_bank, 0);
    EXPECT_EQ(p.traversal_3a, 0);
    EXPECT_EQ(p.traversal_3b, 0);
    EXPECT_EQ(p.traversal_3m, 0);
    EXPECT_EQ(p.hw_12_1, 0);
    EXPECT_EQ(p.sched_13_1, 0);
    EXPECT_EQ(p.engine_choice_bank, 0);
}

TEST(PermutationFlagsV32, SubBankAchse3) {
    PermutationFlagsV32 p {};
    p.traversal_3a = 0b011;  // 3 (third Sub-Baustein)
    p.traversal_3b = 0b100;  // 4
    p.traversal_3m = 0b10;   // 2
    EXPECT_EQ(p.traversal_3a, 0b011);
    EXPECT_EQ(p.traversal_3b, 0b100);
    EXPECT_EQ(p.traversal_3m, 0b10);
}

TEST(PermutationFlagsV32, SubBankAchse6) {
    PermutationFlagsV32 p {};
    p.allocator_6_1 = 0b101;  // 5
    p.allocator_6_2 = 0b11;   // 3
    p.allocator_6_3 = 0b10;   // 2
    p.allocator_6_4 = 0b01;   // 1
    p.allocator_6_5 = 0b11;   // 3
    EXPECT_EQ(p.allocator_6_1, 0b101);
    EXPECT_EQ(p.allocator_6_2, 0b11);
    EXPECT_EQ(p.allocator_6_3, 0b10);
    EXPECT_EQ(p.allocator_6_4, 0b01);
    EXPECT_EQ(p.allocator_6_5, 0b11);
}

TEST(PermutationFlagsV32, SubBankAchse8) {
    PermutationFlagsV32 p {};
    p.concurrency_8_1 = 0b110;  // 6
    p.concurrency_8_2 = 0b10;   // 2
    EXPECT_EQ(p.concurrency_8_1, 0b110);
    EXPECT_EQ(p.concurrency_8_2, 0b10);
}

TEST(PermutationFlagsV32, NeueAchse12Hardware) {
    PermutationFlagsV32 p {};
    p.hw_12_1 = 0b001;  // AVX2
    p.hw_12_2 = 0b01;   // L1Aware
    p.hw_12_3 = 0b00;   // Local
    p.hw_12_4 = 0b01;   // Prefetch
    p.hw_12_5 = 0b01;   // CAS
    EXPECT_EQ(p.hw_12_1, 0b001);
    EXPECT_EQ(p.hw_12_2, 0b01);
    EXPECT_EQ(p.hw_12_3, 0b00);
    EXPECT_EQ(p.hw_12_4, 0b01);
    EXPECT_EQ(p.hw_12_5, 0b01);
}

TEST(PermutationFlagsV32, NeueAchse13Scheduling) {
    PermutationFlagsV32 p {};
    p.sched_13_1 = 0b001;  // ThreadPerCore
    p.sched_13_2 = 0b010;  // 2 SIMD-Worker
    p.sched_13_3 = 0b01;   // HybridAware
    p.sched_13_4 = 0b01;   // Interleave
    p.sched_13_5 = 0b01;   // MicroBatch
    EXPECT_EQ(p.sched_13_1, 0b001);
    EXPECT_EQ(p.sched_13_2, 0b010);
    EXPECT_EQ(p.sched_13_3, 0b01);
    EXPECT_EQ(p.sched_13_4, 0b01);
    EXPECT_EQ(p.sched_13_5, 0b01);
}

TEST(PermutationFlagsV32, Bank14EngineChoice) {
    PermutationFlagsV32 p {};
    p.engine_choice_bank = 0b11;  // V4 Automatic_Adaptive
    EXPECT_EQ(p.engine_choice_bank, 0b11);
}

TEST(PermutationFlagsV32, OperatorEquality) {
    PermutationFlagsV32 a {};
    PermutationFlagsV32 b {};
    EXPECT_EQ(a, b);
    a.page_bank = 5;
    EXPECT_NE(a, b);
    b.page_bank = 5;
    EXPECT_EQ(a, b);
}

TEST(PermutationFlagsV32, SizeIsBounded) {
    EXPECT_LE(sizeof(PermutationFlagsV32), 16);
}
