#pragma once
// cache_engine/measurement/schema_freeze.hpp -- SCHEMA-FREEZE STUFE 1 (2026-08-09).
//
// DER GEGENSTAND. Die Mess-Ausgabe hat eine Spaltenliste, und diese Liste ist ein VERTRAG: die
// Auswerte-Stufen, die Resume-Wache, das Lager, die xlsx-Mappe und das Thesis-PDF haengen alle an
// ihr. Bis heute war der einzige Schutz dieses Vertrags gute Absicht. Diese Datei ersetzt die gute
// Absicht durch ein WERKZEUG: eine Wache, die BRICHT, sobald sich die Liste aendert -- bei Zufuegen,
// bei Entfernen UND bei Umordnen.
//
// WARUM DIE EINGEFRORENE LISTE EINE ZWEITE KOPIE IST -- UND WARUM DAS HIER RICHTIG IST.
// Die Hausregel sagt: keine Abschriften, denn eine Loeschung bricht nur bei Aufrufern, gegen eine
// Kopie ist sie wirkungslos. Eine Freeze-Referenz ist die eine Ausnahme, und zwar nicht trotz,
// sondern WEGEN dieser Regel: sie wird nie zum SCHREIBEN benutzt, sondern ausschliesslich zum
// PRUEFEN, und ihr ganzer Zweck ist es, bei Abdrift zu brechen. Die verbotene Kopie ist stumm; diese
// Kopie ist der Alarm.
//
// Der Umkehrschluss ist die eigentliche Falle, gegen die diese Datei gebaut ist: waere die
// Freeze-Referenz als `= die Erzeuger-Liste` definiert, praeft die Wache eine Sache gegen sich
// selbst. Sie waere immer gruen, saehe von aussen exakt aus wie eine scharfe Wache -- und deckte
// nichts. Genau das ist "der blinde Koeder und die blinde Wache sehen identisch aus". Deshalb sind
// kWideSchemaFreezeStufe1 und kPipeline16FreezeStufe1 EIGENSTAENDIGE Literale und KEINE Aliase.
//
// EINE SCHEMA-AENDERUNG IST ERLAUBT -- ABER LAUT. Wer eine Spalte ergaenzt, ergaenzt sie im selben
// Commit auch hier. Das ist kein Migrationszwang, das ist die Zusicherung, dass der Bruch sichtbar
// wird statt still zu passieren.
//
// DIE DREI SCHEMATA, DIE ES WIRKLICH GIBT (gemessen 09.08. gegen 3454 verfolgte C/C++-Dateien):
//   (1) WIDE / E4, Trenner ';', 189 Spalten -- builder/experiment_tree/cache_engine_builder_iterator.hpp
//       :: lazy_csv_header(). GENAU EINE Definition; alle anderen Stellen rufen sie. HIER eingefroren.
//   (2) PIPELINE, Trenner ',', 16 Spalten (+9 in der vollen Sicht) -- measurement/pipeline_csv_schema.hpp.
//       Seit B-3 genau eine Definition (vorher vier Literale). HIER eingefroren.
//   (3) F15-ALT, Trenner ',', 12 Spalten ueber ExecutionResult -- builder/commands/result_aggregator.hpp
//       :: result_csv_header(). Anderer Datentyp, anderer Konsument, genau eine Definition. NICHT
//       eingefroren: das Format ist als Alt-/Loeschkandidat gefuehrt, und ein Freeze darauf wuerde
//       Totes zementieren statt Lebendes zu schuetzen.
//
// KEINE Abhaengigkeit ausser der Standardbibliothek -- die Wache muss von jeder Schicht rufbar sein.
// DOKTRIN: C++23, header-only; ASCII-only.

#include <array>
#include <cstddef>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::measurement {

/// Der Trenner der WIDE-Mess-CSV. Ein Wechsel des Trenners ist eine Schema-Aenderung wie jede andere.
inline constexpr char kWideCsvTrenner = ';';

// =================================================================================================
// 1. DIE WACHE (generisch -- sie kennt kein einzelnes Schema, nur Spaltenlisten)
// =================================================================================================

/// EINE Position, an der SOLL und IST auseinanderlaufen. Ein leerer String heisst "an dieser Position
/// hat diese Liste keine Spalte mehr" (die kuerzere Liste ist zu Ende).
struct SchemaAbweichung {
    std::size_t position = 0; ///< 0-basiert, wie im Code gezaehlt
    std::string soll;
    std::string ist;
};

/// Das Urteil der Wache. Es traegt IMMER beide Nenner mit -- auch im gruenen Fall. Eine Wache, die
/// nur "ok" sagt, ist von einer Wache, die gar nichts geprueft hat, nicht zu unterscheiden.
struct SchemaFreezeBefund {
    std::size_t soll_spalten = 0; ///< NENNER der eingefrorenen Liste
    std::size_t ist_spalten  = 0; ///< NENNER der vorgelegten Liste

    std::vector<std::string> zugefuegt; ///< Namen, die es nur im IST gibt (inkl. Vielfachheit)
    std::vector<std::string> entfernt;  ///< Namen, die es nur im SOLL gibt (inkl. Vielfachheit)

    /// Positionen, die sich unterscheiden, OBWOHL beide Listen dieselbe Namens-Menge tragen -- das
    /// ist reines Umordnen. Nur dann befuellt; bei Zufuegen/Entfernen waere die Positions-Verschiebung
    /// eine Folge und keine eigene Aussage.
    std::vector<SchemaAbweichung> umgeordnet;

    /// Die erste Positions-Abweichung ueberhaupt -- immer gesetzt, wenn nicht identisch. Sie sagt dem
    /// Leser, WO er nachsehen muss, unabhaengig von der Klasse der Aenderung.
    bool             hat_erste_abweichung = false;
    SchemaAbweichung erste_abweichung{};

    bool identisch = false;
};

/// Zerlegt eine Kopfzeile in ihre Spalten. Abschliessende '\n'/'\r' werden abgeschnitten (der
/// Erzeuger haengt sie an, das Schema kennt sie nicht). Eine leere Kopfzeile ergibt 0 Spalten -- NICHT
/// eine Spalte mit leerem Namen; sonst waere "gar kein Kopf" von "ein Kopf mit einer Leerspalte"
/// nicht zu unterscheiden.
[[nodiscard]] inline std::vector<std::string> spalten_aus_kopfzeile(std::string_view kopf, char trenner) {
    while (!kopf.empty() && (kopf.back() == '\n' || kopf.back() == '\r')) kopf.remove_suffix(1);
    std::vector<std::string> out;
    if (kopf.empty()) return out;
    std::size_t i = 0;
    while (true) {
        std::size_t const t   = kopf.find(trenner, i);
        std::size_t const end = (t == std::string_view::npos) ? kopf.size() : t;
        out.emplace_back(kopf.substr(i, end - i));
        if (t == std::string_view::npos) break;
        i = t + 1;
    }
    return out;
}

/// Hilfs-Praedikat: traegt die Liste einen Namen doppelt? Bei Duplikaten ist die Aussage "umgeordnet"
/// nicht mehr eindeutig (zwei gleiche Namen zu tauschen ist keine Aenderung) -- die Freeze-Referenzen
/// muessen deshalb duplikatfrei sein, und der Selbsttest verlangt genau das.
[[nodiscard]] inline bool hat_doppelte_spalten(std::span<std::string const> spalten) {
    std::map<std::string_view, std::size_t> zaehler;
    for (auto const& s : spalten)
        if (++zaehler[s] > 1) return true;
    return false;
}

/// DIE PRUEFUNG. Vergleicht die vorgelegte Spaltenliste gegen die eingefrorene.
///
/// Die Klassen sind bewusst getrennt und nicht in ein einzelnes bool gefaltet: "die Spaltenliste hat
/// sich geaendert" ist als Meldung wertlos, wenn nicht dabeisteht, ob etwas dazukam, wegfiel oder nur
/// die Reihenfolge kippte -- die drei haben voellig verschiedene Folgen fuer die Bestands-Leser.
[[nodiscard]] inline SchemaFreezeBefund pruefe_schema_freeze(std::span<std::string const> ist,
                                                             std::span<std::string const> soll) {
    SchemaFreezeBefund b;
    b.soll_spalten = soll.size();
    b.ist_spalten  = ist.size();

    // (a) Positions-Vergleich ueber die LAENGERE der beiden Listen -- eine gekuerzte Liste darf nicht
    //     dadurch gruen werden, dass die Schleife frueh endet.
    std::vector<SchemaAbweichung> positions_abweichungen;
    std::size_t const             n = (ist.size() > soll.size()) ? ist.size() : soll.size();
    for (std::size_t i = 0; i < n; ++i) {
        std::string const s = (i < soll.size()) ? soll[i] : std::string{};
        std::string const t = (i < ist.size()) ? ist[i] : std::string{};
        if (s != t) positions_abweichungen.push_back(SchemaAbweichung{i, s, t});
    }

    // (b) Mengen-Vergleich MIT Vielfachheit -- eine Spalte doppelt zu setzen ist ein Zufuegen.
    std::map<std::string, long long> saldo;
    for (auto const& s : soll) ++saldo[s];
    for (auto const& s : ist) --saldo[s];
    for (auto const& [name, d] : saldo) {
        for (long long k = 0; k < d; ++k) b.entfernt.push_back(name);   // im SOLL ueberzaehlig
        for (long long k = 0; k < -d; ++k) b.zugefuegt.push_back(name); // im IST ueberzaehlig
    }

    // (c) Reines Umordnen liegt nur vor, wenn die Namens-Menge deckungsgleich ist.
    if (b.zugefuegt.empty() && b.entfernt.empty()) b.umgeordnet = positions_abweichungen;

    if (!positions_abweichungen.empty()) {
        b.hat_erste_abweichung = true;
        b.erste_abweichung     = positions_abweichungen.front();
    }
    b.identisch = positions_abweichungen.empty() && b.zugefuegt.empty() && b.entfernt.empty();
    return b;
}

/// Bequemlichkeits-Ueberladung: nimmt die Kopfzeile direkt und zerlegt sie selbst.
[[nodiscard]] inline SchemaFreezeBefund pruefe_schema_freeze(std::string_view kopfzeile, char trenner,
                                                             std::span<std::string const> soll) {
    std::vector<std::string> const ist = spalten_aus_kopfzeile(kopfzeile, trenner);
    return pruefe_schema_freeze(ist, soll);
}

/// Der BERICHT der Wache. Beginnt IMMER mit beiden Nennern -- auch im gruenen Fall, damit ein
/// "ok" ohne Zahl gar nicht erst entstehen kann.
[[nodiscard]] inline std::string schema_freeze_bericht(SchemaFreezeBefund const& b, std::string_view schema_name) {
    std::string r = "SCHEMA-FREEZE ";
    r += schema_name;
    r += ": NENNER soll=";
    r += std::to_string(b.soll_spalten);
    r += " ist=";
    r += std::to_string(b.ist_spalten);
    r += b.identisch ? " -> UNVERAENDERT\n" : " -> GEAENDERT\n";
    if (b.identisch) return r;

    if (!b.zugefuegt.empty()) {
        r += "  ZUGEFUEGT (" + std::to_string(b.zugefuegt.size()) + "): ";
        for (std::size_t i = 0; i < b.zugefuegt.size(); ++i) {
            if (i != 0) r += ", ";
            r += b.zugefuegt[i];
        }
        r += '\n';
    }
    if (!b.entfernt.empty()) {
        r += "  ENTFERNT (" + std::to_string(b.entfernt.size()) + "): ";
        for (std::size_t i = 0; i < b.entfernt.size(); ++i) {
            if (i != 0) r += ", ";
            r += b.entfernt[i];
        }
        r += '\n';
    }
    if (!b.umgeordnet.empty()) {
        r += "  UMGEORDNET (" + std::to_string(b.umgeordnet.size()) + " Positionen, gleiche Namensmenge):\n";
        for (auto const& a : b.umgeordnet) {
            r += "    Position " + std::to_string(a.position) + ": soll=\"" + a.soll + "\" ist=\"" + a.ist + "\"\n";
        }
    }
    if (b.hat_erste_abweichung) {
        r += "  ERSTE ABWEICHUNG: Position " + std::to_string(b.erste_abweichung.position) + ": soll=\"" +
             b.erste_abweichung.soll + "\" ist=\"" + b.erste_abweichung.ist + "\"\n";
    }
    r += "  ERLAUBT, ABER LAUT: wer das Schema absichtlich aendert, zieht die eingefrorene Liste in\n"
         "  cache_engine/measurement/schema_freeze.hpp im SELBEN Commit nach.\n";
    return r;
}

/// Kleine Brueckenform fuer Aufrufer, die eine constexpr-Liste vorlegen wollen.
template <std::size_t N>
[[nodiscard]] inline std::vector<std::string> als_spaltenliste(std::array<std::string_view, N> const& a) {
    std::vector<std::string> out;
    out.reserve(N);
    for (auto const& s : a) out.emplace_back(s);
    return out;
}

// =================================================================================================
// 2. DIE EINGEFRORENEN LISTEN -- STUFE 1, gegossen am 2026-08-09
// =================================================================================================

/// (1) WIDE / E4-Mess-CSV, Trenner ';' -- Erzeuger: lazy_csv_header()
/// (builder/experiment_tree/cache_engine_builder_iterator.hpp:504).
///
/// Diese 189 Namen sind am 09.08.2026 aus der laufenden Binary abgeschrieben, nicht aus dem Quelltext
/// gelesen: der Kopf entsteht erst zur Laufzeit aus vier Single-Sources (kOpKindNames,
/// kCompositionAxisNames, kV3AxisSchema und den End-Appends). Genau deshalb ist eine Liste noetig --
/// den Nenner sieht man dem Quelltext nicht an.
inline constexpr std::array<std::string_view, 189> kWideSchemaFreezeStufe1{"binary_id",
                                                                           "setting",
                                                                           "repetition",
                                                                           "n_ops",
                                                                           "total_ns",
                                                                           "ns_per_op",
                                                                           "op_insert_n",
                                                                           "op_insert_p50_ns",
                                                                           "op_insert_p99_ns",
                                                                           "op_lookup_n",
                                                                           "op_lookup_p50_ns",
                                                                           "op_lookup_p99_ns",
                                                                           "op_erase_n",
                                                                           "op_erase_p50_ns",
                                                                           "op_erase_p99_ns",
                                                                           "op_clear_n",
                                                                           "op_clear_p50_ns",
                                                                           "op_clear_p99_ns",
                                                                           "op_scan_n",
                                                                           "op_scan_p50_ns",
                                                                           "op_scan_p99_ns",
                                                                           "op_rmw_n",
                                                                           "op_rmw_p50_ns",
                                                                           "op_rmw_p99_ns",
                                                                           "seg_search_algo_ns",
                                                                           "seg_cache_traversal_ns",
                                                                           "seg_mapping_ns",
                                                                           "seg_path_compression_ns",
                                                                           "seg_node_type_ns",
                                                                           "seg_memory_layout_ns",
                                                                           "seg_allocator_ns",
                                                                           "seg_prefetch_ns",
                                                                           "seg_concurrency_ns",
                                                                           "seg_serialization_ns",
                                                                           "seg_value_handle_ns",
                                                                           "seg_index_organization_ns",
                                                                           "seg_io_dispatch_ns",
                                                                           "seg_migration_policy_ns",
                                                                           "seg_filter_ns",
                                                                           "seg_queuing_q1_ns",
                                                                           "seg_queuing_q2_ns",
                                                                           "seg_persistence_target_ns",
                                                                           "seg_framework_ns",
                                                                           "seg_run_total_ns",
                                                                           "seg_coverage",
                                                                           "search_lookup",
                                                                           "hit",
                                                                           "miss",
                                                                           "insert",
                                                                           "erase",
                                                                           "peak",
                                                                           "bytes_alloc",
                                                                           "bytes_in_use",
                                                                           "alloc_cnt",
                                                                           "dealloc_cnt",
                                                                           "fail",
                                                                           "obs_axes",
                                                                           "fill",
                                                                           "applied_axes",
                                                                           "stat_search_algo_lookup",
                                                                           "stat_search_algo_hit",
                                                                           "stat_search_algo_miss",
                                                                           "stat_search_algo_insert",
                                                                           "stat_search_algo_erase",
                                                                           "stat_search_algo_peak",
                                                                           "stat_cache_traversal_resolve",
                                                                           "stat_cache_traversal_resolve_hit",
                                                                           "stat_cache_traversal_resolve_miss",
                                                                           "stat_cache_traversal_register",
                                                                           "stat_cache_traversal_unregister",
                                                                           "stat_cache_traversal_peak_tracked",
                                                                           "stat_cache_traversal_batch_size",
                                                                           "stat_cache_traversal_batch_visited",
                                                                           "stat_mapping_register",
                                                                           "stat_mapping_resolve",
                                                                           "stat_mapping_resolve_hit",
                                                                           "stat_mapping_resolve_miss",
                                                                           "stat_mapping_reverse_lookup",
                                                                           "stat_mapping_peak_mapped",
                                                                           "stat_mapping_indirect_steps",
                                                                           "stat_path_compression_compress",
                                                                           "stat_path_compression_prefix_len",
                                                                           "stat_path_compression_bytes_saved",
                                                                           "stat_path_compression_cuts",
                                                                           "stat_path_compression_checksum",
                                                                           "stat_node_type_find",
                                                                           "stat_node_type_keys_stored",
                                                                           "stat_node_type_queries",
                                                                           "stat_node_type_checksum",
                                                                           "stat_memory_layout_scan",
                                                                           "stat_memory_layout_records",
                                                                           "stat_memory_layout_field_bytes",
                                                                           "stat_memory_layout_cache_lines",
                                                                           "stat_memory_layout_checksum",
                                                                           "stat_memory_layout_line_bytes",
                                                                           "stat_allocator_bytes_alloc",
                                                                           "stat_allocator_bytes_in_use",
                                                                           "stat_allocator_alloc_cnt",
                                                                           "stat_allocator_dealloc_cnt",
                                                                           "stat_allocator_fail",
                                                                           "stat_allocator_budget_reject",
                                                                           "stat_prefetch_trigger",
                                                                           "stat_prefetch_suggestions",
                                                                           "stat_prefetch_hot_path_hints",
                                                                           "stat_prefetch_max_queue_depth",
                                                                           "stat_prefetch_addrs_enqueued",
                                                                           "stat_concurrency_acquire",
                                                                           "stat_concurrency_release",
                                                                           "stat_concurrency_contention",
                                                                           "stat_concurrency_validation_fail",
                                                                           "stat_concurrency_pattern_id",
                                                                           "stat_serialization_serialize",
                                                                           "stat_serialization_records",
                                                                           "stat_serialization_bytes",
                                                                           "stat_serialization_checksum",
                                                                           "stat_value_handle_access",
                                                                           "stat_value_handle_indirect_deref",
                                                                           "stat_value_handle_version_strips",
                                                                           "stat_value_handle_peak_chain_depth",
                                                                           "stat_index_organization_scan",
                                                                           "stat_index_organization_records",
                                                                           "stat_index_organization_predicate_evals",
                                                                           "stat_index_organization_indirect_lookups",
                                                                           "stat_index_organization_checksum",
                                                                           "stat_io_dispatch_rounds",
                                                                           "stat_io_dispatch_bytes",
                                                                           "stat_io_dispatch_align_adjusts",
                                                                           "stat_io_dispatch_dispatch_cnt",
                                                                           "stat_io_dispatch_checksum",
                                                                           "stat_migration_policy_decisions",
                                                                           "stat_migration_policy_migrations",
                                                                           "stat_migration_policy_hot_votes",
                                                                           "stat_migration_policy_cold_votes",
                                                                           "stat_migration_policy_tier_moves",
                                                                           "stat_filter_probe",
                                                                           "stat_filter_pos",
                                                                           "stat_filter_neg",
                                                                           "stat_filter_hash_probes",
                                                                           "stat_filter_checksum",
                                                                           "stat_queuing_q1_put",
                                                                           "stat_queuing_q1_get",
                                                                           "stat_queuing_q1_overflow",
                                                                           "stat_queuing_q1_underflow",
                                                                           "stat_queuing_q1_peak_size",
                                                                           "stat_queuing_q2_decisions",
                                                                           "stat_queuing_q2_full_flush",
                                                                           "stat_queuing_q2_partial_flush",
                                                                           "stat_queuing_q2_no_flush",
                                                                           "stat_queuing_q2_flush_complete",
                                                                           "stat_persistence_target_rounds",
                                                                           "stat_persistence_target_bytes_staged",
                                                                           "stat_persistence_target_records_staged",
                                                                           "stat_persistence_target_device_flushes",
                                                                           "stat_persistence_target_checksum",
                                                                           "v3_filled_axes",
                                                                           "workload",
                                                                           "two_phase_valid",
                                                                           "series",
                                                                           "sweep_axis",
                                                                           "working_set_n",
                                                                           "platform",
                                                                           "build_version",
                                                                           "pruefling_type",
                                                                           "quality_flag",
                                                                           "pmc_cache_misses_l1",
                                                                           "pmc_cache_misses_l2",
                                                                           "pmc_cache_misses_l3",
                                                                           "pmc_dtlb_misses",
                                                                           "pmc_coherence_invalidations",
                                                                           "pmc_energy_micro_joules",
                                                                           "pmc_available",
                                                                           "container_store_ops",
                                                                           "fairness_mode",
                                                                           "h2_code_quality_score",
                                                                           "pmc_branch_misses",
                                                                           "op_insert_p999_ns",
                                                                           "op_lookup_p999_ns",
                                                                           "op_erase_p999_ns",
                                                                           "op_clear_p999_ns",
                                                                           "op_scan_p999_ns",
                                                                           "op_rmw_p999_ns",
                                                                           "alloc_bytes_in_use_peak",
                                                                           "alloc_external_frag_milli",
                                                                           "alloc_internal_frag_milli",
                                                                           "pmc_l3_miss_uncore_systemweit",
                                                                           "drift_reps",
                                                                           "drift_reruns",
                                                                           "drift_relative",
                                                                           "drift_status"};

/// (2) PIPELINE-CSV, Trenner ',' -- Erzeuger: pipeline16_csv_header()
/// (measurement/pipeline_csv_schema.hpp). BEWUSST als eigenes Literal geschrieben und NICHT als
/// `= kPipeline16Spalten` -- ein Alias waere die blinde Wache aus dem Kopfkommentar.
inline constexpr std::array<std::string_view, 16> kPipeline16FreezeStufe1{"permutation_id",
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

/// (2b) Die Zusatzspalten der vollen Pipeline-Sicht -- ebenfalls eigenstaendiges Literal.
/// NACHGEZOGEN 19.08.2026 (NP-23, #15-Bruch, im SELBEN Commit wie die Erzeuger-Liste): +7 PMC-
/// Quell-Flag-Spalten am Ende (die dokumentierte laute Schema-Aenderung, s. Kopf dieser Datei).
inline constexpr std::array<std::string_view, 16> kPipelineVollZusatzFreezeStufe1{
    "search_insert",
    "search_lookup",
    "search_hit",
    "search_miss",
    "search_erase",
    "search_peak_occupancy",
    "pmc_available",
    "branch_misses",
    "throughput_ops_per_sec",
    "cache_misses_l1_source_available",
    "cache_misses_l2_source_available",
    "cache_misses_l3_source_available",
    "dtlb_misses_source_available",
    "coherence_invalidations_source_available",
    "energy_micro_joules_source_available",
    "branch_misses_source_available"};

/// Die NENNER als benannte Konstanten -- damit ein Aufrufer die Zahl zitieren kann, ohne sie zu raten.
inline constexpr std::size_t kWideSchemaFreezeSpalten = kWideSchemaFreezeStufe1.size();
inline constexpr std::size_t kPipeline16FreezeSpalten = kPipeline16FreezeStufe1.size();
inline constexpr std::size_t kPipelineVollFreezeSpalten =
    kPipeline16FreezeStufe1.size() + kPipelineVollZusatzFreezeStufe1.size();

static_assert(kWideSchemaFreezeSpalten == 189, "WIDE-Mess-Schema, Stufe-1-Freeze vom 2026-08-09: 189 Spalten.");
static_assert(kPipeline16FreezeSpalten == 16, "Pipeline-Schema, Stufe-1-Freeze: 16 Spalten.");
static_assert(kPipelineVollFreezeSpalten == 32, "Volle Pipeline-Sicht: 25 (Stufe 1) + 7 NP-23-Quell-Flags.");

/// Die volle eingefrorene Pipeline-Sicht, abgeleitet aus den beiden Teilen (Praefix-Superset).
inline constexpr std::array<std::string_view, kPipelineVollFreezeSpalten> kPipelineVollFreezeStufe1 = [] {
    std::array<std::string_view, kPipelineVollFreezeSpalten> a{};
    std::size_t                                              i = 0;
    for (auto const& s : kPipeline16FreezeStufe1) a[i++] = s;
    for (auto const& s : kPipelineVollZusatzFreezeStufe1) a[i++] = s;
    return a;
}();

} // namespace comdare::cache_engine::measurement
