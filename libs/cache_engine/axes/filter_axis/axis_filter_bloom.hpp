#pragma once
// V41.F.6.1.R7.5.e axis_filter BloomFilter (Goldstandard-Update, Bloom 1970)
//
// R7.6 Paper-Reference (Task #723):
// Bloom, B.H. "Space/Time Trade-offs in Hash Coding with Allowable Errors."
// Communications of the ACM, Vol. 13, No. 7, July 1970, pp. 422-426.
// DOI: 10.1145/362686.362692
// Klassisches Paper, kein Open-Source-Original (Re-Impl basierend auf
// Pseudocode in Paper §3).

#include "axis_filter_strategy_base.hpp"
#include "axis_filter_subaxes_ft1_to_ft3.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include <axes/filter_axis/axis_filter_flags.hpp>
#include <topics/filter/concepts/topic_filter_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::filter_axis {

/// BloomFilter — Bloom 1970 classical point-query filter.
/// k Hash-Funktionen, m-bit Bitmap. False-positive only, no delete.
class BloomFilter : public FilterStrategyBase<BloomFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using axis_tag  = subaxes::query_type_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::bloom_enabled;

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "filter_bloom"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "BloomFilter (Bloom 1970, k-hash bitmap, point-query)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "BLOOM"; }
};

}  // namespace

namespace comdare::cache_engine::filter_axis {
    static_assert(concepts::FilterStrategy<BloomFilter>);
    static_assert(concepts::CacheEnginePermutationStrategy<BloomFilter>);
}
