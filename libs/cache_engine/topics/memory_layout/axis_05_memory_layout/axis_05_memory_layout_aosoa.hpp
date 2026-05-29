#pragma once
// V41.F.6.1 A4 (R7.x) axis_05 AoSoAMemoryLayout Wrapper (2026-05-29)
//
// @topic memory_layout @achse 05 @family 5 AoSoAMemoryLayout
//
// AoSoA — Array of Structures of Arrays (Hybrid-Layout): die Daten werden in BLOECKE fester
// Breite W gruppiert; INNERHALB eines Blocks liegen die Felder spaltenweise (SoA, jedes Feld
// W-fach kontiguierlich → passt in ein SIMD-Register), die Bloecke selbst sind als Array
// hintereinander abgelegt (AoS → gute Locality beim Block-zu-Block-Scan). Vereint damit die
// SIMD-Vektorisierbarkeit von SoA (innerhalb eines Blocks) mit der raeumlichen Locality von AoS
// (ueber Bloecke) — Standard-Layout in HPC/SIMD-Engines. W wird typischerweise auf die SIMD-Lane-
// Zahl gesetzt (z.B. 8 fuer AVX2-u64, 16 fuer AVX-512-u32) → passt zur axis_09b SIMD-Achse.
//
// Deskriptor-Wrapper (klassifiziert das Daten-Layout; die operative Platzierung liegt in den
// Index/Node-Topics) — analog SoA/AoS/Packed dieser Achse.

#include "axis_05_memory_layout_strategy_base.hpp"
#include "axis_05_memory_layout_subaxes_hm1_to_hm4.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include "axis_05_memory_layout_flags.hpp"
#include "../concepts/topic_memory_layout_concept.hpp"
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {

/// AoSoAMemoryLayout — Array-of-Structures-of-Arrays (Block-SoA + Block-AoS Hybrid).
/// SIMD-vektorisierbar wie SoA innerhalb eines Blocks, lokal wie AoS ueber Bloecke.
class AoSoAMemoryLayout : public MemoryLayoutStrategyBase<AoSoAMemoryLayout> {
public:
    using topic_tag  = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag   = subaxes::data_organization_tag;
    using family_id  = std::integral_constant<int, 5>;

    static constexpr bool enabled = flags::aosoa_enabled;

    /// AoSoA-Block-Breite W (Lanes pro Block) — Default 8 (AVX2-u64-Lane-Zahl). Passt zur SIMD-Achse.
    static constexpr std::size_t kBlockWidth = 8;

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::size_t      block_width()     noexcept { return kBlockWidth; }
    [[nodiscard]] static constexpr std::string_view name()            noexcept { return "memory_layout_aosoa"; }
    [[nodiscard]] static constexpr std::string_view family_name()     noexcept { return "AoSoAMemoryLayout (Array-of-Structures-of-Arrays, Block-SoA + Block-AoS Hybrid, SIMD-tiled)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()     noexcept { return "AOSOA"; }
};

}  // namespace

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {
    static_assert(concepts::MemoryLayoutStrategy<AoSoAMemoryLayout>);
    static_assert(concepts::CacheEnginePermutationStrategy<AoSoAMemoryLayout>);
    static_assert(AoSoAMemoryLayout::block_width() == 8);
}
