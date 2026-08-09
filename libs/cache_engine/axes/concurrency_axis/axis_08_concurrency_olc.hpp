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

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

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
        unsigned const now = version_().load(std::memory_order_acquire);
        valid_sink_()      = (now == snapshot_());
    }

private:
    // `volatile`-SINK des Vergleichs. Der SCHREIBZUGRIFF ist der beobachtbare Effekt, der die
    // Read-Validate-Bahn vor dem Wegoptimieren schuetzt -- gelesen wird der Wert bewusst NIE.
    // Als Zugriffsfunktion formuliert (genau wie version_()/snapshot_()) statt als lokale Variable:
    // eine lokale `set but not used`-Variable mahnen GCC UND clang zu Recht an. So faellt die Warnung
    // URSAECHLICH weg statt per Unterdrueckung -- und die Mess-Bahn bleibt unveraendert, denn erzeugt
    // wird derselbe EINE volatile-Store. Ein `(void)valid_sink;` waere die schlechtere Loesung: es
    // legte einen zusaetzlichen volatile-LOAD in genau den Pfad, dessen Laufzeit hier gemessen wird.
    //
    // thread_local ERGAENZT 09.08.2026 (Warnungs-Runde 1 hatte den zweiten, GETRENNTEN Befund an
    // derselben Stelle): die Senke war als EINZIGE dieser Klasse nicht thread_local, waehrend
    // version_() und snapshot_() es beide sind. Solange einfaedig gemessen wird, faellt das nicht auf;
    // sobald mehrfaedig gemessen wird, ist dieselbe Adresse ein Data Race UND ein False-Sharing-Punkt
    // -- und verfaelscht dann genau die Messung, fuer die die Senke ueberhaupt existiert. Das sind ZWEI
    // Befunde an einer Zeile: die Warnung (hier ueber die Zugriffsfunktion ursaechlich geheilt) und die
    // Teilung (hier ueber thread_local geheilt). Beide zusammen brauchen KEIN [[maybe_unused]]: die
    // Zugriffsfunktion laesst die Warnung gar nicht erst entstehen. Erzeugt wird weiterhin derselbe
    // EINE volatile-Store, die Mess-Bahn bleibt unveraendert.
    [[nodiscard]] static volatile bool& valid_sink_() noexcept {
        static thread_local volatile bool s = false;
        return s;
    }
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
