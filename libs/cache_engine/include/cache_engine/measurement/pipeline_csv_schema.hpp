#pragma once
// cache_engine/measurement/pipeline_csv_schema.hpp -- B-3 (2026-08-09): DIE EINE Quelle der
// Pipeline-CSV-Spaltenliste (Komma-Schema, Stufe 03 -> 04/05/06).
//
// WARUM DIESE DATEI ENTSTEHT (der Befund, gemessen am 09.08. gegen 3454 verfolgte C/C++-Dateien):
// dieselbe 16-spaltige Kopfzeile stand als LITERAL an vier Stellen des cache-engine-Baums --
//   * builder/measurement_snapshot.hpp:192  (volle Sicht, 16 + 9 Zusatzspalten)
//   * builder/measurement_snapshot.hpp:223  (serialize_measurements_pipeline16_csv)
//   * libs/execution_engine/src/result_aggregator.cpp:63 (ResultAggregator::export_csv)
//   * apps/f15_compare/main.cpp:496         (--pipeline-csv)
// und zusaetzlich zweimal im super-Repo (01_sample_data_generator, 03_binary_to_csv). Das ist genau
// die ABSCHRIFT-Falle: wer eine Spalte an EINER Stelle ergaenzt, bricht die anderen drei nicht --
// eine Loeschung bricht nur bei AUFRUFERN, gegen eine Kopie ist sie wirkungslos. Ab hier gibt es
// EINE Liste; die vier Schreiber RUFEN sie, sie schreiben sie nicht ab.
//
// ABGRENZUNG -- es gibt in diesem Repo DREI verschiedene Mess-Spaltenlisten, und sie sind NICHT
// dasselbe Schema. Diese Datei fuehrt nur die zweite:
//   (1) WIDE / E4, SEMIKOLON, 189 Spalten -- builder/experiment_tree/cache_engine_builder_iterator.hpp
//       :: lazy_csv_header(). Schema-Hoheit der eigentlichen Mess-Ausgabe; genau EINE Definition.
//   (2) PIPELINE, KOMMA, 16 Spalten (+ 9 in der vollen Sicht) -- DIESE Datei. Speist die
//       LaTeX-Pipeline-Stufen 04/05/06.
//   (3) F15-ALT, KOMMA, 12 Spalten -- builder/commands/result_aggregator.hpp :: result_csv_header()
//       ueber ExecutionResult. Anderer Datentyp, anderer Konsument; genau EINE Definition.
//
// ORT: bewusst unter libs/cache_engine/include, weil GENAU DAS die einzige Include-Wurzel ist, die
// alle vier Schreiber schon sehen -- builder/ (ueber libs/cache_engine), libs/execution_engine
// (target_include_directories PUBLIC .../libs/cache_engine/include) und apps/f15_compare. Ein Ort
// unter builder/ waere fuer execution_engine nicht erreichbar gewesen.
//
// KEINE Abhaengigkeit ausser der Standardbibliothek: diese Datei ist reine Schema-Wahrheit und darf
// von jeder Schicht gezogen werden, ohne Layering zu verletzen.
//
// DOKTRIN: C++23, header-only, constexpr; ASCII-only.

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace comdare::cache_engine::measurement {

/// Der Trenner des Pipeline-Schemas. Das WIDE-Schema benutzt ';' -- eine Verwechslung der beiden
/// Trenner ist eine Schema-Aenderung und muss genauso auffallen wie eine Spalten-Aenderung.
inline constexpr char kPipelineCsvTrenner = ',';

/// DIE 16 pipeline-kanonischen Spalten in bindender Reihenfolge (Stufe 03 schreibt sie, Stufe 04/05
/// lesen sie). Reihenfolge ist Vertrag, nicht Geschmack: Stufe 04/05 lesen positionsabhaengig, wo sie
/// nicht header-getrieben sind.
inline constexpr std::array<std::string_view, 16> kPipeline16Spalten{"permutation_id",
                                                                     "fingerprint",
                                                                     "succeeded",
                                                                     "workload_used",
                                                                     "op_count",
                                                                     "total_cycles",
                                                                     "cache_misses_l1",
                                                                     "cache_misses_l2",
                                                                     "cache_misses_l3",
                                                                     "dtlb_misses",
                                                                     "coherence_invalidations",
                                                                     "energy_micro_joules",
                                                                     "bytes_allocated",
                                                                     "bytes_in_use_peak",
                                                                     "external_frag",
                                                                     "internal_frag"};

/// Die Zusatzspalten der VOLLEN Sicht (serialize_measurements_csv). Sie stehen HINTER den 16 --
/// die volle Sicht ist damit ein echtes Praefix-Superset der Pipeline-Sicht, kein zweites Schema.
///
/// SELBSTCHECK zum Kommentar-Ist: die Altfassung nannte diesen Block "+6 Observer-Spalten +
/// pmc_available", tatsaechlich sind es NEUN Namen (6 Observer + pmc_available + branch_misses +
/// throughput_ops_per_sec). Der Nenner stand also falsch im Kommentar und richtig im Code. Genau
/// deshalb steht er ab hier als Konstante, die der Compiler zaehlt, und nicht als Prosa.
inline constexpr std::array<std::string_view, 9> kPipelineVollZusatzSpalten{
    "search_insert",         "search_lookup", "search_hit",    "search_miss",           "search_erase",
    "search_peak_occupancy", "pmc_available", "branch_misses", "throughput_ops_per_sec"};

/// Die NENNER, vom Compiler gezaehlt statt behauptet.
inline constexpr std::size_t kPipeline16SpaltenZahl   = kPipeline16Spalten.size();
inline constexpr std::size_t kPipelineVollSpaltenZahl = kPipeline16Spalten.size() + kPipelineVollZusatzSpalten.size();

static_assert(kPipeline16SpaltenZahl == 16, "Die Pipeline-Sicht ist per Vertrag 16-spaltig (Stufe 04/05).");
static_assert(kPipelineVollSpaltenZahl == 25, "Volle Sicht == 16 Pipeline-Spalten + 9 Zusatzspalten.");

/// Die volle Spaltenliste, ABGELEITET statt abgeschrieben: Praefix sind exakt die 16, danach die 9.
/// Damit kann die volle Sicht per Konstruktion nicht von der Pipeline-Sicht abdriften.
inline constexpr std::array<std::string_view, kPipelineVollSpaltenZahl> kPipelineVollSpalten = [] {
    std::array<std::string_view, kPipelineVollSpaltenZahl> a{};
    std::size_t                                            i = 0;
    for (auto const& s : kPipeline16Spalten) a[i++] = s;
    for (auto const& s : kPipelineVollZusatzSpalten) a[i++] = s;
    return a;
}();

/// Setzt eine Spaltenliste zur Kopfzeile zusammen -- mit abschliessendem '\n', weil GENAU DAS die
/// Form ist, die alle vier Schreiber bisher literal trugen (byte-identische Ruecknahme).
template <std::size_t N>
[[nodiscard]] inline std::string pipeline_kopfzeile(std::array<std::string_view, N> const& spalten,
                                                    char trenner = kPipelineCsvTrenner) {
    std::string kopf;
    for (std::size_t i = 0; i < N; ++i) {
        if (i != 0) kopf += trenner;
        kopf += spalten[i];
    }
    kopf += '\n';
    return kopf;
}

/// DIE Kopfzeile der 16-spaltigen Pipeline-CSV (Stufe 03 -> 04/05/06), inkl. '\n'.
[[nodiscard]] inline std::string pipeline16_csv_header() { return pipeline_kopfzeile(kPipeline16Spalten); }

/// DIE Kopfzeile der vollen Sicht (16 + 9), inkl. '\n'.
[[nodiscard]] inline std::string pipeline_voll_csv_header() { return pipeline_kopfzeile(kPipelineVollSpalten); }

} // namespace comdare::cache_engine::measurement
