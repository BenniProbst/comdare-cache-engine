#pragma once
// V41.F.6.1.R7.3 axis_08 NoneConcurrency (Baseline, single-threaded)

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include "axis_08_concurrency_flags.hpp"
#include "../concepts/topic_concurrency_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency::axis_08_concurrency {

/// NoneConcurrency — Baseline: keine Synchronisation (single-threaded / read-only).
/// Vergleichs-Nullpunkt fuer F15 (Concurrency-Overhead-Messung).
class NoneConcurrency : public ConcurrencyStrategyBase<NoneConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using axis_tag  = subaxes::synchronization_pattern_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::none_enabled;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::None;
    }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "concurrency_none"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "NoneConcurrency (no synchronization, single-threaded baseline)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "NONE"; }
};

}  // namespace

namespace comdare::cache_engine::concurrency::axis_08_concurrency {
    static_assert(concepts::ConcurrencyStrategy<NoneConcurrency>);
    static_assert(concepts::CacheEnginePermutationStrategy<NoneConcurrency>);
}
