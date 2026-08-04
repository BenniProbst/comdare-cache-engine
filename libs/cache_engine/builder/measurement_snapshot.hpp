#pragma once
// V5-I1-SUBSTANZ (Task #50) — ComdareMeasurementSnapshotV1: der EINE autoritative Mess-POD (16+6 Spalten).
//
// /goal-I1 (Re-Audit-Blocker, w2s7ckovj): „EIN autoritativer Mess-POD volle 16+6-Spalten." Vorher existierten
// ≥4 inkompatible Schemata (Observer-POD-13 / 16-col-Pipeline-CSV / 24-col-Workload-CSV /
// 16-col-Binary-Record). Dieser POD vereinheitlicht sie: EINE Struktur trägt
//   • 16 PERFORMANCE/META-Spalten (Pipeline-kanonisch, speist die LaTeX-Diagramme 04/05/06):
//       [meta] fingerprint, succeeded, op_count  +  [perf] total_cycles
//       [6 HW-Counter] cache_misses_l1/l2/l3, dtlb_misses, coherence_invalidations, energy_micro_joules
//       [allocator] bytes_allocated, bytes_in_use_peak, external_frag_milli, internal_frag_milli
//     (permutation_id + workload_used = String-Identität, bei der Serialisierung beigestellt)
//   • 6 FUNKTIONALE OBSERVER-Spalten (F15-Substanz „gleiche Funktion, andere Performance"):
//       search_insert, search_lookup, search_hit, search_miss, search_erase, search_peak_occupancy
//
// EHRLICHKEIT (Re-Audit-Lücke 2): die 6 HW-Counter sind P4/PMC-gated. `pmc_available==0` markiert, dass sie
// NICHT real gemessen wurden (statt stiller 0) — kein Schein-Datum ([[feedback_no_success_marks_without_literal_output]]).
//
// @doku docs/sessions/20260531-v5-reverifikation-substanz-luecken.md (Blocker 1) + messarchitektur_v5_design.md

#include <anatomy/observable_tier.hpp>
#include "workload_driver/workload_orchestrator.hpp"
#include <cache_engine/measurement/pmc_source.hpp> // V5-#26: pluggable HW-Counter-Quelle (measurement::PmcCounters) für die +6-Spalten

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder {

/// Der autoritative Mess-POD (16+6). Reine numerische Felder; String-Identität (permutation_id/workload_used)
/// wird bei der Serialisierung beigestellt (POD bleibt trivial-kopierbar/ABI-tauglich).
struct ComdareMeasurementSnapshotV1 {
    static constexpr std::uint32_t kVersion = 1;

    // ── Meta (3 der 16) ───────────────────────────────────────────────────────
    std::uint64_t fingerprint = 0; // FNV-1a über permutation_id (stabile Zeilen-Identität)
    std::uint8_t  succeeded   = 1;
    std::uint64_t op_count    = 0;
    // ── Performance (1 der 16) ────────────────────────────────────────────────
    std::uint64_t total_cycles = 0; // repräsentative Latenz (ns), Stufe-05-Konvention
    // ── 6 HW-Counter (6 der 16) — nur gültig wenn pmc_available==1 ─────────────
    std::uint64_t cache_misses_l1         = 0;
    std::uint64_t cache_misses_l2         = 0;
    std::uint64_t cache_misses_l3         = 0;
    std::uint64_t dtlb_misses             = 0;
    std::uint64_t coherence_invalidations = 0;
    std::uint64_t energy_micro_joules     = 0;
    std::uint8_t  pmc_available           = 0; // 0 = HW-Counter NICHT real gemessen (P4-gated, ehrlich)
    // ── Allocator (4 der 16) — real aus dem Observer ──────────────────────────
    std::uint64_t bytes_allocated = 0;
    // A8-S3 / Befund B7 (2026-08-04) -- FEHL-ETIKETTIERUNG, hier DEKLARIERT statt still gelassen:
    // dieses Feld traegt NICHT den Peak, sondern den END-Wert bytes_in_use aus axis_stats[6][1] (s. die
    // Zuweisung unten). Ein Peak braeuchte periodische tier_observe-Zuege, die der Mess-Pfad heute nicht
    // faehrt. Der NAME bleibt unveraendert -- A8-Auflage 3: CSV-Spalten-Namen sind stabil, Alt-Mess-CSV ist
    // Archiv, und tools/latex_anhang liest die Spalte positions-frei nach Namen. Die EHRLICHE Aussage steht
    // stattdessen im WIDE-Schema: alloc_bytes_in_use_peak == "n/a" (lazy_csv_header, Klasse-C-Block C3).
    // Aufloesung = echte Peak-Quelle (Wire-Slot ODER Zeitreihen-Zug), dann fallen Etikett und Wert zusammen.
    std::uint64_t bytes_in_use_peak = 0;
    // A8-S3 / Katalog-Entscheid E9 -- NICHT ERHOBEN, nicht "unbekannt": das SA-T6-Wire-Schema hat gar keine
    // Fragmentierungs-Felder (die GATTUNGS-Wire-Formen Set/Sequence haben sie, axis_stats[5][5]/[5][6]).
    // Diese beiden Felder bleiben deshalb strukturell 0 -- sie sind KEIN Messwert. Die ehrliche Markierung
    // traegt das WIDE-Schema (alloc_external_frag_milli / alloc_internal_frag_milli == "n/a"); hier bleibt
    // das Zahlenformat, weil tools/latex_anhang beide Spalten als double parst (Reader-Bruch waere die
    // schlechtere Ehrlichkeit). Wer diese Zellen auswertet, muss die WIDE-Spalten gegenlesen.
    std::uint64_t external_frag_milli = 0; // Fragmentierung in Promille -- NICHT ERHOBEN (s.o.), strukturell 0
    std::uint64_t internal_frag_milli = 0; // dito
    // ── 6 funktionale Observer-Spalten (die „+6") ─────────────────────────────
    std::uint64_t search_insert         = 0;
    std::uint64_t search_lookup         = 0;
    std::uint64_t search_hit            = 0;
    std::uint64_t search_miss           = 0;
    std::uint64_t search_erase          = 0;
    std::uint64_t search_peak_occupancy = 0;
    // Additive Host-Messspalten (nur volle CSV, NICHT pipeline16).
    std::uint64_t branch_misses          = 0;
    double        throughput_ops_per_sec = 0.0;
};

namespace detail {
/// FNV-1a 64-bit über einen Namen → stabiler Fingerprint (identisch zur f15-Pfad-A-Konvention).
[[nodiscard]] inline std::uint64_t fnv1a(std::string_view s) noexcept {
    std::uint64_t h = 14695981039346656037ULL;
    for (char c : s) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    return h;
}
/// p50 (Median, Nearest-Rank) über die zusammengeführten Op-Latenzen eines Lastprofil-Laufs.
[[nodiscard]] inline std::int64_t merged_p50_ns(workload_driver::WorkloadRunResult const& r) {
    std::vector<std::int64_t> all;
    all.reserve(r.insert_ns.size() + r.lookup_ns.size() + r.erase_ns.size() + r.clear_ns.size());
    all.insert(all.end(), r.insert_ns.begin(), r.insert_ns.end());
    all.insert(all.end(), r.lookup_ns.begin(), r.lookup_ns.end());
    all.insert(all.end(), r.erase_ns.begin(), r.erase_ns.end());
    all.insert(all.end(), r.clear_ns.begin(), r.clear_ns.end());
    return anatomy_commands::detail::nearest_rank_p(std::move(all), 0.5);
}
[[nodiscard]] inline double throughput_ops_per_sec(std::uint64_t op_count, std::int64_t total_ns) noexcept {
    return (total_ns > 0) ? (static_cast<double>(op_count) / (static_cast<double>(total_ns) / 1'000'000'000.0)) : 0.0;
}
} // namespace detail

/// Baut den autoritativen POD aus einem host-seitigen Lastprofil-Lauf (V5-I9 WorkloadRunResult).
/// Die 16 Performance-Spalten + 6 Observer-Spalten werden mit ECHTEN Mess-/Observer-Werten befüllt;
/// die 6 HW-Counter bleiben 0 mit pmc_available=0 (ehrlich, P4-gated).
[[nodiscard]] inline ComdareMeasurementSnapshotV1
measurement_from_workload_result(workload_driver::WorkloadRunResult const& r, std::string_view permutation_id) {
    ComdareMeasurementSnapshotV1 m;
    m.fingerprint            = detail::fnv1a(permutation_id);
    m.succeeded              = 1;
    m.op_count               = r.op_count;
    m.total_cycles           = static_cast<std::uint64_t>(detail::merged_p50_ns(r)); // repräsentative ns
    m.throughput_ops_per_sec = detail::throughput_ops_per_sec(r.op_count, r.total_ns);
    m.pmc_available          = 0; // PMC nicht angebunden
    // I1: aus dem konsolidierten Observer-POD (search→axis_stats[0], alloc→axis_stats[6]).
    m.bytes_allocated = r.observer.axis_stats[6][0]; // ECHT aus Observer
    // A8-S3 / B7: ECHT aus dem Observer -- aber der END-Wert bytes_in_use, NICHT der Peak. Das Feld-Etikett
    // ist historisch (s. die Deklaration am Feld oben); die ehrliche Peak-Aussage steht als "n/a" im
    // WIDE-Schema. Hier wird bewusst NICHTS umgerechnet: ein aus dem Momentanwert "gerechneter" Peak waere
    // die schlimmere Luege.
    m.bytes_in_use_peak     = r.observer.axis_stats[6][1]; // END-Wert (bytes_in_use), s. B7-Deklaration
    m.search_insert         = r.observer.axis_stats[0][3];
    m.search_lookup         = r.observer.axis_stats[0][0];
    m.search_hit            = r.observer.axis_stats[0][1];
    m.search_miss           = r.observer.axis_stats[0][2];
    m.search_erase          = r.observer.axis_stats[0][4];
    m.search_peak_occupancy = r.observer.axis_stats[0][5];
    return m;
}

/// Wie oben, aber befüllt zusätzlich die 6 HW-Counter aus einer PMC-Quelle (#26). `pmc_available` spiegelt
/// EHRLICH `pmc.available`: NullPmcSource → 0 (HW-Spalten bleiben 0), reale Quelle → 1 + echte Werte.
[[nodiscard]] inline ComdareMeasurementSnapshotV1
measurement_from_workload_result(workload_driver::WorkloadRunResult const& r, std::string_view permutation_id,
                                 measurement::PmcCounters const& pmc) {
    auto m = measurement_from_workload_result(r, permutation_id);
    if (pmc.available) {
        m.cache_misses_l1         = pmc.cache_misses_l1;
        m.cache_misses_l2         = pmc.cache_misses_l2;
        m.cache_misses_l3         = pmc.cache_misses_l3;
        m.dtlb_misses             = pmc.dtlb_misses;
        m.branch_misses           = pmc.branch_misses;
        m.coherence_invalidations = pmc.coherence_invalidations;
        m.energy_micro_joules     = pmc.energy_micro_joules;
        m.pmc_available           = 1;
    }
    return m;
}

/// Kanonischer Serializer (das EINE Mess-Schema). Schreibt die 16 Pipeline-kanonischen Spalten (kompatibel
/// zur LaTeX-Pipeline-Stufe 04/05/06) PLUS die 6 Observer-Spalten + pmc_available — die volle 16+6-Sicht.
/// `rows[i]` ↔ `permutation_ids[i]` ↔ `workload_used[i]` (gleiche Länge). Eine Zeile je (Komposition×Lastprofil).
[[nodiscard]] inline std::string serialize_measurements_csv(std::vector<ComdareMeasurementSnapshotV1> const& rows,
                                                            std::vector<std::string> const& permutation_ids,
                                                            std::vector<std::string> const& workload_used) {
    std::ostringstream os;
    // 16 Pipeline-kanonische Spalten (Stufe-04/05-konsumierbar) ...
    os << "permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,"
          "cache_misses_l1,cache_misses_l2,cache_misses_l3,dtlb_misses,"
          "coherence_invalidations,energy_micro_joules,"
          "bytes_allocated,bytes_in_use_peak,external_frag,internal_frag,"
          // ... + 6 Observer-Spalten + pmc_available (die „+6" + Ehrlichkeits-Flag)
          "search_insert,search_lookup,search_hit,search_miss,search_erase,search_peak_occupancy,pmc_available,"
          "branch_misses,throughput_ops_per_sec\n";
    std::size_t const n = rows.size();
    for (std::size_t i = 0; i < n; ++i) {
        auto const&       m = rows[i];
        std::string const pid =
            (i < permutation_ids.size()) ? permutation_ids[i] : std::string{"row_"} + std::to_string(i);
        std::string const wl = (i < workload_used.size()) ? workload_used[i] : std::string{"default"};
        os << pid << ',' << m.fingerprint << ',' << static_cast<unsigned>(m.succeeded) << ',' << wl << ',' << m.op_count
           << ',' << m.total_cycles << ',' << m.cache_misses_l1 << ',' << m.cache_misses_l2 << ',' << m.cache_misses_l3
           << ',' << m.dtlb_misses << ',' << m.coherence_invalidations << ',' << m.energy_micro_joules << ','
           << m.bytes_allocated << ',' << m.bytes_in_use_peak << ',' << m.external_frag_milli << ','
           << m.internal_frag_milli << ',' << m.search_insert << ',' << m.search_lookup << ',' << m.search_hit << ','
           << m.search_miss << ',' << m.search_erase << ',' << m.search_peak_occupancy << ','
           << static_cast<unsigned>(m.pmc_available) << ',' << m.branch_misses << ',' << m.throughput_ops_per_sec
           << '\n';
    }
    return os.str();
}

/// Nur die 16 Pipeline-kanonischen Spalten (für die bestehende LaTeX-Pipeline 04/05, die exakt 16 erwartet).
/// Identisch zur f15-Pfad-A-`--pipeline-csv`-Konvention, aber gespeist aus dem autoritativen POD (echte Daten).
[[nodiscard]] inline std::string
serialize_measurements_pipeline16_csv(std::vector<ComdareMeasurementSnapshotV1> const& rows,
                                      std::vector<std::string> const&                  permutation_ids,
                                      std::vector<std::string> const&                  workload_used) {
    std::ostringstream os;
    os << "permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,"
          "cache_misses_l1,cache_misses_l2,cache_misses_l3,dtlb_misses,"
          "coherence_invalidations,energy_micro_joules,"
          "bytes_allocated,bytes_in_use_peak,external_frag,internal_frag\n";
    for (std::size_t i = 0; i < rows.size(); ++i) {
        auto const&       m = rows[i];
        std::string const pid =
            (i < permutation_ids.size()) ? permutation_ids[i] : std::string{"row_"} + std::to_string(i);
        std::string const wl = (i < workload_used.size()) ? workload_used[i] : std::string{"default"};
        os << pid << ',' << m.fingerprint << ',' << static_cast<unsigned>(m.succeeded) << ',' << wl << ',' << m.op_count
           << ',' << m.total_cycles << ',' << m.cache_misses_l1 << ',' << m.cache_misses_l2 << ',' << m.cache_misses_l3
           << ',' << m.dtlb_misses << ',' << m.coherence_invalidations << ',' << m.energy_micro_joules << ','
           << m.bytes_allocated << ',' << m.bytes_in_use_peak << ',' << m.external_frag_milli << ','
           << m.internal_frag_milli << '\n';
    }
    return os.str();
}

} // namespace comdare::cache_engine::builder
