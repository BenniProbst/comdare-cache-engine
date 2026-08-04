// test_a8s3_t17_stats_lesezeitpunkt -- A8-S3 (2026-08-04): DAUERHAFTE WACHE gegen den strukturell-0-Befund
// der T17-Observer-Zeile (axis_stats[kV3AxisCount-1]).
//
// BEFUND (Ledger-Nachtrag A8-S1-BEFUNDE, "axis_stats[17]-strukturell-0 = S3-Nachbarschaft"): die
// persistence_target-Achse ist die EINZIGE Achse, deren Mess-Organ AUSSCHLIESSLICH im Pfad-B-Segment-Lauf
// getrieben wird (abi_adapter::fill_segment_timing_v3 ruft pt_organ_.observe_writeback je Batch). Die
// Q1-Sequenz von tier_observe las die Observer-Zeilen aber VOR dem Timing (fill_observer_v3), und
// fill_segment_timing_v3 setzte das Organ an seinem Ende selbst zurueck. Ergebnis:
//   Lesen(0) -> Treiben(n) -> Reset(0) -> Lesen(0) -> ...
// axis_stats[T17][*] war damit STRUKTURELL 0 -- bei JEDEM Lauf, fuer JEDE Strategie, obwohl seg_ns[T17]
// seit A8-S1 nachweislich Zeit traegt (dieselbe Op, dieselbe Schleife). Das ist keine ehrliche 0
// (Vergleichs-Nullpunkt), sondern ein nie gelesener Messwert -- Verstoss gegen die Ehrlichkeits-Doktrin.
//
// FIX (A8-S3, reine BEFUELLUNGS-REIHENFOLGE, KEIN Byte am Wire-Layout): tier_observe liest die
// NUR-Pfad-B-getriebenen Achsen NACH dem Treiben (SCHRITT 3 fill_observer_pathb_driven_v3) und resettet
// deren Organe erst danach (SCHRITT 4). ComdareTierObserverSnapshot bleibt sizeof 1344 / Version 8.
//
// DIESE TU IST DIE WACHE, NICHT NUR DER REGRESSIONSTEST:
//   * sie prueft NIE gegen ein Achsen-Literal, immer gegen anatomy::kV3AxisCount / kV3AxisSchema;
//   * sie trennt STRUKTURELL-0 von EHRLICH-0: dieselbe Probe laeuft mit DiskWritebackTarget (echter
//     Staging-Pfad -> bytes_staged > 0) UND mit MemoryOnlyTarget (kein Rueckschreib-Pfad -> bytes_staged
//     ehrlich 0, aber writeback_rounds/records_staged > 0). Faellt der Lese-Zeitpunkt erneut zurueck,
//     wird BEIDES 0 und beide Faelle brechen;
//   * sie belegt die Kommensurabilitaet Zaehler <-> Zeit: dieselbe Op-Schleife, die seg_ns[T17] fuellt,
//     fuellt writeback_rounds -- die Wache verlangt beide zugleich > 0.
//
// Build: Standalone int main() (kein gtest), Phase-E-Include-Kette wie test_a8s1_t17_vollzaehligkeit.cpp.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/experiment_tree/axis_path_serialization.hpp>       // kCompositionAxisNames (Single-Source)
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // lazy_csv_header / format_csv_row
#include <harness/perm_runner.hpp>                                   // run_observable_perm (realer Mess-Pfad)
#include <permutations/permutation_engine.hpp>                       // PermTuple

#include <compositions/art_reference.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>

#include <axes/persistence_target/axis_persistence_target_disk_writeback.hpp>
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp>

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

/// Die NUR-Pfad-B-getriebene Achse ist die letzte. NIE als Literal, IMMER aus der tragenden Konstante.
constexpr std::size_t kLastAxis = an::kV3AxisCount - 1;

/// Probe-Permutation ueber den T17-Slot parametriert: identische 17 Vorder-Achsen (reale ART-Slots +
/// Array256-Suchorgan), nur die persistence_target-Strategie wechselt. So misst die Wache den
/// LESE-ZEITPUNKT, nicht die Strategie.
template <class PersistenceTarget>
using ProbeTuple = perm::PermTuple<
    ce03a::Array256SearchAlgo, comp::ArtComposition::cache_traversal, comp::ArtComposition::mapping,
    comp::ArtComposition::path_compression, comp::ArtComposition::node_type, comp::ArtComposition::memory_layout,
    comp::ArtComposition::allocator, comp::ArtComposition::prefetch, comp::ArtComposition::concurrency,
    comp::ArtComposition::serialization, comp::ArtComposition::value_handle, comp::ArtComposition::index_organization,
    comp::ArtComposition::io_dispatch, comp::ArtComposition::migration_policy, comp::ArtComposition::filter,
    comp::ArtComposition::queuing_q1, comp::ArtComposition::queuing_q2, PersistenceTarget>;

int g_fail = 0;

void tr(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

/// Feldname aus dem Schema (Single-Source), damit die Ausgabe bei Schema-Nachzug mitzieht.
[[nodiscard]] std::string field_name(std::size_t axis, std::size_t f) {
    char const* n = an::kV3AxisSchema[axis].names[f];
    return (n == nullptr) ? std::string{"<unbenannt>"} : std::string{n};
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

/// Spalten-Index NACH NAME (nie positional -- eine additive Spalte davor darf die Wache nicht verschieben).
[[nodiscard]] std::size_t column_index(std::vector<std::string> const& header, std::string_view name) {
    for (std::size_t i = 0; i < header.size(); ++i)
        if (header[i] == name) return i;
    return header.size();
}

/// Ein Probe-Lauf ueber den realen Mess-Pfad; liefert den EINEN konsolidierten Snapshot.
template <class PersistenceTarget>
[[nodiscard]] ex::PermResult run_probe(std::string const& id) {
    using Composition = an::CompositionFromPermTuple<ProbeTuple<PersistenceTarget>>;
    using Anatomy     = an::SearchAlgorithmAnatomy<Composition>;
    static an::SearchAlgorithmAbiAdapter<Anatomy> tier; // static: gross genug fuer den Stack-Rahmen
    auto* obs = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&tier));
    if (obs == nullptr) {
        tr("IObservableTier via dynamic_cast vorhanden (" + id + ")", false);
        return ex::PermResult{};
    }
    return ex::run_observable_perm(*obs, id, /*n_ops=*/4000);
}

/// Prueft EINE T17-Observer-Zeile. `erwartet_bytes_staged` unterscheidet den echten Staging-Pfad
/// (DiskWritebackTarget) vom deklarierten Vergleichs-Nullpunkt (MemoryOnlyTarget).
void pruefe_t17_zeile(std::string const& id, ex::PermResult const& pr, bool erwartet_bytes_staged) {
    an::ComdareTierObserverSnapshot const& s = pr.unified;
    std::cout << "  -- " << id << " --\n";
    tr("unified observer real (Mess-Build aktiv, " + id + ")", pr.unified_real);
    std::cout << "    seg_ns[T" << kLastAxis << "] = " << s.seg_ns[kLastAxis] << "\n";
    std::cout << "    axis_stats[T" << kLastAxis << "] = ";
    for (std::size_t f = 0; f < an::kV3FieldCount; ++f) {
        if (an::kV3AxisSchema[kLastAxis].names[f] == nullptr) continue;
        std::cout << field_name(kLastAxis, f) << "=" << s.axis_stats[kLastAxis][f] << " ";
    }
    std::cout << "\n";

    // W1: die Zeit ist da (A8-S1-Stand) -- der Nachweis, dass die Achse REAL getrieben wird.
    tr("W1 " + id + ": seg_ns[T" + std::to_string(kLastAxis) + "] > 0 (Achse wird real getrieben)",
       s.seg_ns[kLastAxis] > 0);
    // W2: DER A8-S3-KERN -- die Zaehler derselben Op-Schleife sind sichtbar (vor dem Fix strukturell 0).
    tr("W2 " + id + ": axis_stats[T" + std::to_string(kLastAxis) + "][0] '" + field_name(kLastAxis, 0) +
           "' > 0 (Lese-Zeitpunkt NACH dem Treiben)",
       s.axis_stats[kLastAxis][0] > 0);
    tr("W3 " + id + ": axis_stats[T" + std::to_string(kLastAxis) + "][2] '" + field_name(kLastAxis, 2) +
           "' > 0 (gestagte Records gezaehlt)",
       s.axis_stats[kLastAxis][2] > 0);
    // W4: EHRLICH-0 bleibt erhalten -- der Fix darf den Vergleichs-Nullpunkt nicht zu einer Phantom-Zahl machen.
    if (erwartet_bytes_staged) {
        tr("W4 " + id + ": axis_stats[T" + std::to_string(kLastAxis) + "][1] '" + field_name(kLastAxis, 1) +
               "' > 0 (echter Rueckschreib-Pfad stagt Bytes)",
           s.axis_stats[kLastAxis][1] > 0);
    } else {
        tr("W4 " + id + ": axis_stats[T" + std::to_string(kLastAxis) + "][1] '" + field_name(kLastAxis, 1) +
               "' == 0 (kein Rueckschreib-Pfad -> EHRLICHE 0, keine Phantom-Zahl)",
           s.axis_stats[kLastAxis][1] == 0);
    }
    // W5: device_flushes bleibt 0, solange kein Geraete-Pfad gemeldet wird (Ehrlichkeits-Marke).
    tr("W5 " + id + ": axis_stats[T" + std::to_string(kLastAxis) + "][3] '" + field_name(kLastAxis, 3) +
           "' == 0 (kein Geraete-Flush behauptet)",
       s.axis_stats[kLastAxis][3] == 0);
}

} // namespace

int main() {
    std::cout << "==== A8-S3: T17-Observer-Lesezeitpunkt (kV3AxisCount = " << an::kV3AxisCount << ") ====\n";

    // -- W0 STRUKTUR (compile-time): Schema und POD-Slots decken dieselbe letzte Achse.
    static_assert(ex::kCompositionAxisNames.size() == an::kV3AxisCount,
                  "kCompositionAxisNames und kV3AxisCount sind auseinandergelaufen");
    static_assert(an::kV3AxisSchema[kLastAxis].names[0] != nullptr,
                  "die letzte Achse traegt keine benannte Observer-Spalte -- Schema-Drift");
    std::cout << "    letzte Achse T" << kLastAxis << " = '" << ex::kCompositionAxisNames[kLastAxis]
              << "'  -> Observer-Spalten stat_" << ex::kCompositionAxisNames[kLastAxis] << "_*\n";
    tr("W0 Struktur: letzte Achse ist benannt und in beiden Single-Sources deckungsgleich (static_assert)", true);

    // -- Fall 1: echter Staging-Pfad.
    ex::PermResult const pr_disk = run_probe<pt::DiskWritebackTarget>("a8s3_t17_disk");
    pruefe_t17_zeile("DiskWritebackTarget", pr_disk, /*erwartet_bytes_staged=*/true);

    // -- Fall 2: deklarierter Vergleichs-Nullpunkt (writes_back_to_disk()==false).
    ex::PermResult const pr_mem = run_probe<pt::MemoryOnlyTarget>("a8s3_t17_mem");
    pruefe_t17_zeile("MemoryOnlyTarget", pr_mem, /*erwartet_bytes_staged=*/false);

    // -- W6 CSV-NAHT: die Observer-Spalte der letzten Achse traegt den Messwert (Spalte NACH NAME gesucht).
    ex::LazyMeasuredRow row;
    row.binary_id       = "a8s3_t17_disk";
    row.setting_id      = row.binary_id;
    row.unified         = pr_disk.unified;
    row.unified_real    = pr_disk.unified_real;
    row.total_ns        = pr_disk.total_ns;
    row.n_ops           = pr_disk.n_ops;
    row.timed_ops       = pr_disk.timed_ops;
    row.two_phase_valid = pr_disk.two_phase_valid;

    std::string const stat_col =
        "stat_" + std::string{ex::kCompositionAxisNames[kLastAxis]} + "_" + field_name(kLastAxis, 0);
    std::vector<std::string> const header = split_semicolon(ex::lazy_csv_header());
    std::vector<std::string> const cells  = split_semicolon(ex::format_csv_row(row));
    std::size_t const              col    = column_index(header, stat_col);
    bool const                     found  = (col < header.size()) && (col < cells.size());
    tr("W6a CSV-Header traegt die Spalte '" + stat_col + "'", found);
    if (found) {
        std::cout << "    CSV-Zelle [" << col << "] '" << stat_col << "' = '" << cells[col] << "'\n";
        tr("W6b CSV-Zelle '" + stat_col + "' ist ein Messwert (nicht \"0\")", cells[col] != "0");
    } else {
        ++g_fail;
    }

    if (g_fail == 0)
        std::cout << "==== A8-S3 T17-Lesezeitpunkt-Wache: ALLE OK (Zaehler und Zeit derselben Op-Schleife) ====\n";
    else
        std::cout << "==== A8-S3 T17-Lesezeitpunkt-Wache: " << g_fail << " FEHLER ====\n";
    return g_fail == 0 ? 0 : 1;
}
