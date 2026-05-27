#pragma once
// V41.F.6.1.R7.1.b axis_05 SoALayout Wrapper

#include "axis_05_memory_layout_base.hpp"
#include "axis_05_memory_layout_subaxes_hm1_to_hm4.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include "axis_05_memory_layout_flags.hpp"
#include "../concepts/topic_memory_layout_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {

/// SoALayout — Struct-of-Arrays. Vorteil: SIMD-Vectorization, gute Cache-Density
/// bei Spalt-orientierten Zugriffen (z.B. nur Key-Scan ohne Value). Typischer
/// Layout fuer columnar OLAP-Indizes + LOUDS-Succinct.
class SoALayout : public MemoryLayoutBase<SoALayout> {
public:
    using topic_tag  = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag   = subaxes::data_organization_tag;
    using family_id  = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::soa_enabled;

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view name()            noexcept { return "memory_layout_soa"; }
    [[nodiscard]] static constexpr std::string_view family_name()     noexcept { return "SoALayout (Struct-of-Arrays, SIMD-friendly, column-scan optimal)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()     noexcept { return "SOA"; }
};

}  // namespace

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {
    static_assert(concepts::MemoryLayoutStrategy<SoALayout>);
    static_assert(concepts::CacheEnginePermutationStrategy<SoALayout>);
}
