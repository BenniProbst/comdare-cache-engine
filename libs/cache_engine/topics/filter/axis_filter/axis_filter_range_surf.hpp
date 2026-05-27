#pragma once
// V41.F.6.1.R7.5.e axis_filter RangeSurfFilter (Zhang SIGMOD 2018)
//
// R7.6 Paper-Reference (Task #723):
// Zhang, H., Lim, H., Leis, V., Andersen, D.G., Kaminsky, M., Keeton, K.,
// Pavlo, A. "SuRF: Practical Range Query Filtering with Fast Succinct Tries."
// Proceedings of the 2018 International Conference on Management of Data
// (SIGMOD 2018), pp. 323-336.
// DOI: 10.1145/3183713.3196931
// URL: https://www.cs.cmu.edu/~huanche1/publications/surf_paper.pdf
// Code: https://github.com/efficient/SuRF (BSD-3-Clause License)
// Original-Code-File: include/surf.hpp + src/louds_dense.cpp

#include "axis_filter_strategy_base.hpp"
#include "axis_filter_subaxes_ft1_to_ft3.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include "axis_filter_flags.hpp"
#include "../concepts/topic_filter_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::filter::axis_filter {

/// RangeSurfFilter — Zhang/Lim/Leis/Andersen/Pavlo SIGMOD 2018.
/// Succinct Range Filter (SuRF) basierend auf LOUDS-Bitmap-Trie.
/// **Erstes Filter mit Range-Query** (lower_bound/upper_bound). Wird in
/// RocksDB-Optimierungen verwendet (Range-Filter fuer SST-Files).
class RangeSurfFilter : public FilterStrategyBase<RangeSurfFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using axis_tag  = subaxes::query_type_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::range_surf_enabled;

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "filter_range_surf"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "RangeSurfFilter (Zhang SIGMOD 2018, succinct LOUDS-Trie, range-query)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "RANGE_SURF"; }
};

}  // namespace

namespace comdare::cache_engine::filter::axis_filter {
    static_assert(concepts::FilterStrategy<RangeSurfFilter>);
    static_assert(concepts::CacheEnginePermutationStrategy<RangeSurfFilter>);
}
