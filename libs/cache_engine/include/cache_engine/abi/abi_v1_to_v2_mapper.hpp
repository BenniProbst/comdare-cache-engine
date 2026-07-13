#pragma once
// V32.EE.4 (2026-05-18 spaet) - ABI V1 -> V2 Migrations-Mapper (DD.4)
//
// @subsystem CE
//
// Migriert V31.F PermutationFlags (9 Banks) zu V32 PermutationFlagsV32 (14 Banks).
// Default-Werte fuer neue Banks (12 HARDWARE, 13 SCHEDULING, 14 ENGINE-CHOICE).
// Erlaubt Backward-Compat: V31-Code kompiliert weiter mit V32-ABI.

#include "../concepts/permutation_flags_v32.hpp"

namespace comdare::cache_engine::abi {

/**
 * @brief AbiV1ToV2Mapper - migriert V31-PermutationFlags zu V32-Format
 * @subsystem CE
 *
 * Verwendung im Bestandscode (V31 Adapter etc.):
 *   auto v32_flags = AbiV1ToV2Mapper::migrate(v31_flags);
 *   // v32_flags hat alle V31-Werte + Default 0 fuer neue Banks
 */
class AbiV1ToV2Mapper {
public:
    /**
     * @brief migrate - V31-Flags zu V32 mit Default-Werten fuer NEU-Banks
     *
     * Erwartet V31-Flags-Struct-Layout (9 Banks):
     *   page (8) + node (5) + traversal (5) + value_handle (3) + memory_layout (3)
     *   + allocator (7) + prefetch (3) + concurrency (5) + isa (4) = ~43 bit
     */
    template <typename V31Flags>
    [[nodiscard]] static concepts::PermutationFlagsV32 migrate(const V31Flags& v31) noexcept {
        concepts::PermutationFlagsV32 v32{};

        // Direkter Mapping pro existierender Bank
        v32.page_bank = v31.page_bank;
        v32.node_bank = v31.node_bank;
        // V31 Traversal flach -> V32 Sub-Banks (Default-Logik):
        //   alter Traversal-Wert geht nach 3.A (SearchAlgo); 3.B + 3.M = 0 (Default Cache-Walk + Linear-Mapping)
        v32.traversal_3a       = v31.traversal_bank;
        v32.traversal_3b       = 0;
        v32.traversal_3m       = 0;
        v32.value_handle_bank  = v31.value_handle_bank;
        v32.memory_layout_bank = v31.memory_layout_bank;
        // V31 Allocator flach -> V32 Sub-Banks 6.1 (Rest = Default 0)
        v32.allocator_6_1 = v31.allocator_bank;
        v32.allocator_6_2 = 0;
        v32.allocator_6_3 = 0;
        v32.allocator_6_4 = 0;
        v32.allocator_6_5 = 0;
        v32.prefetch_bank = v31.prefetch_bank;
        // V31 Concurrency flach -> V32 Sub-Banks 8.1 (8.2 = Default 0 = read-only)
        v32.concurrency_8_1  = v31.concurrency_bank;
        v32.concurrency_8_2  = 0;
        v32.isa_bank         = v31.isa_bank;
        v32.measurement_bank = v31.measurement_bank;
        v32.telemetry_bank   = v31.telemetry_bank;

        // NEUE Banks - Default 0
        v32.hw_12_1            = 0;
        v32.hw_12_2            = 0;
        v32.hw_12_3            = 0;
        v32.hw_12_4            = 0;
        v32.hw_12_5            = 0;
        v32.sched_13_1         = 0;
        v32.sched_13_2         = 0;
        v32.sched_13_3         = 0;
        v32.sched_13_4         = 0;
        v32.sched_13_5         = 0;
        v32.engine_choice_bank = 0;

        return v32;
    }
};

} // namespace comdare::cache_engine::abi
