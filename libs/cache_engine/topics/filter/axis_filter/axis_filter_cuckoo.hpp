#pragma once
// V41.F.6.1.R7.5.e axis_filter CuckooFilter (Fan CoNEXT 2014)

#include "axis_filter_strategy_base.hpp"
#include "axis_filter_subaxes_ft1_to_ft3.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include "axis_filter_flags.hpp"
#include "../concepts/topic_filter_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::filter::axis_filter {

/// CuckooFilter — Fan/Andersen/Kaminsky/Mitzenmacher CoNEXT 2014.
/// Bucketed-Cuckoo-Hashing mit Fingerprints. Vorteil vs Bloom:
/// **unterstuetzt delete + lookup-Counting**. Trade-off: hoeherer Insert-Aufwand.
class CuckooFilter : public FilterStrategyBase<CuckooFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using axis_tag  = subaxes::mutability_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::cuckoo_enabled;

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "filter_cuckoo"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "CuckooFilter (Fan CoNEXT 2014, supports delete + counting)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "CUCKOO"; }
};

}  // namespace

namespace comdare::cache_engine::filter::axis_filter {
    static_assert(concepts::FilterStrategy<CuckooFilter>);
    static_assert(concepts::CacheEnginePermutationStrategy<CuckooFilter>);
}
