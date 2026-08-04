// test_a8s1_t17_vollzaehligkeit -- A8-S1 (2026-08-04): DAUERHAFTE WACHE gegen den stillen T17-Messwert-Verlust.
//
// BEFUND B-6 (A8-Dossier "20260803-a8_f2_benchmarking_schnitt_soll_design.md" Abschn. 3, am Code CONFIRMED):
// drei Kopierschleifen der Mess-Kette liefen mit dem LITERAL 17, obwohl die Achsen-Zahl seit STRUKT-R ORG-18
// anatomy::kV3AxisCount == 18 ist (anatomy/observable_tier.hpp):
//   * abi_adapter.hpp fill_segment_timing (Pfad A): acc[17] wurde GEMESSEN, aber nicht nach out->seg_ns kopiert
//     und nicht in total_ns summiert -> die Zeit floss still in seg_framework_ns.
//   * abi_adapter.hpp fill_segment_timing_v3 (Pfad B): identisches Muster.
//   * node_value_measurement.hpp: die Knoten-Projektion verlor seg_ns[17] UND die axis_stats[17]-Zeile.
// Die CSV schreibt aber kCompositionAxisNames.size() == 18 seg-Spalten -> die Spalte
// "seg_persistence_target_ns" trug IMMER 0 statt eines Messwerts = stiller Messwert-Verlust
// (Verstoss gegen die Ehrlichkeits-Doktrin; 0 ist hier eine Behauptung, kein erhobener Wert).
//
// DIESE TU IST DIE WACHE, NICHT NUR DER REGRESSIONSTEST: sie prueft NIE gegen ein Literal (17/18), sondern
// IMMER gegen anatomy::kV3AxisCount bzw. ex::kCompositionAxisNames. Waechst die Achsen-Zahl auf 19, bricht
// sie an der neuen letzten Achse -- statt deren Messwert still zu verlieren.
//
// Mess-Baustein der Probe: DiskWritebackTarget auf dem T17-Slot. Anders als MemoryOnlyTarget
// (persistence_writeback_scan == return 0) leistet es echte Staging-/Faltungs-Arbeit je Record -> das
// T17-Segment traegt eine unzweideutig positive Zeit, die Wache ist damit nicht auf Uhr-Aufloesung angewiesen.
//
// Build: Standalone int main() (kein gtest), Phase-E-Include-Kette wie test_seg_coverage.cpp.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/experiment_tree/axis_path_serialization.hpp>       // kCompositionAxisNames (Single-Source)
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // lazy_csv_header / format_csv_row
#include <builder/experiment_tree/node_value_measurement.hpp>        // measure_composition<P> (Knoten-Projektion)
#include <harness/perm_runner.hpp>                                   // run_observable_perm (realer Mess-Pfad)
#include <permutations/permutation_engine.hpp>                       // PermTuple

#include <compositions/art_reference.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>

#include <axes/persistence_target/axis_persistence_target_disk_writeback.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace pt   = ::comdare::cache_engine::persistence_target;
namespace perm = ::comdare::cache_engine::permutations;

namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;

/// Die LETZTE Achse ist der Verlust-Kandidat. NIE als Literal, IMMER aus der tragenden Konstante.
constexpr std::size_t kLastAxis = an::kV3AxisCount - 1;

/// Probe-Permutation: reale ART-Slots + Array256-Suchorgan + DiskWritebackTarget auf T17.
/// Als PermTuple formuliert, damit dieselbe Probe SOWOHL den ABI-Adapter (via CompositionFromPermTuple)
/// ALS AUCH die Knoten-Projektion (measure_composition<P>) treibt -- die beiden Fundstellen-Gruppen.
using ProbeTuple = perm::PermTuple<
    ce03a::Array256SearchAlgo, comp::ArtComposition::cache_traversal, comp::ArtComposition::mapping,
    comp::ArtComposition::path_compression, comp::ArtComposition::node_type, comp::ArtComposition::memory_layout,
    comp::ArtComposition::allocator, comp::ArtComposition::prefetch, comp::ArtComposition::concurrency,
    comp::ArtComposition::serialization, comp::ArtComposition::value_handle, comp::ArtComposition::index_organization,
    comp::ArtComposition::io_dispatch, comp::ArtComposition::migration_policy, comp::ArtComposition::filter,
    comp::ArtComposition::queuing_q1, comp::ArtComposition::queuing_q2,
    /* T17 (STRUKT-R ORG-18): echter Staging-Pfad, damit das Segment unzweideutig Zeit traegt */
    pt::DiskWritebackTarget>;

using ProbeComposition = an::CompositionFromPermTuple<ProbeTuple>;

int g_fail = 0;

void tr(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

/// Zerlegt eine ';'-getrennte CSV-Zeile; trailing CR/LF wird verworfen.
[[nodiscard]] std::vector<std::string> split_semicolon(std::string_view line) {
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.remove_suffix(1);
    std::vector<std::string> out;
    std::size_t              begin = 0;
    for (std::size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == ';') {
            out.emplace_back(line.substr(begin, i - begin));
            begin = i + 1;
        }
    }
    return out;
}

/// Spalten-Index NACH NAME aus dem Header (nie positional -- eine additive Spalte davor darf die Wache
/// nicht verschieben). npos-Ersatz: header.size().
[[nodiscard]] std::size_t column_index(std::vector<std::string> const& header, std::string_view name) {
    for (std::size_t i = 0; i < header.size(); ++i)
        if (header[i] == name) return i;
    return header.size();
}

} // namespace

int main() {
    std::cout << "==== A8-S1: T17-Vollzaehligkeit der Mess-Kette (kV3AxisCount = " << an::kV3AxisCount << ") ====\n";

    // -- W1 STRUKTUR (compile-time): die tragende Konstante deckt Namen UND POD-Slots. Bricht bei Achsen-Zuwachs,
    //    der nur EINE der Quellen nachzieht.
    static_assert(ex::kCompositionAxisNames.size() == an::kV3AxisCount,
                  "kCompositionAxisNames und kV3AxisCount sind auseinandergelaufen -- die CSV schriebe mehr oder "
                  "weniger seg-Spalten als der Observer-POD Slots hat.");
    static_assert(sizeof(an::ComdareTierObserverSnapshot{}.seg_ns) / sizeof(std::int64_t) == an::kV3AxisCount,
                  "seg_ns-Slotzahl != kV3AxisCount");
    static_assert(sizeof(ex::NodeObserverSnapshot{}.seg_ns) / sizeof(std::int64_t) == an::kV3AxisCount,
                  "NodeObserverSnapshot::seg_ns-Slotzahl != kV3AxisCount (Knoten-Projektion verlore Slots)");
    tr("W1 Struktur: kCompositionAxisNames.size() == kV3AxisCount == seg_ns-Slots (static_assert)", true);

    std::string const last_axis_name{ex::kCompositionAxisNames[kLastAxis]};
    std::string const last_seg_column = "seg_" + last_axis_name + "_ns";
    std::cout << "    letzte Achse T" << kLastAxis << " = '" << last_axis_name << "'  -> CSV-Spalte '"
              << last_seg_column << "'\n";

    // -- Realer Mess-Pfad: tier_clear -> insert/lookup -> EIN konsolidierter Observer-POD (Pfad B).
    using Anatomy = an::SearchAlgorithmAnatomy<ProbeComposition>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto* obs = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&tier));
    if (obs == nullptr) {
        tr("IObservableTier via dynamic_cast vorhanden", false);
        std::cout << "==== A8-S1 T17-Wache: " << g_fail << " FEHLER ====\n";
        return 1;
    }

    ex::PermResult const                   pr = ex::run_observable_perm(*obs, "a8s1_t17_probe", /*n_ops=*/4000);
    an::ComdareTierObserverSnapshot const& s  = pr.unified;
    tr("unified observer real (Mess-Build aktiv)", pr.unified_real);

    std::cout << "    seg_ns[T0.." << kLastAxis << "] = ";
    for (std::size_t t = 0; t < an::kV3AxisCount; ++t)
        std::cout << s.seg_ns[t] << (t + 1 < an::kV3AxisCount ? "," : "");
    std::cout << "\n    seg_framework_ns=" << s.seg_framework_ns << "  seg_run_total_ns=" << s.seg_run_total_ns << "\n";

    // -- W2 LAUFZEIT (Pfad B, ABI-POD): die letzte Achse traegt Zeit. VOR dem A8-S1-Fix ist dieser Slot 0,
    //    obwohl er gemessen wurde -- genau der stille Verlust.
    std::cout << "    seg_ns[T" << kLastAxis << " " << last_axis_name << "] = " << s.seg_ns[kLastAxis] << "\n";
    tr("W2 Pfad-B-POD: seg_ns[T" + std::to_string(kLastAxis) + " " + last_axis_name +
           "] > 0 (letzte Achse traegt Messwert, nicht 0)",
       s.seg_ns[kLastAxis] > 0);

    // -- W3 IDENTITAET UEBER ALLE SLOTS: Sum(seg_ns[0..kV3AxisCount-1]) + framework == run_total.
    //    Solange die Kopierschleife kuerzer ist als der POD, wandert die verlorene Zeit in seg_framework_ns
    //    und diese Summe stimmt NICHT mehr.
    std::int64_t seg_sum = 0;
    for (std::size_t t = 0; t < an::kV3AxisCount; ++t) seg_sum += s.seg_ns[t];
    std::cout << "    Sum(seg_ns ueber ALLE " << an::kV3AxisCount << " Slots)=" << seg_sum
              << "  + framework=" << s.seg_framework_ns << "  == run_total=" << s.seg_run_total_ns << " ?\n";
    tr("W3 Coverage: Sum(seg_ns ueber ALLE Slots) + seg_framework_ns == seg_run_total_ns",
       (s.seg_run_total_ns > 0) && (seg_sum + s.seg_framework_ns == s.seg_run_total_ns));

    // -- W4 CSV-NAHT: die Spalte der letzten Achse traegt den Messwert. Spalte NACH NAME gesucht.
    ex::LazyMeasuredRow row;
    row.binary_id       = "a8s1_t17_probe";
    row.setting_id      = row.binary_id;
    row.unified         = pr.unified;
    row.unified_real    = pr.unified_real;
    row.total_ns        = pr.total_ns;
    row.n_ops           = pr.n_ops;
    row.timed_ops       = pr.timed_ops;
    row.two_phase_valid = pr.two_phase_valid;

    std::vector<std::string> const header = split_semicolon(ex::lazy_csv_header());
    std::vector<std::string> const cells  = split_semicolon(ex::format_csv_row(row));
    std::size_t const              col    = column_index(header, last_seg_column);
    bool const                     found  = (col < header.size()) && (col < cells.size());
    tr("W4a CSV-Header traegt die Spalte '" + last_seg_column + "'", found);
    if (found) {
        std::cout << "    CSV-Zelle [" << col << "] '" << last_seg_column << "' = '" << cells[col] << "'\n";
        tr("W4b CSV-Zelle '" + last_seg_column + "' ist ein Messwert (nicht \"0\")", cells[col] != "0");
    } else {
        ++g_fail;
    }

    // -- W6 PFAD A (IMeasurableWorkloadV3::run_workload_segmented_v2): die ZWEITE Kopierschleife im
    //    abi_adapter. Eigener POD (ComdareSegmentLatencyV2) mit eigenem total_ns -> hier ist zusaetzlich
    //    pruefbar, dass Kopie UND Summe dieselbe Slot-Zahl sehen (faengt einen halben Nachzug bei Achsen-Zuwachs).
    auto* v3 = dynamic_cast<an::IMeasurableWorkloadV3*>(static_cast<an::IAnatomyBase*>(&tier));
    if (v3 == nullptr) {
        tr("W6 Pfad A: IMeasurableWorkloadV3 vorhanden", false);
    } else {
        an::ComdareSegmentLatencyV2 pa{};
        std::uint64_t const         batches =
            v3->run_workload_segmented_v2(/*ops_per_batch=*/4096, /*batches=*/3, /*seed=*/0xA8501ull, &pa);
        std::int64_t pa_sum = 0;
        for (std::size_t t = 0; t < an::kV3AxisCount; ++t) pa_sum += pa.seg_ns[t];
        std::cout << "    Pfad A: batches_measured=" << pa.batches_measured << "  seg_ns[T" << kLastAxis << " "
                  << last_axis_name << "]=" << pa.seg_ns[kLastAxis] << "  Sum(ALLE)=" << pa_sum
                  << "  total_ns=" << pa.total_ns << "\n";
        tr("W6a Pfad A: run_workload_segmented_v2 lieferte batches_measured > 0",
           batches > 0 && pa.batches_measured > 0);
        tr("W6b Pfad A: seg_ns[T" + std::to_string(kLastAxis) + " " + last_axis_name + "] > 0 (letzte Achse kopiert)",
           pa.seg_ns[kLastAxis] > 0);
        tr("W6c Pfad A: total_ns == Sum(seg_ns ueber ALLE Slots) (Kopie und Summe sehen dieselbe Slot-Zahl)",
           pa.total_ns == pa_sum);
    }

    // -- W5 KNOTEN-PROJEKTION (node_value_measurement.hpp): dieselbe Schleife kopiert seg_ns UND die
    //    axis_stats-Zeile je Slot -- eine korrigierte Grenze deckt beide. Geprueft wird der beobachtbare
    //    Teil (seg_ns der letzten Achse), da die T17-Stats-Quelle pro Timing-Lauf zurueckgesetzt wird.
    ex::NodeValue const nv = ex::measure_composition<ProbeTuple>(/*n_keys=*/512);
    std::cout << "    Knoten-Projektion: observer.seg_ns[T" << kLastAxis << "] = " << nv.observer.seg_ns[kLastAxis]
              << "  (observer_real=" << (nv.observer_real ? 1 : 0) << ")\n";
    tr("W5 Knoten-Projektion: NodeObserverSnapshot.seg_ns[T" + std::to_string(kLastAxis) + "] > 0",
       nv.observer_real && nv.observer.seg_ns[kLastAxis] > 0);

    if (g_fail == 0)
        std::cout << "==== A8-S1 T17-Wache: ALLE OK (kein stiller Messwert-Verlust) ====\n";
    else
        std::cout << "==== A8-S1 T17-Wache: " << g_fail << " FEHLER ====\n";
    return g_fail == 0 ? 0 : 1;
}
