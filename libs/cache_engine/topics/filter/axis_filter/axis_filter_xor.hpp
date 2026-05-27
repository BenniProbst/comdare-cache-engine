#pragma once
// V41.F.6.1.R7.5.e axis_filter XorFilter (Graf/Lemire 2020)

#include "axis_filter_strategy_base.hpp"
#include "axis_filter_subaxes_ft1_to_ft3.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include "axis_filter_flags.hpp"
#include "../concepts/topic_filter_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::filter::axis_filter {

/// XorFilter — Graf/Lemire 2020 (JEA, MIT-License).
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
