#pragma once
// V41.F.6.1.R7.3 axis_08 RcuConcurrency (Read-Copy-Update, Reclamation)
// Klasse C: P29 RCU LGPL-2.1 → eigene Re-Impl (F2 Task #652). is_original=false.

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include "axis_08_concurrency_flags.hpp"
#include "../concepts/topic_concurrency_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency::axis_08_concurrency {

/// RcuConcurrency — Read-Copy-Update: lock-freie Reader, deferred Reclamation
/// via Grace-Periods. (McKenney OLS 2001; URCU Desnoyers 2012.)
class RcuConcurrency : public ConcurrencyStrategyBase<RcuConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using axis_tag  = subaxes::reclamation_scheme_tag;
    using family_id = std::integral_constant<int, 7>;

    static constexpr bool enabled = flags::rcu_enabled;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::RCU;
    }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "concurrency_rcu"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "RcuConcurrency (Read-Copy-Update, deferred grace-period reclamation)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "RCU"; }
};

}  // namespace

namespace comdare::cache_engine::concurrency::axis_08_concurrency {
    static_assert(concepts::ConcurrencyStrategy<RcuConcurrency>);
    static_assert(concepts::CacheEnginePermutationStrategy<RcuConcurrency>);
}
