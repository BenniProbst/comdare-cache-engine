#pragma once
// V41.F.6.1.R7.5.e axis_filter XorFilter (Graf/Lemire 2020)
//
// R7.6 Paper-Reference (Task #723):
// Graf, T.M., Lemire, D. "Xor Filters: Faster and Smaller Than Bloom and
// Cuckoo Filters." ACM Journal of Experimental Algorithmics (JEA), Vol. 25,
// Article No. 1.5, March 2020.
// DOI: 10.1145/3376122
// URL (Preprint): https://arxiv.org/abs/1912.08258
// Code: https://github.com/FastFilter/fastfilter_cpp (Apache-2.0 License)
// Original-Code-File: src/xorfilter/xorfilter.h

#include "axis_filter_strategy_base.hpp"
#include "axis_filter_subaxes_ft1_to_ft3.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include "axis_filter_flags.hpp"
#include "../concepts/topic_filter_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::filter::axis_filter {

/// XorFilter — Graf/Lemire 2020 (JEA, Apache-2.0 License).
/// XOR-Hash-Based Filter mit kleinerer Space-Bound als Bloom (~9 bits/key
/// statt ~10 bits/key bei gleicher false-positive-Rate). Immutable nach Build.
class XorFilter : public FilterStrategyBase<XorFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using axis_tag  = subaxes::error_profile_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::xor_enabled;

    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "filter_xor"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "XorFilter (Graf+Lemire 2020, ~9 bits/key, smaller than Bloom)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "XOR"; }
};

}  // namespace

namespace comdare::cache_engine::filter::axis_filter {
    static_assert(concepts::FilterStrategy<XorFilter>);
    static_assert(concepts::CacheEnginePermutationStrategy<XorFilter>);
}
