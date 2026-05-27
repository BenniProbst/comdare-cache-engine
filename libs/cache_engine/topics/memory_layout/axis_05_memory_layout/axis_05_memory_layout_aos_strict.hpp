#pragma once
// V41.F.6.1.R7.1.b axis_05 AoSStrictLayout Wrapper

#include "axis_05_memory_layout_base.hpp"
#include "axis_05_memory_layout_subaxes_hm1_to_hm4.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include "axis_05_memory_layout_flags.hpp"
#include "../concepts/topic_memory_layout_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {

/// AoSStrictLayout — Array-of-Structs, packed (ohne Cache-Line Padding).
/// Vorteil: kompakte Daten. Nachteil: False-Sharing + schlechter Cache-Line-Hit
/// bei concurrent Schreibern. Typischer Wormhole-Layout (strict AoS).
class AoSStrictLayout : public MemoryLayoutBase<AoSStrictLayout> {
public:
    using topic_tag  = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag   = subaxes::data_organization_tag;
    using family_id  = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::aos_strict_enabled;

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 1; }  // strict packed, kein alignment
    [[nodiscard]] static constexpr std::string_view name()            noexcept { return "memory_layout_aos_strict"; }
    [[nodiscard]] static constexpr std::string_view family_name()     noexcept { return "AoSStrictLayout (strict packed, no cache-line alignment, dense)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()     noexcept { return "AOS_STRICT"; }
};

}  // namespace

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {
    static_assert(concepts::MemoryLayoutStrategy<AoSStrictLayout>);
    static_assert(concepts::CacheEnginePermutationStrategy<AoSStrictLayout>);
}
