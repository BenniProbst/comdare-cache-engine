#pragma once
// V41.F.6.1.R7.5.a axis_07 NonePrefetch (Goldstandard-Update)

#include "axis_07_prefetch_strategy_base.hpp"
#include "axis_07_prefetch_subaxes_pf1_to_pf3.hpp"
#include "concepts/axis_07_prefetch_cache_engine_permutation_concept.hpp"
#include "axis_07_prefetch_flags.hpp"
#include "../concepts/topic_prefetch_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::prefetch::axis_07_prefetch {

/// NonePrefetch — Default: kein Prefetch (Baseline fuer Mess-Reihen).
class NonePrefetch : public PrefetchStrategyBase<NonePrefetch> {
public:
    using topic_tag = ::comdare::cache_engine::prefetch::concepts::PrefetchTopicTag;
    using axis_tag  = subaxes::trigger_mechanism_tag;
    using family_id = std::integral_constant<int, 0>;

    static constexpr bool enabled = flags::none_enabled;

    [[nodiscard]] static constexpr bool             is_active()    noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "prefetch_none"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "NonePrefetch (no prefetch baseline)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "NONE"; }
};

}  // namespace

namespace comdare::cache_engine::prefetch::axis_07_prefetch {
    static_assert(concepts::PrefetchStrategy<NonePrefetch>);
    static_assert(concepts::CacheEnginePermutationStrategy<NonePrefetch>);
}
