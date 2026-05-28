#pragma once
// V41.F.6.1.R7.3 axis_08 LockFreeConcurrency (CAS-basiert, lock-free)

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include "axis_08_concurrency_flags.hpp"
#include "../concepts/topic_concurrency_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency::axis_08_concurrency {

/// LockFreeConcurrency — Lock-free via Compare-And-Swap (std::atomic CAS-Loops).
/// System-weiter Fortschritt garantiert (mindestens ein Thread macht Fortschritt).
class LockFreeConcurrency : public ConcurrencyStrategyBase<LockFreeConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using axis_tag  = subaxes::synchronization_pattern_tag;
    using family_id = std::integral_constant<int, 5>;

    static constexpr bool enabled = flags::lock_free_enabled;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::LockFree;
    }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "concurrency_lock_free"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "LockFreeConcurrency (CAS-based, lock-free progress guarantee)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "LOCK_FREE"; }
};

}  // namespace

namespace comdare::cache_engine::concurrency::axis_08_concurrency {
    static_assert(concepts::ConcurrencyStrategy<LockFreeConcurrency>);
    static_assert(concepts::CacheEnginePermutationStrategy<LockFreeConcurrency>);
}
