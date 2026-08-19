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
#include <builder/commands/latency_stats.hpp> // D5-1: der EINE Perzentil-Kanon (stats::percentile_ns)
#include "workload_driver/workload_orchestrator.hpp"
// B-3: DIE EINE Pipeline-Spaltenliste (kein Literal mehr hier).
#include <cache_engine/measurement/pipeline_csv_schema.hpp>
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
    // -- NP-23 (#15-Bruch, 19.08.2026): SIEBEN Quell-Flags, eines je transportiertem PMC-Zaehler --
    // (6 HW-Counter oben + branch_misses unten). BEZIFFERUNG: 7 -- nicht die 5/6 der OV-S13-3-Skizze,
    // weil die Quelle (measurement/pmc_source.hpp) seit B-5 fuer ALLE sieben eigene Flags traegt und
    // die WIDE-Pipeline (pmc_zelle) die Sieben-Zaehler-Wahrheit bereits rendert; jede kleinere Zahl
    // liesse "echt 0 vs. keine Quelle" fuer die Rest-Zaehler weiter verschmelzen. uint8_t 0/1 --
    // POD bleibt trivial kopierbar, CSV-Zellen bleiben numerisch.
    std::uint8_t cache_misses_l1_source_available         = 0;
    std::uint8_t cache_misses_l2_source_available         = 0;
    std::uint8_t cache_misses_l3_source_available         = 0;
    std::uint8_t dtlb_misses_source_available             = 0;
    std::uint8_t coherence_invalidations_source_available = 0;
    std::uint8_t energy_micro_joules_source_available     = 0;
    std::uint8_t branch_misses_source_available           = 0;
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
/// p50 (Median) ueber die zusammengefuehrten Op-Latenzen eines Lastprofil-Laufs.
/// SELBSTCHECK (D5-1, 2026-08-09)
///   ZUSICHERT: der Median ist der KANON-Fall q=0.5 (stats::percentile_ns) und KEINE eigene Bauart;
///              bei gerader Sample-Zahl ist das die UNTERE Mitte.
///   ZUSICHERT NICHT: Vergleichbarkeit mit frueheren Laeufen -- vorher lieferte detail::nearest_rank_p
///              hier die OBERE Mitte. Alle vor dem 2026-08-09 erhobenen p50 sind anders gerechnet.
[[nodiscard]] inline std::int64_t merged_p50_ns(workload_driver::WorkloadRunResult const& r) {
    std::vector<std::int64_t> all;
    all.reserve(r.insert_ns.size() + r.lookup_ns.size() + r.erase_ns.size() + r.clear_ns.size());
    all.insert(all.end(), r.insert_ns.begin(), r.insert_ns.end());
    all.insert(all.end(), r.lookup_ns.begin(), r.lookup_ns.end());
    all.insert(all.end(), r.erase_ns.begin(), r.erase_ns.end());
    all.insert(all.end(), r.clear_ns.begin(), r.clear_ns.end());
    return commands::stats::percentile_ns(all, 0.5).count();
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
///
/// M-3a (2026-08-07) -- KEINE BLINDE KOPIE MEHR. Vorher uebernahm dieser Zweig bei `pmc.available` ALLE
/// sieben Zaehler, ohne die feinkoernigen PmcCounters::*_source_available zu befragen. `available` ist
/// aber die ZEILEN-weite Aussage ("mindestens ein Zaehler hat geliefert"), nicht die Aussage ueber den
/// EINZELNEN Zaehler -- auf AMD Zen5 ist available==true (L1D oeffnete), waehrend cache_misses_l3 mangels
/// generischem LL-Event (ENOENT) beim POD-Default 0 bleibt. Jeder flag-tragende Zaehler wird deshalb nur
/// noch uebernommen, wenn SEINE Quelle wirklich offen war.
///
/// EHRLICHE EINORDNUNG DER WIRKUNG (nicht mehr behaupten, als es ist): am heutigen Bestand aendert das
/// keinen einzigen Zahlenwert -- LinuxPerfPmcSource und der PAPI-Pfad setzen Wert und Flag stets
/// gemeinsam, ein Wert ohne Flag entsteht nirgends. Die Pruefung ist damit eine WACHE gegen kuenftige
/// Drift (eine neue IPmcSource, die einen Wert ohne Oeffnungs-Beleg setzt, wird hier nicht mehr
/// durchgereicht), keine Korrektur eines heute falschen Wertes.
///
/// NP-23/NP-24 (#15-Bruch, 19.08.2026) -- DIE FRUEHERE SCHEMA-GRENZE IST GEHOBEN: der POD traegt jetzt
/// je transportiertem PMC-Zaehler ein eigenes Quell-Flag (7 Stueck, s. POD oben). Ein Zaehler ohne
/// Quelle ist damit auch in der Pipeline-CSV von einer echten Nullmessung unterscheidbar; die WIDE-
/// Pipeline (cache_engine_builder_iterator.hpp, pmc_zelle) rendert dieselbe Wahrheit seit B-5 als
/// "n/a" -- und zwar fuer ALLE SIEBEN Zaehler inkl. l1+dtlb (der fruehere Halbsatz "l1/dtlb gehen ueber
/// zelle statt pmc_zelle" war seit B-5 stale und faellt hiermit).
/// NP-24-BAUPUNKT (benannt statt "Abstimmung mit dem #15-Bruch"): DIESER Overload x
/// serialize_measurements_csv (unten) x measurement/pipeline_csv_schema.hpp (Voll-Sicht +7) x
/// measurement/schema_freeze.hpp (Freeze-Nachzug im selben Commit).
[[nodiscard]] inline ComdareMeasurementSnapshotV1
measurement_from_workload_result(workload_driver::WorkloadRunResult const& r, std::string_view permutation_id,
                                 measurement::PmcCounters const& pmc) {
    auto m = measurement_from_workload_result(r, permutation_id);
    if (pmc.available) {
        // JEDER Zaehler traegt seit NP-23 die EIGENE Quelle in den POD -- l1+dtlb eingeschlossen
        // (B-5-Paritaet zur WIDE-Pipeline). Wert-Uebernahme nur mit Quellen-Beleg; am Bestand
        // verhaltensgleich, weil LinuxPerfPmcSource/PAPI Wert und Flag stets gemeinsam setzen
        // (s. EHRLICHE EINORDNUNG oben) -- die Pruefung bleibt die Wache gegen kuenftige Drift.
        if (pmc.cache_misses_l1_source_available) m.cache_misses_l1 = pmc.cache_misses_l1;
        if (pmc.dtlb_misses_source_available) m.dtlb_misses = pmc.dtlb_misses;
        if (pmc.cache_misses_l2_source_available) m.cache_misses_l2 = pmc.cache_misses_l2;
        if (pmc.cache_misses_l3_source_available) m.cache_misses_l3 = pmc.cache_misses_l3;
        if (pmc.branch_misses_source_available) m.branch_misses = pmc.branch_misses;
        if (pmc.coherence_invalidations_source_available) m.coherence_invalidations = pmc.coherence_invalidations;
        if (pmc.energy_micro_joules_source_available) m.energy_micro_joules = pmc.energy_micro_joules;
        m.pmc_available = 1;
        // Die 7 Quell-Flags reisen 1:1 in den POD (und damit in die +7 CSV-Spalten).
        m.cache_misses_l1_source_available         = pmc.cache_misses_l1_source_available ? 1 : 0;
        m.cache_misses_l2_source_available         = pmc.cache_misses_l2_source_available ? 1 : 0;
        m.cache_misses_l3_source_available         = pmc.cache_misses_l3_source_available ? 1 : 0;
        m.dtlb_misses_source_available             = pmc.dtlb_misses_source_available ? 1 : 0;
        m.coherence_invalidations_source_available = pmc.coherence_invalidations_source_available ? 1 : 0;
        m.energy_micro_joules_source_available     = pmc.energy_micro_joules_source_available ? 1 : 0;
        m.branch_misses_source_available           = pmc.branch_misses_source_available ? 1 : 0;
    }
    return m;
}

/// Kanonischer Serializer (das EINE Mess-Schema). Schreibt die 16 Pipeline-kanonischen Spalten (kompatibel
/// zur LaTeX-Pipeline-Stufe 04/05/06) PLUS die 6 Observer-Spalten + pmc_available + 2 Host-Messspalten
/// PLUS die 7 NP-23-Quell-Flags am Zeilenende -- die volle 16+9+7-Sicht (32 Spalten, Altsicht = Praefix).
/// `rows[i]` ↔ `permutation_ids[i]` ↔ `workload_used[i]` (gleiche Länge). Eine Zeile je (Komposition×Lastprofil).
[[nodiscard]] inline std::string serialize_measurements_csv(std::vector<ComdareMeasurementSnapshotV1> const& rows,
                                                            std::vector<std::string> const& permutation_ids,
                                                            std::vector<std::string> const& workload_used) {
    std::ostringstream os;
    // B-3 (2026-08-09): die Spaltenliste stand hier als LITERAL und dreimal identisch woanders. Sie kommt
    // jetzt aus der EINEN Quelle (measurement/pipeline_csv_schema.hpp) -- gerufen, nicht abgeschrieben.
    // Byte-identisch zur Altfassung: 16 Pipeline-Spalten + 9 Zusatzspalten, Komma, abschliessendes '\n'.
    os << ::comdare::cache_engine::measurement::pipeline_voll_csv_header();
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
           << static_cast<unsigned>(m.pmc_available) << ',' << m.branch_misses << ','
           << m.throughput_ops_per_sec
           // NP-23: die 7 Quell-Flags, Reihenfolge == pipeline_csv_schema.hpp (l1,l2,l3,dtlb,coh,energy,branch).
           << ',' << static_cast<unsigned>(m.cache_misses_l1_source_available) << ','
           << static_cast<unsigned>(m.cache_misses_l2_source_available) << ','
           << static_cast<unsigned>(m.cache_misses_l3_source_available) << ','
           << static_cast<unsigned>(m.dtlb_misses_source_available) << ','
           << static_cast<unsigned>(m.coherence_invalidations_source_available) << ','
           << static_cast<unsigned>(m.energy_micro_joules_source_available) << ','
           << static_cast<unsigned>(m.branch_misses_source_available) << '\n';
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
    // B-3: dieselbe EINE Quelle wie die volle Sicht -- die 16 sind deren Praefix, kein zweites Schema.
    os << ::comdare::cache_engine::measurement::pipeline16_csv_header();
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
