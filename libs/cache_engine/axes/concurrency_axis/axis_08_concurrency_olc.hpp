#pragma once
// V41.F.6.1.R7.3 axis_08 OlcOptimisticConcurrency (Optimistic Lock-Coupling)
// Goldstandard-Update (vorher Stufe-A).

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
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "olc_optimistic"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::concurrency_axis::OlcOptimisticConcurrency",
                                  "axes/concurrency_axis/axis_08_concurrency_olc.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "OlcOptimisticConcurrency (Optimistic Lock-Coupling, ART-Sync Pattern)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "OPTIMISTIC"; }

    // V41 F15 Pfad-A — treibbare Concurrency-Op (acquire/release-Paar). OLC = Optimistic Lock-
    // Coupling (Leis et al., DaMoN 2016): KEIN Blockieren. acquire() liest die Versions-Zahl
    // (atomic load, acquire-Order) in einen thread_local Snapshot; release() liest erneut + VALIDIERT
    // (Re-Read + Vergleich). Reale, strategie-abhaengige Laufzeit (2 atomare Loads + Compare, KEINE
    // RMW/Sperre → billiger als Lock-Free-CAS, charakteristisch fuer den optimistischen Reader-Pfad).
    // Version-Zaehler thread_local-static (via version_()), Snapshot ebenso (via snapshot_()).
    static void acquire() noexcept { snapshot_() = version_().load(std::memory_order_acquire); }
    static void release() noexcept {
        // Optimistische Validierung: Re-Read der Version, Vergleich gegen den acquire-Snapshot.
        // Im Single-Thread-Pfad-A stets gueltig (keine Nebenlaeufer) — exerziert aber die echte
        // Read-Validate-Bahn. `volatile`-Sink verhindert Wegoptimieren des Vergleichs.
        unsigned const       now        = version_().load(std::memory_order_acquire);
        static volatile bool valid_sink = false;
        valid_sink                      = (now == snapshot_());
    }

private:
    [[nodiscard]] static std::atomic<unsigned>& version_() noexcept {
        static thread_local std::atomic<unsigned> v{0u};
        return v;
    }
    [[nodiscard]] static unsigned& snapshot_() noexcept {
        static thread_local unsigned s = 0u;
        return s;
    }
};

} // namespace comdare::cache_engine::concurrency_axis

namespace comdare::cache_engine::concurrency_axis {
static_assert(concepts::ConcurrencyStrategy<OlcOptimisticConcurrency>);
static_assert(concepts::CacheEnginePermutationStrategy<OlcOptimisticConcurrency>);
} // namespace comdare::cache_engine::concurrency_axis
