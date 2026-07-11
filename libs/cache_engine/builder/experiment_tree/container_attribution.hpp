#pragma once
// CMD-2 / #252 (2026-07-11) — ContainerAttribution: host-seitige "Container-in-SearchAlgorithm"-Attribution
// (E2-Sidecar, Variante a; User "ENTSCHIEDEN" 02.07., #268 "RESERVE-REICHT" -> 0 neue POD-Spalten, ABI-Major 4
// unberuehrt). Thesis-Kern (Trennbarkeits-Problem, 07_results_evaluation:44 "messbar zurechenbar"): der Beitrag des
// internen Speicher-Organs (container_ = LayoutAwareChunkedStore, thesis-kanonisch T4+T5+T6, 02_suchbaeume:374-380)
// INNERHALB der SA-Messung wird host-seitig aus dem BESTEHENDEN Observer-POD (axis_stats) abgeleitet -- reine
// Etikettierung/Aggregation (Doc 24 §5.2), KEINE neue DLL-Messung, KEIN ContainerObserver-Kategorie-Eintrag
// (die Kategorie bleibt der ECHTEN Container-Gattung Variante b reserviert, axis_observer_classification.hpp:12-14
// -- TABU: der #224-Kategorienfehler).
//
// GELIEFERT (kohaerent, doppelzaehl-frei, thesis-geerdet; Deep-Research wf_d054f1d0):
//   c1 store_ops = axis_stats[0][lookup] + [insert] + [erase] = Gesamt-Store-Operationen der Suche T0 ueber den Store.
//        hit/miss ausgeschlossen (lookup == hit + miss, axis_03a_search_algo_linear_scan.hpp:95-99 -> sonst 2x-Zaehlung);
//        peak ausgeschlossen (Gauge/Fuellstand, kein Op-Zaehler). lookup/insert/erase sind disjunkt + gleichdimensional.
//   c3 "Store-Anteil" = LABEL/Gruppierung der bereits transportierten Store-Achsen-Spalten (kStoreAxes = T4/T5/T6);
//        KEINE arithmetische Summe -- die mischte Pruefsummen ([4][3]/[5][4]/[13][4]) + Bytes ([6][0..1]) +
//        4x-dieselbe-Traversierung (T4.find/T5.scan/T11.access/T13.scan messen denselben container_algorithm_-Scan)
//        = Phantom. Die Per-Achsen-Spalten (stat_node_type_*/stat_memory_layout_*/stat_allocator_*) stehen bereits
//        in der WIDE-CSV; "Store-Anteil" ist ihre host-seitige Gruppierung, keine neue Zahl.
//
// BEWUSST NICHT geliefert (Phantom ohne modalitaets-gleiche Definition -- gehoert zur Sensitivitaetsanalyse Reihe B
//   = Cross-Permutation ueber permutierte Kompositionen, 06_evaluation_methodology:17-19, NICHT zu einem Einzel-
//   Snapshot): c2 "Gewichtungs-Anteil" + ein c3-Skalar. Der User-Gate ist 2026-07-11 freigegeben, ABER eine
//   freigegebene Gate macht ein Phantom nicht real (Anti-Phantom-Doktrin) -- ein dimensionslos-inkohaerenter
//   Snapshot-Quotient (c3/c1: Fuellstand-Scan / kumulativer Op-Count) waere eine von der Thesis nicht gedeckte Metrik.

#include "../../anatomy/observable_tier.hpp" // ComdareTierObserverSnapshot + kV3AxisSchema (single-source)

#include <array>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

/// c3-als-Label: die thesis-kanonischen Store-Achsen (LayoutAwareChunkedStore = Knotentyp+Layout+Allokator,
/// 02_suchbaeume_grundlagen:374-380). NICHT T11/T13 -- die Thesis buendelt sie nie in den Store (T13 = eigene
/// Index-Organisations-Achse 03_state_of_the_art:308; T11 = eigenes Value-Handle 04_concept_architecture:99); der
/// "+T11/T13"-Zusatz der Masterplan-Formel ist Ledger-Erweiterung, keine Thesis-Definition.
inline constexpr std::array<std::string_view, 3> kStoreAxes{"node_type", "memory_layout", "allocator"};

/// Host-seitige Container-in-SA-Attribution aus dem bestehenden Observer-POD (0 neue POD-Spalten, reads-only).
struct ContainerAttribution {
    std::uint64_t store_ops = 0; ///< c1: Gesamt-Store-Operationen (lookup + insert + erase ueber T0 / den Store).
};

/// c1: doppelzaehl-frei (lookup == hit + miss; peak = Gauge -> nur die drei disjunkten Op-Familien addieren).
[[nodiscard]] constexpr ContainerAttribution
container_attribution(anatomy::ComdareTierObserverSnapshot const& s) noexcept {
    return ContainerAttribution{s.axis_stats[0][0] + s.axis_stats[0][3] + s.axis_stats[0][4]};
}

// Index-Freeze: bricht den Build (statt still die Daten zu verfaelschen), falls kV3AxisSchema[0] je umgeordnet wird.
static_assert(std::string_view{anatomy::kV3AxisSchema[0].names[0]} == std::string_view{"lookup"});
static_assert(std::string_view{anatomy::kV3AxisSchema[0].names[3]} == std::string_view{"insert"});
static_assert(std::string_view{anatomy::kV3AxisSchema[0].names[4]} == std::string_view{"erase"});

} // namespace comdare::cache_engine::builder::experiment
