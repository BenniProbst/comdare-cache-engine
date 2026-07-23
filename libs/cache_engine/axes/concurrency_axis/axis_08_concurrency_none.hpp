#pragma once
// V41.F.6.1.R7.3 axis_08 NoneConcurrency (Baseline, single-threaded)

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include <axes/concurrency_axis/axis_08_concurrency_flags.hpp>
#include <topics/concurrency/concepts/topic_concurrency_concept.hpp>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::concurrency_axis {

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
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "concurrency_none"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::concurrency_axis::NoneConcurrency",
                                  "axes/concurrency_axis/axis_08_concurrency_none.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "NoneConcurrency (no synchronization, single-threaded baseline)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "NONE"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    // V41 F15 Pfad-A — treibbare Concurrency-Op (acquire/release-Paar). NoneConcurrency =
    // single-threaded Baseline: acquire/release sind BEWUSST no-ops (kein Synchronisations-Pattern).
    // Ehrliche Mindest-Op: der Vergleichs-Nullpunkt (0-Overhead) ggü. den synchronisierenden
    // Strategien. `volatile`-Stempel verhindert vollstaendiges Wegoptimieren, ohne eine Sperre
    // vorzutaeuschen (die reale Op-Differenz entsteht in den anderen Wrappern, nicht hier).
    static void acquire() noexcept {
        static volatile unsigned char sink = 0;
        sink                               = sink;
    }
    static void release() noexcept {
        static volatile unsigned char sink = 0;
        sink                               = sink;
    }
};

} // namespace comdare::cache_engine::concurrency_axis

namespace comdare::cache_engine::concurrency_axis {
static_assert(concepts::ConcurrencyStrategy<NoneConcurrency>);
static_assert(concepts::CacheEnginePermutationStrategy<NoneConcurrency>);
} // namespace comdare::cache_engine::concurrency_axis
