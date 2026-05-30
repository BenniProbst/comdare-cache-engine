#pragma once
// V41.F.6.1.R7.3 axis_08 WaitFreeConcurrency (per-thread bounded, wait-free)

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include <axes/concurrency_axis/axis_08_concurrency_flags.hpp>
#include <topics/concurrency/concepts/topic_concurrency_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency_axis {

/// WaitFreeConcurrency — Wait-free: jeder Thread schliesst in beschraenkter
/// Schrittzahl ab (staerkste Garantie). Z.B. wait-free Reads via Sequence-Lock.
class WaitFreeConcurrency : public ConcurrencyStrategyBase<WaitFreeConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using axis_tag  = subaxes::synchronization_pattern_tag;
    using family_id = std::integral_constant<int, 6>;

    static constexpr bool enabled = flags::wait_free_enabled;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::WaitFree;
    }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "concurrency_wait_free"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "WaitFreeConcurrency (bounded per-thread steps, strongest guarantee)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "WAIT_FREE"; }
};

}  // namespace

namespace comdare::cache_engine::concurrency_axis {
    static_assert(concepts::ConcurrencyStrategy<WaitFreeConcurrency>);
    static_assert(concepts::CacheEnginePermutationStrategy<WaitFreeConcurrency>);
}
