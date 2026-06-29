#pragma once
// V32.EE.4 (2026-05-18 spaet) - PermutationFlags 14 Banks (82 bit) Erweiterung
//
// @subsystem CE
// @phase_owner CEB
//
// V31: 9 Banks (~50 bit)
// V32.2: 14 Banks (82 bit) mit Sub-Bank-Bitfield-Encoding fuer:
//   - Bank 3 TRAVERSAL: 3.A SearchAlgo + 3.B Cache-Memory + 3.M Mapping
//   - Bank 6 ALLOCATOR: 6.1-6.5 Sub-Achsen
//   - Bank 8 CONCURRENCY: 8.1 Pattern + 8.2 Locking-Mode
//   - Bank 12 HARDWARE-STRATEGY: 12.1-12.5 (NEU)
//   - Bank 13 SCHEDULING-STRATEGY: 13.1-13.5 (NEU)
//   - Bank 14 ENGINE-CHOICE: V1-V4 (Meta)
//
// Migration: PermutationFlagsV1 (9 Banks) -> PermutationFlagsV32 (14 Banks)
// via abi_v1_to_v2_mapper.hpp

#include <cstdint>

namespace comdare::cache_engine::concepts {

/**
 * @brief PermutationFlagsV32 - 82-bit Permutations-ID (V32.2 ERWEITERUNG)
 * @subsystem CE
 * @phase_owner CEB
 *
 * Bitfield-Layout (Total 82 bit):
 *   Bank 1   PAGE-TYPE                8 bit
 *   Bank 2   NODE-TYPE                5 bit
 *   Bank 3   TRAVERSAL                3+3+2 = 8 bit (gesplittet)
 *   Bank 4   VALUEHANDLE              3 bit
 *   Bank 5   MEMORY-LAYOUT            3 bit
 *   Bank 6   ALLOCATOR                3+2+2+2+2 = 11 bit (gesplittet)
 *   Bank 7   PREFETCH                 3 bit
 *   Bank 8   CONCURRENCY              3+2 = 5 bit (gesplittet)
 *   Bank 9   ISA                      4 bit
 *   Bank 10  MEASUREMENT              4 bit
 *   Bank 11  TELEMETRY-COLLECTION     3 bit (10 Auspraegungen inkl. Kuehn)
 *   Bank 12  HARDWARE-STRATEGY        3+2+2+2+2 = 11 bit (NEU)
 *   Bank 13  SCHEDULING-STRATEGY      3+3+2+2+2 = 12 bit (NEU)
 *   Bank 14  ENGINE-CHOICE            2 bit
 *   ----------
 *   Total                            82 bit (passt in __uint128_t)
 */
struct PermutationFlagsV32 {
    // Bank 1 PAGE-TYPE (8 bit, 26 Bausteine)
    std::uint8_t page_bank : 8 {0};

    // Bank 2 NODE-TYPE (5 bit, 13 Bausteine)
    std::uint8_t node_bank : 5 {0};

    // Bank 3 TRAVERSAL gesplittet (8 bit)
    std::uint8_t traversal_3a : 3 {0}; ///< SearchAlgo-Traversal (8 Sub-Bausteine)
    std::uint8_t traversal_3b : 3 {0}; ///< Cache-Memory-Traversal (6 Sub-Bausteine)
    std::uint8_t traversal_3m : 2 {0}; ///< Mapping (4 Sub-Bausteine)

    // Bank 4 VALUEHANDLE (3 bit)
    std::uint8_t value_handle_bank : 3 {0};

    // Bank 5 MEMORY-LAYOUT (3 bit)
    std::uint8_t memory_layout_bank : 3 {0};

    // Bank 6 ALLOCATOR gesplittet (11 bit)
    std::uint8_t allocator_6_1 : 3 {0}; ///< Allocation-Strategy
    std::uint8_t allocator_6_2 : 2 {0}; ///< Reclamation-Policy
    std::uint8_t allocator_6_3 : 2 {0}; ///< NUMA-Affinity
    std::uint8_t allocator_6_4 : 2 {0}; ///< Huge-Page-Policy
    std::uint8_t allocator_6_5 : 2 {0}; ///< Free-List-Strategy

    // Bank 7 PREFETCH (3 bit)
    std::uint8_t prefetch_bank : 3 {0};

    // Bank 8 CONCURRENCY gesplittet (5 bit)
    std::uint8_t concurrency_8_1 : 3 {0}; ///< Pattern (7 Patterns)
    std::uint8_t concurrency_8_2 : 2 {0}; ///< Locking-Mode (4 Modes)

    // Bank 9 ISA (4 bit)
    std::uint8_t isa_bank : 4 {0};

    // Bank 10 MEASUREMENT (4 bit)
    std::uint8_t measurement_bank : 4 {0};

    // Bank 11 TELEMETRY-COLLECTION (3 bit, 10 Auspraegungen inkl. Kuehn 11.X1-X4)
    std::uint8_t telemetry_bank : 3 {0};

    // Bank 12 HARDWARE-STRATEGY (NEU, 11 bit)
    std::uint8_t hw_12_1 : 3 {0}; ///< SIMD-Family
    std::uint8_t hw_12_2 : 2 {0}; ///< Cache-Level-Targeting
    std::uint8_t hw_12_3 : 2 {0}; ///< NUMA-Strategy
    std::uint8_t hw_12_4 : 2 {0}; ///< Prefetch-Hardware
    std::uint8_t hw_12_5 : 2 {0}; ///< Atomic-Instruction-Family

    // Bank 13 SCHEDULING-STRATEGY (NEU, 12 bit)
    std::uint8_t sched_13_1 : 3 {0}; ///< Worker-Pool-Layout
    std::uint8_t sched_13_2 : 3 {0}; ///< SIMD-Worker-Count-Limit
    std::uint8_t sched_13_3 : 2 {0}; ///< Hetero-Core-Dispatch
    std::uint8_t sched_13_4 : 2 {0}; ///< Co-Routine-Strategy
    std::uint8_t sched_13_5 : 2 {0}; ///< Batch-Granularity

    // Bank 14 ENGINE-CHOICE (2 bit, V1-V4 Meta-Achse)
    std::uint8_t engine_choice_bank : 2 {0};

    // Roundtrip-API
    [[nodiscard]] constexpr bool operator==(const PermutationFlagsV32&) const noexcept = default;
};

static_assert(sizeof(PermutationFlagsV32) <= 16, "PermutationFlagsV32 muss in 16 bytes passen");

} // namespace comdare::cache_engine::concepts
