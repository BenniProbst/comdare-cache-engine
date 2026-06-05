#pragma once
// Per-Achsen-Observer Phase B (2026-06-04) — ObservableFilter<Strategy>: ObservableAxis-Huelle um eine
// filter_axis-Strategie (T16, axis_filter). EXAKT analog axis_05_memory_layout_observable.hpp: die Strategie
// selbst (Bloom/Cuckoo/RangeSurf/Xor) traegt zwar die verhaltens-tragende static Methode filter_probe_scan(),
// hat aber KEIN statistics()/snapshot_t (ist KEIN ObservableAxis). Die Mess-Mechanik gehoert daher in diese
// Huelle, die filter_probe_scan static durchreicht (seg19-Aufrufer bleiben heil) UND einen Instanz-Driver
// observe_probe() ergaenzt, der beim Treiben die Probe-Statistik trackt.
//
// @topic filter @achse 16 @saeule 2 (Per-Achsen-Observer) @task Phase-B
//
// **Achsen-Semantik (REALER In-Memory-Filter, EHRLICH deklariert):** Die filter-Achse misst Punkt-Query-Filter
// (Hauptagent-Entscheid: realer In-Memory-Filter — `buf` ist die reale Pseudo-Bitmap/Bucket-Tabelle, ueber die
// die Strategie ihre k Hash-/Bucket-Probes ausfuehrt; kein Disk-IO, keine erfundene Konstante). Die Huelle macht
// die Probe-Aktivitaet observable: probe_count = Anzahl observe_probe-Aufrufe (Probe-Runden); queries_positive /
// queries_negative = je Query klassifiziert ueber das ECHTE Strategie-Ergebnis (filter_probe_scan mit Einzel-
// Query-Fenster q=1: Treffer-Beitrag != 0 → positive, == 0 → negative); hash_probes_total = ehrlich deklarierte
// Probe-Multiplizitaet (Punkt-Query × probe_multiplicity() der Strategie — Bloom k=4 Bit-Tests, Cuckoo 2 Bucket-
// Lookups; Default 1) ueber alle Queries; last_checksum = letztes filter_probe_scan-Batch-Ergebnis. Jeder Zaehler
// folgt der echten Op — die positive/negative-Klassifikation nutzt KEINE Strategie-Internas, nur das oeffentliche
// filter_probe_scan-Ergebnis je Einzel-Query.
//
// Gating exakt nach Praezedenz (axis_05): snapshot_t/statistics()/reset() unter COMDARE_CE_ENABLE_STATISTICS.
// Bei OFF: filter_probe_scan = nackter Pass-Through (0 Footprint), ObservableAxis<...> = false → observe_all()
// faellt auf EmptyAxisSnapshot zurueck (Release-Pfad, korrekt).

#include "concepts/axis_filter_concept.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::filter_axis {

/// ABI-taugliches Filter-Snapshot (NUR uint64 → standard_layout + trivially_copyable, Cross-ABI-POD-mappbar).
struct FilterStatistics {
    std::uint64_t probe_count        = 0;   ///< Anzahl observe_probe-Aufrufe (Probe-Runden)
    std::uint64_t queries_positive   = 0;   ///< Queries mit Treffer-Beitrag != 0 (Filter sagt "moeglicherweise enthalten")
    std::uint64_t queries_negative   = 0;   ///< Queries mit Treffer-Beitrag == 0 (Filter sagt "definitiv nicht enthalten")
    std::uint64_t hash_probes_total  = 0;   ///< Σ Hash-/Bucket-Probes (Queries × probe_multiplicity der Strategie)
    std::uint64_t last_checksum      = 0;   ///< letztes filter_probe_scan-Batch-Ergebnis (Korrektheits-/Anti-Wegopt-Anker)

    [[nodiscard]] bool operator==(FilterStatistics const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<FilterStatistics>);
static_assert(std::is_trivially_copyable_v<FilterStatistics>);

/// ObservableAxis-Huelle: filter-Strategie + Per-Achsen-Mess-Mechanik (gegated).
/// KEIN Aggregat (private member + Methoden) → direkt als Anatomie-Member haltbar.
template <class Strategy>
    requires concepts::FilterStrategy<Strategy>
class ObservableFilter {
public:
    using strategy_type = Strategy;
    // topic_tag durchgereicht → die Huelle erfuellt FilterComponent/FilterStrategy und ist als filter-Slot
    // einsetzbar (composition_registry / axis_path_serialization rufen C::filter::name()).
    using topic_tag = typename Strategy::topic_tag;

    // Transparenter Decorator: Strategie-Inspektion durchgereicht.
    [[nodiscard]] static constexpr bool             supports_range_query() noexcept { return Strategy::supports_range_query(); }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return Strategy::name(); }
    [[nodiscard]] static constexpr std::string_view family_name()
        noexcept requires requires { Strategy::family_name(); } { return Strategy::family_name(); }
    [[nodiscard]] static constexpr std::string_view flag_suffix()
        noexcept requires requires { Strategy::flag_suffix(); } { return Strategy::flag_suffix(); }
    // get_compiler() ist eine AxisBase-Eigenschaft (Default "original"), die die RAW-Strategie traegt —
    // transparent durchgereicht (SFINAE-sicher: existiert nur, falls die Strategie sie hat).
    [[nodiscard]] static constexpr std::string_view get_compiler()
        noexcept requires requires { Strategy::get_compiler(); } { return Strategy::get_compiler(); }

    /// STATIC Pass-Through (Drop-in-Kompatibilität): die Strategie-Methode unveraendert durchgereicht, damit die
    /// Huelle als filter-Slot die bestehenden seg19-Aufrufer NICHT bricht (abi_adapter.hpp T16
    /// `Filter::filter_probe_scan`). Diese Variante trackt NICHT (static, kein Instanz-State).
    [[nodiscard]] static std::uint64_t filter_probe_scan(unsigned char const* buf, std::size_t n,
                                                         unsigned char const* queries, std::size_t q) noexcept {
        return Strategy::filter_probe_scan(buf, n, queries, q);
    }

    /// Mess-Kopplung (der eigentliche „Driver", Instanz): treibt den Probe-Scan + trackt. Der Observer-Treiber
    /// (abi_adapter::fill_observer_v3 / tier_insert-Kopplung) ruft dies → die Probe-Aktivitaet wird observable.
    /// queries_positive/negative je Query ueber das ECHTE Einzel-Query-Strategie-Ergebnis (q=1-Fenster → keine
    /// Strategie-Internas). Getrennt von der static-Variante, weil die seg19-Aufrufer static bleiben muessen.
    [[nodiscard]] std::uint64_t observe_probe(unsigned char const* buf, std::size_t n,
                                              unsigned char const* queries, std::size_t q) noexcept {
        std::uint64_t const checksum = Strategy::filter_probe_scan(buf, n, queries, q);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.probe_count;
        stats_.hash_probes_total += static_cast<std::uint64_t>(q) * probe_multiplicity();
        // Positiv/Negativ-Split je Query ueber das oeffentliche Einzel-Query-Ergebnis der Strategie (q=1-Fenster).
        for (std::size_t i = 0; i < q; ++i) {
            std::uint64_t const single = Strategy::filter_probe_scan(buf, n, queries + i, std::size_t{1});
            if (single != 0) ++stats_.queries_positive;   // Filter: "moeglicherweise enthalten" (Treffer-Beitrag)
            else             ++stats_.queries_negative;   // Filter: "definitiv nicht enthalten"
        }
        stats_.last_checksum = checksum;
#endif
        return checksum;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = FilterStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; }

private:
    /// Ehrlich deklarierte Probe-Multiplizitaet je Query (Anzahl Hash-/Bucket-Tests). Default 1; bekannte
    /// Strategien tragen ihre k via static probe_multiplicity() (Bloom k=4, Cuckoo 2). Reine Diagnose-Schaetzung
    /// fuer hash_probes_total — KEIN korrektheitskritischer Wert.
    [[nodiscard]] static constexpr std::uint64_t probe_multiplicity() noexcept {
        if constexpr (requires { { Strategy::probe_multiplicity() } -> std::convertible_to<std::uint64_t>; }) {
            return static_cast<std::uint64_t>(Strategy::probe_multiplicity());
        } else {
            return 1u;
        }
    }

    snapshot_t stats_{};
#endif
};

}  // namespace comdare::cache_engine::filter_axis
