#pragma once
// Per-Achsen-Vervollstaendigung Phase B (2026-06-04) — ObservableIndexOrg<Strategy>: ObservableAxis-Huelle um
// eine index_organization-Strategie (axis_01, T13). Exaktes Schwester-Muster zu ObservableNodeType<Strategy>
// (axis_04_node_type_observable.hpp) / ObservableMemoryLayout<Strategy> (axis_05_memory_layout_observable.hpp):
// die Strategie selbst (HeapIndexOrganization / ClusteredIndexOrganization / NonClustered / IOT) traegt zwar die
// verhaltens-tragende static `index_org_scan()` (T13-Treibe-Op, abi_adapter.hpp do_seg19:557), hat aber KEIN
// statistics()/snapshot_t (kein ObservableAxis). Die Mess-Mechanik gehoert daher in diese Huelle, die
// index_org_scan durchreicht (Drop-in fuer den seg19-Timer) UND dabei via index_org_observe() trackt.
//
// @topic search_engine @achse 01 @saeule 2 (Per-Achsen-Observer) @task Phase-B-T13
//
// **Achsen-Semantik (treu, nicht erfunden):** Die index_organization-Achse modelliert das Zugriffsmuster der
// Speicher-Organisation (Garcia-Molina/Ullman „Database Systems"): Clustered = SEQUENTIAL (Index-Order ==
// Storage-Order, reiner Summen-Scan), Heap = UNORDERED Full-Scan MIT Predicate-Branch je Record (kein Index →
// kein Frueh-Abbruch), NonClustered = indirekter Lookup ueber eine Index-Indirektion, IOT = Daten in den
// Index-Leaf-Pages eingebettet. Die Counter (scan_count / records_scanned / predicate_evals / indirect_lookups)
// machen genau diese strategie-divergente Scan-Aktivitaet observable; der reine Latenz-Unterschied der Muster
// bleibt der Wall-Clock-Messung vorbehalten (Pfad B, Hybrid-Messmodell, Doku 24 §8).
//
// EHRLICHKEIT der Zaehler (Pflicht-Deklaration, [[no-success-marks-without-literal-output]]): predicate_evals
// wird NUR fuer Strategien hochgezaehlt, die laut ihrer is_clustered()-Eigenschaft wirklich einen Predicate-Scan
// fahren (Heap = nicht-clustered, kein Frueh-Abbruch); fuer Clustered/IOT bleibt predicate_evals==0 (sequentieller
// Summen-Scan ohne Vergleichslast). indirect_lookups folgt has_secondary_indexes() (NonClustered). Kein Zaehler
// wird mit einem erfundenen Konstantwert befuellt — jeder Zaehler folgt der echten, strategie-deklarierten Op.
//
// Gating exakt nach Praezedenz (ObservableNodeType/ObservableMemoryLayout): snapshot_t/statistics()/reset() unter
// COMDARE_CE_ENABLE_STATISTICS. Bei OFF: index_org_scan = nackter static Pass-Through (0 Footprint),
// ObservableAxis<...> = false → observe_all() faellt auf EmptyAxisSnapshot zurueck (Release-Pfad, korrekt).

#include "concepts/axis_01_index_organization_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::index_organization {

/// ABI-taugliches Index-Organisation-Snapshot (standard_layout + trivially_copyable → spaeter in den generischen
/// Cross-ABI-POD ComdareTierObserverSnapshotV3 axis_stats[13] mappbar). NUR uint64-Felder.
struct IndexOrgStatistics {
    std::uint64_t scan_count       = 0;   ///< Anzahl index_org_observe-Aufrufe
    std::uint64_t records_scanned  = 0;   ///< kumulierte Datensatz-Zahl ueber alle Scans
    std::uint64_t predicate_evals  = 0;   ///< Predicate-Vergleiche (NUR fuer nicht-clustered Full-Scan-Strategien)
    std::uint64_t indirect_lookups = 0;   ///< Index-Indirektions-Schritte (NUR fuer Strategien mit Sekundaer-Index)
    std::uint64_t last_checksum    = 0;   ///< letztes index_org_scan-Ergebnis (Korrektheits-/Strategie-Anker)

    [[nodiscard]] bool operator==(IndexOrgStatistics const&) const noexcept = default;
};

/// ObservableAxis-Huelle: index_organization-Strategie + Per-Achsen-Mess-Mechanik (gegated). KEIN Aggregat
/// (private member + Methoden) → direkt als Anatomie-/abi_adapter-Member haltbar. Reicht die static API durch,
/// damit die Huelle ueberall als index_organization-Slot funktioniert (seg19-Timer ruft IndexOrg::index_org_scan;
/// composition_registry/axis_path_serialization rufen C::index_organization::name() / is_clustered()).
template <class Strategy>
    requires concepts::IndexOrganizationStrategy<Strategy>
class ObservableIndexOrg {
public:
    using strategy_type = Strategy;
    using topic_tag     = typename Strategy::topic_tag;  // SearchEngineComponent → IndexOrganizationStrategy erfuellt

    // Transparenter Decorator: Strategie-Inspektion durchgereicht (Pflicht fuer IndexOrganizationStrategy +
    // die bestehenden Aufrufer in composition_registry / axis_path_serialization).
    [[nodiscard]] static constexpr bool             is_clustered()          noexcept { return Strategy::is_clustered(); }
    [[nodiscard]] static constexpr bool             has_secondary_indexes() noexcept { return Strategy::has_secondary_indexes(); }
    [[nodiscard]] static constexpr bool             data_embedded_in_leaf() noexcept { return Strategy::data_embedded_in_leaf(); }
    [[nodiscard]] static constexpr std::string_view name()                  noexcept { return Strategy::name(); }
    [[nodiscard]] static constexpr std::string_view family_name()
        noexcept requires requires { Strategy::family_name(); } { return Strategy::family_name(); }
    [[nodiscard]] static constexpr std::string_view flag_suffix()
        noexcept requires requires { Strategy::flag_suffix(); } { return Strategy::flag_suffix(); }

    /// STATIC Pass-Through (Drop-in-Kompatibilitaet): die verhaltens-tragende Treibe-Op wird unveraendert
    /// durchgereicht, damit die Huelle als index_organization-Slot den seg19-Timer NICHT bricht
    /// (abi_adapter.hpp do_seg19: `IndexOrg::index_org_scan(lbuf, kRecords, kRecordSize)`). Trackt NICHT (static).
    [[nodiscard]] static std::uint64_t index_org_scan(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        return Strategy::index_org_scan(buf, n, record_size);
    }

    /// Mess-Kopplung (der eigentliche „Driver", Instanz): delegiert an die strategie-echte index_org_scan +
    /// trackt strategie-treu. Der Observer-Treiber (abi_adapter fill_observer_v3, Pfad B) ruft dies → die
    /// Index-Organisations-Scan-Aktivitaet wird observable (statistics()). predicate_evals/indirect_lookups
    /// folgen den static-deklarierten Strategie-Eigenschaften (kein erfundener Wert).
    [[nodiscard]] std::uint64_t index_org_observe(unsigned char const* buf, std::size_t n,
                                                  std::size_t record_size) noexcept {
        std::uint64_t const checksum = Strategy::index_org_scan(buf, n, record_size);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.scan_count;
        stats_.records_scanned += static_cast<std::uint64_t>(n);
        // Nicht-clustered = Full-Scan mit Predicate-Branch je Record (kein Index → kein Frueh-Abbruch). Heap/
        // NonClustered: ein Predicate-Eval pro gescanntem Record. Clustered/IOT (sequential): 0 (reiner Summen-Scan).
        if (!Strategy::is_clustered()) {
            stats_.predicate_evals += static_cast<std::uint64_t>(n);
        }
        // Sekundaer-Index-Strategien (NonClustered) leisten je Lookup einen Index-Indirektions-Schritt.
        if (Strategy::has_secondary_indexes()) {
            stats_.indirect_lookups += static_cast<std::uint64_t>(n);
        }
        stats_.last_checksum = checksum;
#endif
        return checksum;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = IndexOrgStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif
};

}  // namespace comdare::cache_engine::index_organization
