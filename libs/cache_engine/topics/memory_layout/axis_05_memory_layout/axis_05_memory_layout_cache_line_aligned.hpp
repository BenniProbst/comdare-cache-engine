#pragma once
// V41.F.6.1.R7.1.b axis_05 CacheLineAlignedLayout Default-Wrapper (Goldstandard-Update)

#include "axis_05_memory_layout_strategy_base.hpp"
#include "axis_05_memory_layout_subaxes_hm1_to_hm4.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include "axis_05_memory_layout_flags.hpp"
#include "../concepts/topic_memory_layout_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {

/// CacheLineAlignedLayout — Default: 64-byte aligned AoS layout.
/// Standard fuer ART/HOT/Masstree/START. Vermeidet False-Sharing,
/// optimal fuer concurrent Schreiber.
class CacheLineAlignedLayout : public MemoryLayoutStrategyBase<CacheLineAlignedLayout> {
public:
    using topic_tag  = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag   = subaxes::alignment_strategy_tag;
    using family_id  = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::cache_line_aligned_enabled;

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view name()            noexcept { return "memory_layout_cache_line_aligned"; }
    [[nodiscard]] static constexpr std::string_view family_name()     noexcept { return "CacheLineAlignedLayout (64-byte AoS, standard cache architectures)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()     noexcept { return "CACHE_LINE_ALIGNED"; }
};

}  // namespace

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {
    static_assert(concepts::MemoryLayoutStrategy<CacheLineAlignedLayout>);
    static_assert(concepts::CacheEnginePermutationStrategy<CacheLineAlignedLayout>);
}
