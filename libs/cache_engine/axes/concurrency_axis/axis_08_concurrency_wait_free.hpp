#pragma once
// V41.F.6.1.R7.3 axis_08 WaitFreeConcurrency (per-thread bounded, wait-free)

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
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "concurrency_wait_free"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::concurrency_axis::WaitFreeConcurrency",
                                  "axes/concurrency_axis/axis_08_concurrency_wait_free.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "WaitFreeConcurrency (bounded per-thread steps, strongest guarantee)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "WAIT_FREE"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    // V41 F15 Pfad-A — treibbare Concurrency-Op (acquire/release-Paar). WaitFree = beschraenkte
    // Schrittzahl OHNE Retry (staerkste Garantie, im Ggs. zu LockFree's CAS-Retry-Schleife):
    // acquire() = EIN atomares fetch_add (acquire-Order), release() = EIN atomares fetch_sub
    // (release-Order). Genau eine RMW pro Op, garantiert in 1 Schritt fertig. Reale, strategie-
    // abhaengige Laufzeit: distinkt billiger als LockFree (kein Spin/compare_exchange-Loop), aber
    // echte atomare RMW. Zaehler thread_local-static (via counter_()).
    static void acquire() noexcept { (void)counter_().fetch_add(1u, std::memory_order_acquire); }
    static void release() noexcept { (void)counter_().fetch_sub(1u, std::memory_order_release); }

private:
    [[nodiscard]] static std::atomic<unsigned>& counter_() noexcept {
        static thread_local std::atomic<unsigned> c{0u};
        return c;
    }
};

} // namespace comdare::cache_engine::concurrency_axis

namespace comdare::cache_engine::concurrency_axis {
static_assert(concepts::ConcurrencyStrategy<WaitFreeConcurrency>);
static_assert(concepts::CacheEnginePermutationStrategy<WaitFreeConcurrency>);
} // namespace comdare::cache_engine::concurrency_axis
