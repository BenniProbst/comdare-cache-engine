#pragma once
// V41.F.6.1.R7.3 axis_08 RcuConcurrency (Read-Copy-Update, Reclamation)
// Klasse C: P29 RCU LGPL-2.1 → eigene Re-Impl (F2 Task #652). is_original=false.

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include <axes/concurrency_axis/axis_08_concurrency_flags.hpp>
#include <topics/concurrency/concepts/topic_concurrency_concept.hpp>
#include <atomic>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::concurrency_axis {

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
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "concurrency_rcu"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::concurrency_axis::RcuConcurrency",
                                  "axes/concurrency_axis/axis_08_concurrency_rcu.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "RcuConcurrency (Read-Copy-Update, deferred grace-period reclamation)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "RCU"; }

    // V41 F15 Pfad-A — treibbare Concurrency-Op (acquire/release-Paar). RCU = Read-Copy-Update
    // (McKenney OLS 2001): die Read-Side ist EXTREM billig — rcu_read_lock()/rcu_read_unlock()
    // sind nur ein per-Thread Nesting-Zaehler-Bump (RELAXED-Order, KEINE atomare RMW-Sperre, KEIN
    // Memory-Barrier auf der Read-Side). acquire() inkrementiert, release() dekrementiert den
    // Read-Side-Nesting-Zaehler. Reale, strategie-abhaengige Laufzeit: der billigste
    // synchronisierende Wrapper (nur relaxed-Counter), distinkt von WaitFree (acq/rel-Order) und
    // den Mutex-Wrappern. Zaehler thread_local-static (via read_nesting_()).
    static void acquire() noexcept { read_nesting_().fetch_add(1u, std::memory_order_relaxed); }
    static void release() noexcept { read_nesting_().fetch_sub(1u, std::memory_order_relaxed); }

private:
    [[nodiscard]] static std::atomic<unsigned>& read_nesting_() noexcept {
        static thread_local std::atomic<unsigned> n{0u};
        return n;
    }
};

} // namespace comdare::cache_engine::concurrency_axis

namespace comdare::cache_engine::concurrency_axis {
static_assert(concepts::ConcurrencyStrategy<RcuConcurrency>);
static_assert(concepts::CacheEnginePermutationStrategy<RcuConcurrency>);
} // namespace comdare::cache_engine::concurrency_axis
