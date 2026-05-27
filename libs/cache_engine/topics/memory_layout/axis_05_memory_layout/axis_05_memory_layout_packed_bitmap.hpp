#pragma once
// V41.F.6.1.R7.1.b axis_05 PackedBitmapLayout Wrapper

#include "axis_05_memory_layout_strategy_base.hpp"
#include "axis_05_memory_layout_subaxes_hm1_to_hm4.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include "axis_05_memory_layout_flags.hpp"
#include "../concepts/topic_memory_layout_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {

/// PackedBitmapLayout — Bit-packed (1 Bit pro Slot), succinct.
/// Typischer Layout fuer LOUDS / SuRF Filter-Trees. Sehr kompakt
/// (n * lg(sigma) bits), aber hoeherer Decode-Aufwand pro Lookup.
class PackedBitmapLayout : public MemoryLayoutStrategyBase<PackedBitmapLayout> {
public:
    using topic_tag  = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag   = subaxes::packing_density_tag;
    using family_id  = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::packed_bitmap_enabled;

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 8; }  // 64 bit = 1 word
    [[nodiscard]] static constexpr std::string_view name()            noexcept { return "memory_layout_packed_bitmap"; }
    [[nodiscard]] static constexpr std::string_view family_name()     noexcept { return "PackedBitmapLayout (bit-packed succinct, LOUDS/SuRF, n*lg(sigma) bits)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()     noexcept { return "PACKED_BITMAP"; }
};

}  // namespace

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {
    static_assert(concepts::MemoryLayoutStrategy<PackedBitmapLayout>);
    static_assert(concepts::CacheEnginePermutationStrategy<PackedBitmapLayout>);
}
