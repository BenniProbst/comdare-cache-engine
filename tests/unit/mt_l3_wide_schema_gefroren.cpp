// mt_l3_wide_schema_gefroren.cpp -- MT-L3, DIE EINGEFRORENE SEITE (2026-08-10).
//
// DIESE UEBERSETZUNGSEINHEIT IST DAS ORAKEL. Sie traegt den Kopf der grossen Produktions-Mess-CSV
// EINMAL als eingefrorenes Byte-Literal. Sie prueft nichts, sie rechnet nichts, sie ruft nichts --
// sie ERINNERT. Geprueft wird nebenan, in test_mt_l3_wide_schema_orakel.cpp.
//
// WARUM DAS EINE EIGENE .cpp IST UND NICHT EIN BLOCK IM TEST:
// Ein Orakel taugt genau so lange, wie niemand es aus dem Pruefling ableiten kann. Die Versuchung
// heisst `auto soll = lazy_csv_header();` -- eine Zeile, und die Wache prueft eine Sache gegen sich
// selbst: immer gruen, von aussen nicht von einer scharfen Wache zu unterscheiden, und deckt nichts.
// Diese Datei macht diese Zeile UNMOEGLICH statt sie nur zu verbieten: die Include-Liste unten ist
// vollstaendig, sie besteht aus zwei Standard-Headern, und der Erzeuger
// (builder/experiment_tree/cache_engine_builder_iterator.hpp) ist hier weder direkt noch mittelbar
// sichtbar. Wer das Orakel selbstreferenziell machen wollte, muesste erst diese Include-Liste
// aendern -- und das ist eine sichtbare, begruendungspflichtige Aenderung im Diff.
//
// WIE DAS LITERAL NACHZUZIEHEN IST, WENN EINE SPALTE ABSICHTLICH DAZUKOMMT:
// Eine Schema-Aenderung ist ERLAUBT -- aber LAUT. Der Ablauf ist bewusst kurz gehalten, damit
// niemand in Versuchung kommt, statt nachzuziehen das Orakel zu umgehen:
//   1. Die Spalte am Erzeuger lazy_csv_header() ergaenzen (nur ANHAENGEN -- jede Einfuegung in der
//      Mitte verschiebt Positionen und bricht jeden Bestands-Leser der alten CSV).
//   2. `ctest -R test_mt_l3_wide_schema_orakel --output-on-failure` fahren. Der Test faellt und
//      DRUCKT den vollstaendigen neuen Kopf bereits als fertigen C++-Block aus ("NEUES LITERAL,
//      einfuegefertig"). Dieser Block ersetzt kGefrorenerWideKopf unten -- abschreiben von Hand ist
//      nicht noetig und ausdruecklich nicht erwuenscht.
//   3. Im SELBEN Commit auch kWideSchemaFreezeStufe1 in
//      cache_engine/measurement/schema_freeze.hpp nachziehen. Der Test haelt die beiden
//      eingefrorenen Listen auch GEGENEINANDER: wer nur eine nachzieht, bleibt rot. Das ist die
//      Schwesterstelle (T-6), und sie ist als Werkzeug verdrahtet und nicht als gute Absicht.
//   4. Die Zahl im static_assert unten mitziehen. Sie ist die Zusicherung "ich habe die Spaltenzahl
//      BEWUSST geaendert" -- ohne sie waere ein versehentliches Anhaengen von einem gewollten nicht
//      zu unterscheiden.
//
// DER GEGENSTAND, praezise: das WIDE/E4-Mess-Schema, Trenner ';', Erzeuger lazy_csv_header() in
// builder/experiment_tree/cache_engine_builder_iterator.hpp. NICHT der f15-Export
// (result_csv_header() in builder/commands/result_aggregator.hpp) -- das ist ein anderer Datentyp,
// ein anderer Konsument und ein anderes Schema.
//
// DOKTRIN: C++23, ASCII-only.
//
// SELBSTCHECK: Wenn diese Datei jemals etwas anderes als <cstddef>/<string_view> inkludiert, ist das
// Orakel kaputt -- nicht der Test.

#include <cstddef>
#include <string_view>

namespace comdare::mt_l3 {

// Die Zusicherungen dieser Uebersetzungseinheit nach aussen. Sie stehen hier -- und nicht in einem
// Header, den beide Seiten teilen -- weil ein gemeinsamer Header ein Weg waere, ueber den der
// Erzeuger doch noch in das Orakel gelangen koennte. Der Test wiederholt genau diese drei Zeilen;
// weichen sie voneinander ab, bricht der Binder, nicht erst die Auswertung.
std::string_view gefrorener_wide_kopf() noexcept;
std::size_t      gefrorener_wide_nenner() noexcept;
char             gefrorener_wide_trenner() noexcept;

/// DER EINGEFRORENE KOPF, byte-genau so, wie ihn lazy_csv_header() am 2026-08-10 zurueckgab.
///
/// Byte-genau und nicht als Namensliste: eine Namensliste verliert den Trenner, verliert das Fehlen
/// eines abschliessenden Trenners und verliert jedes Leerzeichen. Alle drei sind Schema-Aenderungen
/// mit Bestands-Folgen, und alle drei wuerden an einer reinen Namensliste vorbeilaufen.
inline constexpr std::string_view kGefrorenerWideKopf =
    "binary_id;setting;repetition;n_ops;total_ns;ns_per_op;op_insert_n;op_insert_p50_ns;op_insert_p99_ns;"
    "op_lookup_n;op_lookup_p50_ns;op_lookup_p99_ns;op_erase_n;op_erase_p50_ns;op_erase_p99_ns;op_clear_n;"
    "op_clear_p50_ns;op_clear_p99_ns;op_scan_n;op_scan_p50_ns;op_scan_p99_ns;op_rmw_n;op_rmw_p50_ns;"
    "op_rmw_p99_ns;seg_search_algo_ns;seg_cache_traversal_ns;seg_mapping_ns;seg_path_compression_ns;"
    "seg_node_type_ns;seg_memory_layout_ns;seg_allocator_ns;seg_prefetch_ns;seg_concurrency_ns;"
    "seg_serialization_ns;seg_value_handle_ns;seg_index_organization_ns;seg_io_dispatch_ns;"
    "seg_migration_policy_ns;seg_filter_ns;seg_queuing_q1_ns;seg_queuing_q2_ns;seg_persistence_target_ns;"
    "seg_framework_ns;seg_run_total_ns;seg_coverage;search_lookup;hit;miss;insert;erase;peak;bytes_alloc;"
    "bytes_in_use;alloc_cnt;dealloc_cnt;fail;obs_axes;fill;applied_axes;stat_search_algo_lookup;"
    "stat_search_algo_hit;stat_search_algo_miss;stat_search_algo_insert;stat_search_algo_erase;"
    "stat_search_algo_peak;stat_cache_traversal_resolve;stat_cache_traversal_resolve_hit;"
    "stat_cache_traversal_resolve_miss;stat_cache_traversal_register;stat_cache_traversal_unregister;"
    "stat_cache_traversal_peak_tracked;stat_cache_traversal_batch_size;"
    "stat_cache_traversal_batch_visited;stat_mapping_register;stat_mapping_resolve;"
    "stat_mapping_resolve_hit;stat_mapping_resolve_miss;stat_mapping_reverse_lookup;"
    "stat_mapping_peak_mapped;stat_mapping_indirect_steps;stat_path_compression_compress;"
    "stat_path_compression_prefix_len;stat_path_compression_bytes_saved;stat_path_compression_cuts;"
    "stat_path_compression_checksum;stat_node_type_find;stat_node_type_keys_stored;"
    "stat_node_type_queries;stat_node_type_checksum;stat_memory_layout_scan;stat_memory_layout_records;"
    "stat_memory_layout_field_bytes;stat_memory_layout_cache_lines;stat_memory_layout_checksum;"
    "stat_memory_layout_line_bytes;stat_allocator_bytes_alloc;stat_allocator_bytes_in_use;"
    "stat_allocator_alloc_cnt;stat_allocator_dealloc_cnt;stat_allocator_fail;"
    "stat_allocator_budget_reject;stat_prefetch_trigger;stat_prefetch_suggestions;"
    "stat_prefetch_hot_path_hints;stat_prefetch_max_queue_depth;stat_prefetch_addrs_enqueued;"
    "stat_concurrency_acquire;stat_concurrency_release;stat_concurrency_contention;"
    "stat_concurrency_validation_fail;stat_concurrency_pattern_id;stat_serialization_serialize;"
    "stat_serialization_records;stat_serialization_bytes;stat_serialization_checksum;"
    "stat_value_handle_access;stat_value_handle_indirect_deref;stat_value_handle_version_strips;"
    "stat_value_handle_peak_chain_depth;stat_index_organization_scan;stat_index_organization_records;"
    "stat_index_organization_predicate_evals;stat_index_organization_indirect_lookups;"
    "stat_index_organization_checksum;stat_io_dispatch_rounds;stat_io_dispatch_bytes;"
    "stat_io_dispatch_align_adjusts;stat_io_dispatch_dispatch_cnt;stat_io_dispatch_checksum;"
    "stat_migration_policy_decisions;stat_migration_policy_migrations;stat_migration_policy_hot_votes;"
    "stat_migration_policy_cold_votes;stat_migration_policy_tier_moves;stat_filter_probe;stat_filter_pos;"
    "stat_filter_neg;stat_filter_hash_probes;stat_filter_checksum;stat_queuing_q1_put;"
    "stat_queuing_q1_get;stat_queuing_q1_overflow;stat_queuing_q1_underflow;stat_queuing_q1_peak_size;"
    "stat_queuing_q2_decisions;stat_queuing_q2_full_flush;stat_queuing_q2_partial_flush;"
    "stat_queuing_q2_no_flush;stat_queuing_q2_flush_complete;stat_persistence_target_rounds;"
    "stat_persistence_target_bytes_staged;stat_persistence_target_records_staged;"
    "stat_persistence_target_device_flushes;stat_persistence_target_checksum;v3_filled_axes;workload;"
    "two_phase_valid;series;sweep_axis;working_set_n;platform;build_version;pruefling_type;quality_flag;"
    "pmc_cache_misses_l1;pmc_cache_misses_l2;pmc_cache_misses_l3;pmc_dtlb_misses;"
    "pmc_coherence_invalidations;pmc_energy_micro_joules;pmc_available;container_store_ops;fairness_mode;"
    "h2_code_quality_score;pmc_branch_misses;op_insert_p999_ns;op_lookup_p999_ns;op_erase_p999_ns;"
    "op_clear_p999_ns;op_scan_p999_ns;op_rmw_p999_ns;alloc_bytes_in_use_peak;alloc_external_frag_milli;"
    "alloc_internal_frag_milli;pmc_l3_miss_uncore_systemweit;drift_reps;drift_reruns;drift_relative;"
    "drift_status\n";

/// Der eingefrorene NENNER. Steht als eigene Zahl da, damit ein Aufrufer sie zitieren kann, ohne sie
/// aus dem Literal nachzuzaehlen -- und damit ein versehentliches Anhaengen auffaellt.
inline constexpr std::size_t kGefrorenerWideSpalten = 189;

/// Der Trenner. Ein Wechsel des Trenners ist eine Schema-Aenderung wie jede andere.
inline constexpr char kGefrorenerWideTrenner = ';';

// Die Selbstzusicherung dieser Datei: das Literal ist nicht leer und traegt den Trenner. Das faengt
// nicht die Schema-Drift (dafuer ist der Test da), wohl aber ein versehentlich geleertes Orakel --
// ein leeres Orakel waere gegen ein leeres Ergebnis gruen.
static_assert(!kGefrorenerWideKopf.empty(), "MT-L3: das eingefrorene Literal darf nie leer sein.");
static_assert(kGefrorenerWideKopf.find(kGefrorenerWideTrenner) != std::string_view::npos,
              "MT-L3: das eingefrorene Literal muss den Trenner tragen.");

std::string_view gefrorener_wide_kopf() noexcept { return kGefrorenerWideKopf; }
std::size_t      gefrorener_wide_nenner() noexcept { return kGefrorenerWideSpalten; }
char             gefrorener_wide_trenner() noexcept { return kGefrorenerWideTrenner; }

} // namespace comdare::mt_l3
