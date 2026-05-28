#pragma once
// V41.F.6.1.R7.3 axis_08 OlcOptimisticConcurrency (Optimistic Lock-Coupling)
// Goldstandard-Update (vorher Stufe-A).

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include "axis_08_concurrency_flags.hpp"
#include "../concepts/topic_concurrency_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency::axis_08_concurrency {

/// OlcOptimisticConcurrency — Optimistic Lock-Coupling (ART-Sync, PRT-ART Default).
/// Versions-Counter pro Node, lock-freie Reader mit Retry-on-Conflict.
/// (Leis et al. "The ART of Practical Synchronization", DaMoN 2016.)
class OlcOptimisticConcurrency : public ConcurrencyStrategyBase<OlcOptimisticConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using axis_tag  = subaxes::synchronization_pattern_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::optimistic_enabled;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::Optimistic;
    }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "olc_optimistic"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "OlcOptimisticConcurrency (Optimistic Lock-Coupling, ART-Sync Pattern)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "OPTIMISTIC"; }
};

}  // namespace

namespace comdare::cache_engine::concurrency::axis_08_concurrency {
    static_assert(concepts::ConcurrencyStrategy<OlcOptimisticConcurrency>);
    static_assert(concepts::CacheEnginePermutationStrategy<OlcOptimisticConcurrency>);
}
