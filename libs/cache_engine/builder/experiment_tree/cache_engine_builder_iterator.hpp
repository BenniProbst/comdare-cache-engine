#pragma once
// L-LAZY-E2E (gate-frei, 2026-06-03) — cache_engine_builder_iterator: DIE EINE Host-Treiber-Funktion, die den
// Experiment-B+-Baum (Permutations-/Präfixbaum, kein textbook-B+-Baum — s. experiment_tree.hpp:2-10) END-TO-END LAZY durchläuft: erst statische Kompilierung (Tier-Binary-DLLs), dann dynamische
// Variablen-Variation, messen, ingest. Verdrahtet die schon EINZELN verifizierten Bausteine zu EINER Kette —
// baut NICHTS Bestehendes um.
//
// Die 3 EXPLIZITEN, LAZY Iteratoren (alle ohne ∏-Voll-Materialisierung — Doc 26 §2):
//   (1) HAUPT- / STATISCH-ITERATOR  : StaticBinaryView + BuildSelection (erste N Blätter). Je Blatt LAZY `view[i]`
//        → BuildOrchestrator baut die DLL (resumierbar via .version-Sidecar, RAM-gated, multithreaded). = die
//        STATISCHE Kompilierung. Materialisiert nie alle ∏ — nur die K=|selection| Specs (O(K)).
//   (2) je gebaute DLL: AnatomyModuleLoader::load → IAnatomyBase* → via dynamic_cast die zwei ABI-Sub-Interfaces:
//        IObservableTier (Mess-Antrieb, COMDARE_MEASUREMENT_ON) + IResourceControllableTier (Laufzeit-Steuerung).
//   (3) GEFILTERT-DYNAMISCH-ITERATOR : RuntimeVariableLoop.run(tier, tree.dynamic_filter(), visitor) — LAZY über
//        die virtuelle Kartesik des dynamischen Sub-Filterbaums auf der GELADENEN Binary (KEIN Neu-Bauen). Je
//        Setting: tier_apply_resource_control (im Loop) → messen (run_observable_perm-artig) → format_perm_result
//        mit setting-spezifischer ID → ingest_result_line(tree, line) → sparse NodeValue im Baum.
//
// Ergebnis: je (Binary × dyn-Setting) eine GEMESSENE Zeile im Baum (sparse, observer_real=true) + eine CSV-Zeile.
//
// ENGINE-AGNOSTISCH (wie BuildOrchestrator): der reale-Anatomie-Source-Generator (SourceGenFn), der Compiler
// (CompileFn) und die RAM-Abfrage (FreeRamFn) sind INJIZIERT. Dadurch bleibt dieser Header frei vom schweren
// all_axes_umbrella.hpp-Include (Windows-Compiler-OOM, registry_to_axis_levels.hpp §0) und deterministisch testbar
// (Mock-CompileFn). Der PilotEngine-spezifische Pfad→Source-Map-Generator wird host-seitig (Harness-.cpp) gebaut
// und als SourceGenFn übergeben — siehe make_pilot_source_gen / emit_pilot_sources unten (Template, opt-in include).
// Header-only, C++23.

#include "experiment_tree.hpp"         // ExperimentTree / StaticBinaryView / NodeObserverSnapshot
#include "axis_path_serialization.hpp" // (X) kCompositionAxisNames[17] — Single-Source der seg_*-Spaltennamen (INC-2d)
#include "coverage_selection.hpp"      // BuildSelection
#include "runtime_variable_loop.hpp"   // RuntimeVariableLoop / RuntimeSetting (gefiltert-dynamisch)
#include "container_attribution.hpp"   // CMD-2/#252: host-seitige Container-in-SA-Attribution (c1 store_ops)
#include <harness/perm_runner.hpp> // A2-Neben Stufe 2: run_observable_perm / format_perm_result (nach harness/ herausgeloest)
#include <harness/drift_gated_cell.hpp> // T-15: DriftGateConfig + run_cell_with_drift_gate (Zell-Klammer)
#include <cache_engine/measurement/axis_error.hpp> // E-6/K-10: SampleStatus + sample_status_token (n/a-Zell-Renderer)
#include "measure_parallelism.hpp"   // #45: resolve_measure_parallelism (Debug-Methodik -> Mess-Pool; Entry-Konsum)
#include "result_ingest.hpp"         // ingest_result_line / parse_result_line_to_node_value (#45 reine Parse-Naht)
#include "parallel_measure_pool.hpp" // #45 (§16.2-M1/§61-MODI): collect_ordered (Debug-Modus paralleler Mess-Loop)
#include "../build_orchestrator/build_orchestrator.hpp" // BuildOrchestrator / BuildConfig / *Fn
#include "../artifact_transport/artifact_cache.hpp"     // Storage #51: CachePushFn / MeasurementSinkFn (No-Op-Naht)
#include "../artifact_transport/async_push_pump.hpp"    // W11 (§43.c): AsyncPushPump (BAU-Modus async Push-Pump)
#include "../bestandslog/builder_registration.hpp"      // #46b I1: Bestandslog-Registrierung/Dedup (opt-in, No-Op-Naht)
#include "../bestandslog/lager_presence.hpp" // Lager-TP1(B)/G-E2: PresenceFn aus lager_contains + Fingerprint binden
#include "../bestandslog/messwert_registrierung.hpp" // G-E3: der produktive Schreiber des measurement-Genus
#include "../bestandslog/planer_driven_build.hpp" // #46b I1b: Planer-getriebener Slice-Bau (opt-in, SlicePlanner/Queue)
#include "../bestandslog/eta_kalibrierung.hpp"    // A5/F5: laufende Kalibrierung (Schwelle, Median, Re-Kalibrierung)
#include "../bestandslog/reservation_lifecycle.hpp" // #46b I1b: Reservierungs-Lifecycle je Slice (pro-forma/Kalib/Done)
#include "progress_delta.hpp" // Welle 5 (E-W5-2): ProgressDelta / ProgressSinkFn / Delta-Logik (builder-Sibling-Leaf, §38 hinauf)
#include "progress_heartbeat.hpp" // S1 (§62-B Log-Flush): geflushtes Mess-Fortschritts-Testat je Zelle (Befund 6h-stumm)
#include "slice_marker.hpp"       // E-04-P1: Marker-Familie v2 (die EINE Renderer-Quelle des Live-Fortschritts-Kanals)
#include "../anatomy_module_loader/anatomy_module_loader.hpp" // AnatomyModuleLoader / AnatomyModuleHandle
#include "../pruef_dock/search_algorithm_dock.hpp" // INC-2a (Q4): acquire_search_algorithm_drive (Dock-Vertrag)
#include "../pruef_dock/pruef_only.hpp" // S3 (§62-B COMDARE_PRUEF_ONLY): run_so_conformance_gate (Load+Gate, kein Bau/Mess)
#include "../../anatomy/observable_tier.hpp" // IObservableTier
#include "../../anatomy/measurable_workload.hpp" // (X): IMeasurableWorkloadV3 + ComdareSegmentLatencyV2 (17 Segmente, INC-2d)
#include "../../anatomy/resource_controllable_tier.hpp" // IResourceControllableTier

#include <algorithm>                          // #165-B: std::nth_element (Gruppen-Median im quality_flag)
#include <builder/commands/latency_stats.hpp> // D5-1: der EINE Perzentil-Kanon (stats::nearest_rank_index)
#include <array>                              // GOAL-L1: LazyMeasuredRow::op_lat (per-Interface-Funktions-Latenzen)
#include <chrono>                             // #46b I1b: Slice-Wall-Clock fuer die ETA-Kalibrierung
#include <cstddef>
#include <map> // #165-B: Gruppen-Buckets (binary_id|profile_name) im annotate_quality_flags
#include <cstdint>
#include <cstdio>    // (C-1) std::snprintf fuer ns_per_op-Formatierung
#include <exception> // TP1FK1-B2: std::exception -- der werfende Transport wird im Mess-Pfad klassifiziert gefangen
#include <filesystem>
#include <fstream>    // (E) per-Binary-Ergebnis-CSV schreiben
#include <functional> // #46b I1: bestand_key_of-Injektion (std::function)
#include <memory>     // #45: std::unique_ptr<IPmcSource> als per-Worker-ctx im parallelen Mess-Loop
#include <optional>   // #46b I1: bestand_key_of-Rueckgabe (std::optional<std::string>)
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// Storage #51 (No-Op-Naht): die Transport-Injektions-Typen liegen kanonisch im artifact_transport-Modul (haelt den
// achsen-blinden Iterator transport-frei) — hier nur als Alias hochgezogen (ex::CachePushFn / ex::MeasurementSinkFn),
// analog dazu, wie CompileFn/AlgoSigFn im experiment-Namespace sichtbar sind.
using CachePushFn = ::comdare::cache_engine::builder::artifact_transport::CachePushFn;
using CachePullFn = ::comdare::cache_engine::builder::artifact_transport::CachePullFn; // S2 (#46a): Warm-Cache-Pull
using MeasurementSinkFn = ::comdare::cache_engine::builder::artifact_transport::MeasurementSinkFn;
// W11 (§43.c): der Teil-Marker-Sink + der async Push-Pump (BAU-Modus). Wie CachePushFn transport-kanonisch, hier aliast.
using PartialMarkerFn = ::comdare::cache_engine::builder::artifact_transport::PartialMarkerFn;
// #46b I1 (opt-in Bestandslog-Verdrahtung): der Transport-Naht-Typ + die Registrierungs-Zustandsmaschine liegen im
// bestandslog-Modul; hier als Alias hochgezogen (Muster wie CachePushFn -- haelt den Iterator stamp-/transport-frei).
namespace bestandslog  = ::comdare::cache_engine::builder::bestandslog;
using BestandTransport = bestandslog::BestandTransport;
using AsyncPushPump    = ::comdare::cache_engine::builder::artifact_transport::AsyncPushPump;
// Welle 5 (E-W5-2, §38-Rueck-Kanal): ProgressDelta / ProgressSinkFn / progress_delta_between liegen im
// builder-Sibling-Leaf progress_delta.hpp (SELBER Namespace builder::experiment) -> hier ohne Alias direkt
// sichtbar. Der achsen-blinde Iterator inkludiert NUR den builder-Leaf (kein Aufwaerts-Include in profile_facade).

// ── Konfiguration des Lazy-E2E-Laufs ──────────────────────────────────────────
struct LazyRunConfig {
    std::size_t           max_binaries  = 150;         // wie viele statische Blätter (erste N) gebaut+gemessen werden
    std::uint64_t         n_ops         = 1000;        // Mess-Workload je dyn-Setting (insert+lookup)
    std::string           build_version = "v1";        // Resume-Marke (BuildOrchestrator .version-Sidecar)
    std::filesystem::path source_dir;                  // perm_<id>.cpp (Source-Ausgabe)
    std::filesystem::path output_dir;                  // perm_<id>.dll (Build-Ausgabe)
    std::size_t           cores_per_build         = 4; // KF-16b Default (keine Oversubscription)
    std::uint64_t         ram_per_build_bytes     = 0; // 0 = RAM-Gate aus (nur CPU-Cap)
    std::uint64_t         ram_safety_margin_bytes = 0;
    // W6 (2026-07-19, Ledger §32-F7): expliziter Override der parallelen Compile-Worker-Zahl des Bau-Pools
    // (BuildConfig::build_parallelism). 0 = ungesetzt => parallel_jobs()-Heuristik = heutiges byte-neutrales
    // Verhalten. >0 = harte Worker-Zahl. Der Facade-Rand belegt dies aus Env COMDARE_BUILD_PARALLEL. NUR der
    // provision-/Bau-Pfad (STATISCHE Kompilierung) wird davon parallelisiert; die Mess-Schleife bleibt 1-Thread.
    std::size_t build_parallelism = 0;
    // (E): je Tier-Binary ein eigener Unterordner output_dir/<stem>/ (DLL + Source + .obj + .cl.log + .version
    // + per-Binary-Ergebnis-CSV). Default false = altes flaches Verhalten (rückwärtskompatibel, opt-in).
    bool per_binary_subdirs = false;
    // (D, KF-10): Anzahl der Wiederholungen je (Binary×Setting). Wirkt über die repetition-DynamicDim im Baum
    // (repetition-DynamicDim aus dem Profil-<runtime_dynamic>; hier durchgereicht). Default 3 (0 → 1 normalisiert).
    std::uint32_t n_repeats = 3;
    // (C-2): per-Segment-Workload-Parameter (run_workload_segmented). seg_batches=0 → kein Segment-Timing (n/a).
    std::uint64_t seg_ops_per_batch = 4000;   // Operationen je Batch im 18-Segment-Workload (X)
    std::uint64_t seg_batches       = 32;     // gemessene Batches (Warmup verworfen); 0 = Segment-Timing aus
    std::uint64_t seg_seed          = 0xB15u; // deterministischer Seed
    // Achse 2 (INC-3): fester Seed für die Workload-Op-Sequenz-Materialisierung. Hängt NUR vom Profil ab (via
    // profile_by_name), NICHT von Binary/Setting/Rep → dieselbe Workload erzeugt bit-identische Sequenzen über
    // ALLE Binaries (Cross-Binary-Vergleichbarkeit = Spalte des kartesischen Kreuzes). [[feedback_two_phase_warmup_mandatory_validity]]
    std::uint64_t workload_seed = 42u;
    // Achse 2 (INC-3c): YCSB-Load-Phase. Anzahl der VOR der gemessenen Run-Phase befüllten Sätze (records). 0 →
    // records = n_ops. Key-Verteilung des Profils wird auf [1, records] ausgerichtet (read-heavy/scan treffen Keys).
    std::uint64_t workload_records = 0u;
    // Achse 2 (#135): XML-Lastprofil-Registry (id → WorkloadConfig: op-mix/dist/negative_query_pct aus dem XML).
    // Leer → run_workload_perm fällt auf hartcodiertes profile_by_name (env-String) zurück. Befüllt von run_lazy_150
    // via discover_load_profiles(load_profiles/). Die ids sind die Werte der dynamischen Workload-Achse.
    // (run_lazy_150 geloescht 2026-07-11; Befuellt heute von Code/02_messung_driver, E4-XML)
    std::map<std::string, wd::WorkloadConfig> workload_configs{};
    // Laufzeit-Obergrenze (System-Limits) für die dyn-Variation (RuntimeVariableLoop clamp gegen caps∩env).
    anatomy::ComdareResourceControlV1 env_limits{};
    // M-1/D-2 (06.08.2026) -- DIE SOLL-SEITE DES MESS-VERTRAGS CEB <-> TIER-BINARY (LEDGER:3319).
    //
    // Die Mess-Stempel-Zeile, die DIESE CEB in die Tier-Quellen stempelt. Jede geladene Tier-Binary muss sie
    // in ihrer eigenen Deklaration (comdare_anatomy_version_lines()->measurement_line) BYTE-GLEICH tragen,
    // sonst wird sie NICHT gemessen (pruef_dock::pruefe_mess_konsistenz, fail-closed).
    //
    // BELEGT WIRD SIE AUS DERSELBEN FUNKTION, die die Quelle stempelt -- profile_facade
    // measurement_stamp_from_env(), gespeist aus der EINEN Aufloesung resolve_live_measurement_combo_legend
    // (M-1/D-1-Naht). Damit hat der Vertrag keine zweite Wahrheit: SOLL und die eingestempelte Zeile
    // entstehen aus demselben Aufruf, nicht aus zwei Ableitungen, die jemand synchron halten muesste.
    //
    // WARUM SIE HIER STEHT UND NICHT IM BUILDER ABGELEITET WIRD: der Builder ist system- und mess-BLIND
    // (W4-B-Invariante) und kennt COMDARE_MEASUREMENT_COMBO_CT nicht -- das Makro lebt am Fassaden-Rand
    // (comdare_measurement_combo_ct). Die Erwartung muss also von dort HEREINGEREICHT werden.
    //
    // LEER == "die CEB benennt ihre einkompilierte Mess-Achse nicht". Das ist KEIN Freifahrtschein: das Gate
    // klassifiziert es als erwartung_leer und weist ab. Der Produktions-Rand (profile_run_entry::make_cfg)
    // belegt das Feld deshalb immer.
    std::string erwartete_mess_zeile;
    // M-1/H-B (06.08.2026) -- TRAEGT DIE TIER-BINARY DIESES LAUFS DEN OBSERVER (G2/G3)?
    //
    // Seit M-1/D-1 ist die Mess-Achse wirksam: eine [wallclock]-Binary wird OHNE
    // COMDARE_CE_ENABLE_STATISTICS uebersetzt. tier_observe() hat seinen KOMPLETTEN Rumpf unter genau diesem
    // Gate (anatomy/abi_adapter.hpp) und liefert dann einen LEEREN Snapshot. Gemessen, beide echten .so ueber
    // dlopen, gleicher Treiber, gleicher Lauf:
    //     [all]        IObservableTier=JA  insert_ok=256 lookup_ok=256  observable_axes=9  fill_level=256
    //     [wallclock]  IObservableTier=JA  insert_ok=256 lookup_ok=256  observable_axes=0  fill_level=0
    // Das Tier haelt in BEIDEN Faellen real 256 Eintraege -- und meldet unter [wallclock] fill_level=0.
    // tier_fill_level und observable_axis_count sind dabei ausdruecklich KEIN Mess-Zustand
    // (axis_operability_classification.hpp: "passive Build-/Compile-Konstante"); sie werden nur deshalb 0, weil
    // fill_observer_v3 gar nicht erst gerufen wird.
    //
    // WARUM DAS OHNE DIESES FELD EINE LUEGE IN DEN MESSDATEN WAERE: perm_runner setzt unified_real
    // BEDINGUNGSLOS auf true, und eine [wallclock]-Binary TRAEGT das Mess-Interface (IObservableTier=JA) --
    // die vorhandene, ehrliche n/a-Alternative unten war fuer genau diesen neuen Fall unerreichbar. Eine
    // [wallclock]-Zeile waere von einer echten Messung eines leeren Tiers mit lauter Null-Zaehlern nicht
    // unterscheidbar gewesen. Der Zustand war vor D-1 NICHT erreichbar (keine Tier-Binary ohne STATISTICS war
    // baubar) -- diese Scheibe hat die Gefahr erzeugt und schliesst sie hier.
    //
    // QUELLE: profile_facade::live_mess_observer_ausstattung() -- DIESELBE Aufloesung und DIESELBE Abbildung,
    // aus der der Bau sein -DCOMDARE_CE_ENABLE_STATISTICS zieht und der Stempel sein Glied [3] rendert. Kein
    // zweiter Parser, keine zweite Wahrheit. Weil pruefe_mess_konsistenz (D-2) VOR der Messung erzwingt, dass
    // die Zeile des GELADENEN Moduls byte-gleich zu erwartete_mess_zeile ist, ist die aus der CEB-Seite
    // abgeleitete Ausstattung nachweislich die des Moduls.
    //
    // DEFAULT true == IDENTITAET fuer den gesamten Bestand: jeder [all]-Lauf traegt den Observer, und jeder
    // Bestands-Lauf ist ein [all]-Lauf (Sidecar-Bestand 0). Der Wert kann nur ABWERTEN, nie aufwerten.
    bool mess_observer_ausstattung = true;
    // M3v2-SELEKTION (2026-06-18, Task #156): Lauf-weite Tags je Mess-Zeile, damit die Auswertung die drei
    // Mess-Klassen (Basis-320 / Per-Achsen-Sweep / SOTA-Reihen A/B/C) UND die Working-Set-N-Dimension UND die
    // Plattform/Build-Version trennen kann. NUR Metadaten (kein Mess-Einfluss) — sie reisen rein über die
    // LazyMeasuredRow/CSV-Tag-Spalten (series/sweep_axis/working_set_n/platform/build_version), NICHT in die binary_id
    // (die binary_id bleibt die reine Achsen-Rekombination — keine Tag-Verschmutzung der Round-Trip-Identität).
    // Quelle: das Diplomarbeit-Mess-Profil (profile_run_entry/run_profile setzt diese 5 Felder je Basis-/Sweep-/SOTA-Pass).
    std::string row_series = "-"; // SOTA-Reihe ∈ {A,B,C} bzw. "-" (Basis/Sweep, keine Reihe)
    // #171 (2026-06-20): die Pruefling-Auspraegung "full" (Reihe A self-contained / Originalkonfiguration) vs
    // "abstract" (Reihe B/C Teilmenge + Host-Fallback) — abgeleitet aus merge (sota_catalog::derive_pruefling_type),
    // getaggt je SOTA-Pass; "-" fuer Basis/Sweep/cowfix-v1 (kein Pruefling). REINE Metadaten (kein Mess-Einfluss),
    // reist wie series rein in der CSV-Tag-Spalte — NICHT in die binary_id (binary_id-Drift vermieden).
    std::string row_pruefling_type = "-";          // "full" / "abstract" / "-" (Basis/Sweep)
    std::string row_sweep_axis     = "-";          // gesweepte Achse (z.B. "migration_policy") bzw. "-" (Basis/SOTA)
    std::string row_platform       = "win-x86_64"; // Plattform-Tag (Infra-Agent überschreibt für ZIH-Reihen)
    std::string row_build_version  = "m3v2";       // Build-Version-Tag (= BuildVersion-Marke; Default m3v2)
    // GO-5 Fork 6 (2026-07-12, Thesis §sec:fairness): der Fairness-Modus der SOTA-Reihe dieses Passes —
    // "common_denominator" / "native" / "-" (Basis/Sweep/ungesetzt). REINE Metadaten wie series/pruefling_type
    // (kein Mess-Einfluss heute; die common_denominator-Kompositions-Pinnung ist DATEN-gated), reist in der
    // CSV-Tag-Spalte fairness_mode + im Resume-Stamp — NICHT in der binary_id (keine Tag-Verschmutzung).
    std::string row_fairness_mode = "-"; // "common_denominator" / "native" / "-"
    // GO-5 Fork 7 (2026-07-12, Thesis-Hypothese H2): der TOOL-BERECHNETE Code-Qualitaets-Score der
    // SOTA-Reihe dieses Passes (aus der Akte sota_h2_scores.xml via h2_score_for; Formel-Single-Source
    // profile_facade/h2_score_akte.hpp). "-" = Basis/Sweep (keine Reihe); "n/a" = SOTA-Reihe ohne
    // Akten-Eintrag (prt_art/fehlende Akte — honest, NIE 0). REINE Metadaten wie fairness_mode (kein
    // Mess-Einfluss, CSV-Tag-Spalte, NICHT in der binary_id). BEWUSST NICHT im Resume-Stamp: der Score
    // ist eine abgeleitete Eigenschaft des Lebewesens (kein Lauf-Konfigurations-Freiheitsgrad — dieselbe
    // binary_id kann nie mit zwei Scores kollidieren); Schema-Drift faengt ohnehin der Header-Vergleich.
    std::string row_h2_score = "-"; // "%.3f"-Dichte / "n/a" / "-"
    // GO-5 Fork 1 (2026-07-12): die deterministische <datasets>-Deklarations-Signatur des Profils
    // (profile_datasets_signature; leer = keine deklariert = heutiges Verhalten). Geht in den Resume-Stamp:
    // eine geaenderte Dataset-Deklaration macht alte per-Binary-Staende konservativ NICHT resume-faehig
    // (ehrliche Neu-Messung; der Loader-MESS-Konsum selbst ist lauf-gated, s. profile_runner.hpp).
    std::string profile_datasets;
    // working_set_n je Zeile = cfg.workload_records (der N-Sweep ruft den Treiber je N-Wert mit gesetztem records).
    // Mess-RESUME (#139, User 2026-06-10 „Wiedereinstieg bei einem bestimmten nicht fertigen Tier"): Binaries,
    // deren per-Binary result.csv VOLLSTÄNDIG + KONFIGURATIONS-AKTUELL ist (result.csv.stamp == aktueller
    // Config-Stempel: build_version/n_ops/seed/records/ALLE dyn-Dimensionen inkl. Workload-Set + Zeilenzahl;
    // Schema via Header-Identität), werden ÜBERSPRUNGEN — ihre Zeilen fließen unverändert in die globale CSV
    // (LazyRunResult::resumed_csv_rows). Unfertige/stale (z.B. anderer n_ops-Testlauf, andere BuildVersion)
    // werden NEU gemessen — der Zwei-Phasen-Cache-Warmup gilt auf Re-Entry intrinsisch je Op (Mess-Gültigkeit,
    // [[feedback_two_phase_warmup_mandatory_validity]]). Nur wirksam mit per_binary_subdirs.
    bool resume_completed_binaries = true;
    // INC-G6 (Ledger 33/34/35, 2026-07-19, BAUPLAN Abschnitt 2): NUR bauen (DLLs provisionieren), NICHT messen.
    // Kehrt in run_lazy_static_then_dynamic NACH provision_all (die DLLs stehen versions-aktuell) VOR der
    // Lade-/Mess-Phase zurueck -> keine gemessenen CSV-Zeilen, kein DLL-Laden. Entkoppelt den ~8h-Materialisierungs-
    // Bau vom mehrtaegigen Messlauf (Storage-cachebar). Default false = altes Verhalten (byte-identisch).
    bool provision_only = false;
    // S3 (§62-B COMDARE_PRUEF_ONLY): NUR das Konformitaets-Gate je bereits gebauter .so -- KEINE Messung, KEIN Neubau
    // (provision_all laeuft im Resume-Modus -> versions-aktuelle .so werden uebersprungen). Kehrt in
    // run_lazy_static_then_dynamic nach provision_all VOR der Mess-Phase zurueck. Default false = byte-identisch.
    // Gegenseitig ausschliessend mit provision_only (das zuerst greift). Nur wirksam mit per_binary_subdirs.
    bool pruef_only = false;
    // Storage #51 (Naht-Injektion, No-Op-Default => byte-neutral; Muster wie CompileFn/AlgoSigFn). Der Iterator ruft
    // sie SYNCHRON an der per-Binary-Naht (NACH result.csv+stamp, VOR RAII-DLL-Unload) — nie async/detached (I/O-
    // Contention = Messfehler). cache_push: perm.dll(+.version) -> Objekt-Store (Ebene B). measurement_sink:
    // result.csv -> measure-drop additiv (Ebene C). Leer (Default) => No-Op => golden/CI byte-identisch (Anti-Phantom).
    CachePushFn cache_push;
    // S2 (#46a): die BATCH-Warm-Cache-Hydrierung VOR dem Bau (Phase A, VOR provision_all). Leer (Default) => keine
    // Hydrierung => byte-neutral; der Host belegt sie via ArtifactCache::pull_tier_prefix (scharf nur via Env/CI).
    CachePullFn       cache_pull;
    MeasurementSinkFn measurement_sink;
    // Welle 5 (E-W5-2, §38-Fortschritts-Rueck-Kanal): No-Op-Default => byte-neutral; Muster EXAKT wie cache_push/
    // measurement_sink. Der Iterator feuert je Binary an der Per-Binary-Synchron-Naht (NACH result.csv+stamp/nach
    // Provisionierung) GENAU EINEN ProgressDelta (erste Meldung = Voll-Konfiguration, danach mixed-radix-minimale
    // Deltas in StaticBinaryView-Ordnung) und am Fensterende genau EINEN done=true-Delta (= §38.b-Fertig-Signal).
    // KEIN Mess-Daten-Rueckfluss. Leer (Default) => No-Op => golden/CI byte-identisch (Anti-Phantom).
    ProgressSinkFn progress_sink;
    // W11 (Ledger §43.c): der BAU-MODUS async Push-Pump. NUR im provision_only-Zweig aktiv (Mess-Modus bleibt strikt
    // synchron). Ist cache_push gesetzt + per_binary_subdirs, ueberlappt der Push mit dem Bau (statt Batch danach).
    //  - partial_marker_sink: nach je chunk_part_size gepushten DLLs EIN Teil-Marker (Cluster-Resume). Leer = keine.
    //  - chunk_part_size: N (0 = keine Teil-Marker). Der Host belegt es aus COMDARE_GN_PART_SIZE (Default 1024).
    // Beide leer/0 => byte-neutral (kein Pump-Teil-Marker; der Pump selbst ist reines Ueberlappen ohne Verhaltens-
    // Aenderung an der Push-MENGE). Byte-neutral bleibt ausserdem garantiert, wenn cache_push leer ist (Storage inert).
    PartialMarkerFn partial_marker_sink;
    std::size_t     chunk_part_size = 0;
    // #45 (§16.2-M1/§61-MODI, paralleler Mess-Loop): die Zahl paralleler MESS-Worker (ueber die Mess-Zellen). 0/1 =>
    // STRIKT sequentiell (1-Thread-Mess-Vollzug, §38.b) => verhaltens-/byte-identisch zum Ist (Measure/Release/Default,
    // golden-neutral). >1 => NUR im Debug-Modus (RunMethodology::Debug, single_thread==false) gesetzt: parallelisiert
    // ueber die Zellen, Ergebnisse in KANONISCHER builds-Reihenfolge gemerged (CSV strukturell identisch; MESSWERTE ohne
    // Garantie -- §61 "DASS es funktioniert"). Der Host/Entry belegt es aus der Methodik + COMDARE_MEASURE_PARALLEL.
    // KLAR GETRENNT vom Bau-Pool (build_parallelism/COMDARE_BUILD_PARALLEL = KOMPILATIONS-Worker).
    std::size_t measure_parallelism = 0;
    // #46b I1 (Bestandslog-Verdrahtung, opt-in; Muster EXAKT wie cache_push/cache_pull): der Objekt-Store-Transport
    // + Doc-Key + Key-Provider. Der Iterator konsultiert das Bestandslog VOR dem Bau (Dedup-Basis) und registriert
    // frisch gebaute Binaries DANACH (store_document_merged). bestand_key_of nimmt den Binary-Ausgabepfad und liefert
    // den 128-hex-Fingerprint (Option A, Host-injiziert; nullopt => diese Binary wird nicht registriert). ALLE
    // leer/ungesetzt (Default) => KEINE Registrierung/Dedup => byte-/verhaltensneutral (golden/CI byte-identisch).
    // Der Host (Facade) belegt sie aus Env COMDARE_BESTANDSLOG; der reale Provider kommt in I2 (.fingerprint-Sidecar).
    BestandTransport                                                        bestand_transport;
    std::function<std::optional<std::string>(std::filesystem::path const&)> bestand_key_of;
    std::string                                                             bestand_doc_key;
    // #46b I1b (Planer-getriebener Slice-Bau, opt-in unter bestandslog_active UND provision_only -- s.
    // planer_driven_active): der Producer slict die SELEKTIERTEN
    // indices in 4096er-Fenster und baut Fenster fuer Fenster; je Slice ein Reservierungs-Lifecycle
    // (pro-forma -> Kalibrierung eta_s+avg_size -> Done, PromiseGuard). bestand_present ist der Miss-Scan und seit
    // Lager-TP1(B)/G-A2 der BAU-FILTER (LEDGER:3397: per-Binary erkannt, Bestands-Treffer VOR dem Bau-Versuch
    // uebersprungen; dll_is_current bleibt zweite Verteidigungslinie). Host-Injektion hat Vorrang (Test-Naht);
    // absent bindet der Iterator sie selbst aus lager_contains + bestand_fingerprint_fn + bestand_zelle -- ohne
    // Provider bleibt sie leer => "alles fehlt" => volles Fenster (byte-identisch). owner_uuid/maschine
    // identifizieren die Reservierungen; threads = build_parallelism. Alle leer/absent (Default) -> die
    // Reservierungs-Schreibung ist inert; der Bau bleibt determin.
    bestandslog::PresenceFn bestand_present;
    std::string             bestand_owner_uuid;
    std::string             bestand_maschine;
    // T2-A/F4 (Owner-KERN Zaehler-Resume, Ledger abend-10: "Batch-Plan [Reihenfolge+Faecher] PERSISTENT VOR dem
    // Lauf, Resume = Zaehler je Phase [kompiliert/separat gemessen] gegen den Plan"): die Ablage dieses Plans.
    // Der planer-getriebene Bau schreibt den vollen Fenster-Plan hierhin, BEVOR das erste Fenster gebaut wird,
    // und fuehrt daneben (<datei>.zaehler) den Bau-Zaehler fort -- immer NACH dem Vollzug eines Fensters und nie
    // fuer ein fehlgeschlagenes (Codex-K1: "IDs vor Bau/Messung gezaehlt inkl. Fehlversuche"). Der SEPARATE
    // Mess-Lauf schreibt in dieselbe Datei die Mess-Front; sein feinkoerniger Resume-Arbiter bleibt die
    // per-Binary result.csv+stamp-Naht (T2-A/K2). LEER (Default) => keine Ablage, kein Plan-Resume =>
    // byte-/verhaltensidentisch zum Ist (golden/CI unberuehrt).
    std::filesystem::path batch_plan_datei;
    // T2-A/F4-NB2 (Korn-Divergenz, Vorwellen-Notiz mittag-20) -- DAS KORN DES PLANS, EINE QUELLE FUER BEIDE
    // WEGE. Es lag bis hierher zweimal im Code: der BAU-Weg fuehrte es als Funktions-Parameter von
    // run_planer_driven_provision, der MESS-Weg schrieb bestandslog::kBuildSliceGrain HART hin. Bei
    // abweichendem Korn tragen die beiden Stempel verschiedene "|korn="-Glieder -- der Mess-Lauf findet den
    // Plan seines eigenen Bau-Laufs dann NICHT und schreibt die Mess-Front nicht fort (fail-closed, aber
    // still falsch: der Plan IST da). Ab hier lesen BEIDE Wege diese eine Zahl. 0 (Default) == die Konstante
    // kBuildSliceGrain -> der produktive Lauf ist byte-identisch.
    //
    // WIE WEIT DIE KETTE REICHT, ehrlich: sie endet HIER. Es gibt kein RunProfileArgs-/XML-Feld, das dieses
    // Korn belegt -- so wie der frueher an dieser Stelle stehende Funktions-Parameter ebenfalls nur seinen
    // Default trug. Das ist KEIN Rueckschritt gegenueber dem Ist und auch keine tote Achse: das Korn ist erst
    // dann eine Host-Frage, wenn die Plan-Ablage selbst eine ist (batch_plan_datei ist im produktiven Host
    // heute unbelegt, Ledger mittag-16 Delta (3)). Wer die Ablage belegt, belegt beides in EINEM Zug -- und
    // findet die Naht dann an EINER Stelle statt an zweien.
    std::size_t batch_plan_korn = 0;
    // G-E3 (A1-Lager-Rest-Welle): das ZWEITE Genus PRODUKTIV. Bis hierher schrieb der Iterator nur
    // Binaries ins Lager; die MesswertKeyPolicy der B3-Factory hatte keinen Schreiber (Dossier-Befund
    // G-E3). Diese drei Felder verdrahten ihn -- exakt im Muster der drei bestand_*-Felder darueber
    // und mit derselben OPT-IN-Regel: alle leer/ungesetzt (Default) => KEINE Messwert-Registrierung
    // => byte-/verhaltensneutral. Der Schluessel ist NICHT der Binary-Fingerprint, sondern der
    // Messwert-Schluessel (messwert_key_hex ueber Mess-Zeilen + Hardware-Identitaet, keine Fusion);
    // mess_bestand_key_of nimmt das bin_dir der gemessenen Zelle und liefert ihn.
    // D-05: Bestandslog wird beim BAU **und** beim MESSEN fortgeschrieben, JE REALM (eigener doc_key).
    std::string                                                             mess_bestand_doc_key;
    std::function<std::optional<std::string>(std::filesystem::path const&)> mess_bestand_key_of;
    // G-E6 (syntax_version 4): der optionale Versions-Tag der Haupt-Achsen ("achse@X.Y.Zc;..."), den
    // jeder Messwert-Eintrag mitfuehrt. Leer = nicht gemeldet (v3-byte-gleiche Ausgabe).
    std::string mess_bestand_versions;
    // Folge-A (§62-NACHTRAG-4): die drei Zell-Koordinaten [d,e,f] DIESES Laufs (combo/opt/simd). Der Lager-
    // Schluessel ist das TUPEL (Fingerprint, Zelle) -- der Fingerprint allein traegt die per-Zelle-ISA nicht,
    // zwei Bauten derselben Permutation unter avx2/avx512 wuerden sonst falsch dedupliziert. Je Lauf konstant
    // (combo aus der Prozess-Umgebung, opt/simd aus der per-Perm build_version); der Host (Facade) belegt sie
    // aus denselben Quellen wie den Objekt-Key-Praefix. Leer (Default) => Zell-Feld leer => das Dedup verhaelt
    // sich wie zuvor ueber den Fingerprint allein (byte-/verhaltensneutral).
    bestandslog::ZellKoordinaten bestand_zelle;
    // I2 (Lager-Gate Integration): der reale Fingerprint-Provider (binary_id -> 128-hex K7b-Fingerprint). Der Host
    // (Facade) komponiert ihn aus denselben Stempel-Zeilen wie der Emitter; der Orchestrator schreibt je gebauter
    // Binary das `.fingerprint`-Sidecar, das bestand_key_of dann als Lager-Index-Schluessel liest. Leer = byte-neutral.
    FingerprintFn bestand_fingerprint_fn;
    // A7-B (G2 Folge-B): die MENGEN-Signatur der CMake-enabled Achsen-Typlisten DIESES Treibers (der Host/Facade
    // belegt sie opt-in aus driver_build_variant_signature()). Sie reist unveraendert nach BuildConfig.build_variant_sig
    // weiter; der Orchestrator vergleicht sie beim Skip-Check gegen das `.variant`-Sidecar und schreibt sie bei Erfolg.
    // Leer (Default) = Variant-Gate AUS (byte-neutral: exakt der bisherige Versions-/Organ-Skip).
    std::string build_variant_sig;
    // E-04-P1 (Marker-Familie v2): die PFLICHT-Koordinaten des Live-Fortschritts-Kanals. Der Host
    // (Facade) belegt sie aus DERSELBEN Env, aus der die CI-Emission ihre Testat-Zelle bildet
    // (COMDARE_LANE / COMDARE_GN_OPT+COMDARE_GN_SIMD / COMDARE_MEASUREMENT_COMBO) und rendert die
    // zelle= ueber die EINE Legenden-Quelle des Planers -- keine zweite Ableitung, kein Raten.
    // Ungesetzt (Default) => die Marker-Zeilen tragen den ehrlichen Sentinel "unbelegt"; die Zeile
    // selbst entfaellt NIE (der Kanal darf nie stumm sein). Rein beobachtend: nur std::cerr.
    MarkerKontext marker_kontext;
    // T-15 (09.08.2026) -- DIE DRIFT-KLAMMER JE ZELLE. reps/threshold/max_reruns kommen aus dem
    // Profil-XML (<drift_gate reps threshold_pct max_reruns/>), NICHT aus dem Code: die Schwelle ist
    // #156-kalibrierungs-gegatet und darf ohne Neubau aenderbar sein.
    //
    // SELBSTCHECK: dies ist NICHT n_repeats. n_repeats (oben, KF-10) ist die Berichts-Wiederholung und
    // erzeugt N eigene Zeilen mit eigenem repetition_index; drift_gate.reps ist gruppen-intern und
    // erzeugt EIN Drift-Urteil ueber EINE Zelle. Wer die beiden vermengt, misst N*M statt N.
    //
    // Der Default (reps=3, 5 %, max_reruns=5) IST die Owner-Regel aus GOAL-v8 VI.5 -- ein Default von
    // "aus" haette bedeutet, dass die Regel ueberall dort schweigt, wo das XML sie nicht nennt, und
    // genau das war der Zustand, den T-15 beendet.
    DriftGateConfig drift_gate{};
};

// ── Eine gemessene CSV-Zeile (Binary × dyn-Setting) ───────────────────────────
struct LazyMeasuredRow {
    std::string          binary_id;              // statische Rekombination (= die Tier-Binary)
    std::string          setting_label;          // dyn. Belegung "axis.var=value/…" (leer = keine dyn-Dimensionen)
    std::string          setting_id;             // binary_id (+ "#" + setting_label) = eindeutiger Baum-Key
    NodeObserverSnapshot observer{};             // die real gezogenen Observer-Werte (>0 bei echter Messung)
    std::uint64_t        applied_axis_count = 0; // wie viele Achsen die Steuerung real annahmen
    // (B): Host-Gesamt-Wall-Clock DIESER Messung + Eingabe-n_ops; GOAL-M1.1 (Audit K2): ns_per_op =
    // total_ns/timed_ops (Workload-Pfad: Σ getimte Samples; Legacy: 2*n_ops). timed_ops==0 → ns_per_op=0.
    std::int64_t  total_ns  = 0;
    std::uint64_t n_ops     = 0;
    std::uint64_t timed_ops = 0;
    // GOAL-L1: per-Interface-Funktions-Latenzen (Reihenfolge kOpKindNames) — z-Achsen-Quelle der 3D-Auswertung.
    std::array<OpKindLatency, 6> op_lat{};
    // KONSOLIDIERUNG (I1): der EINE konsolidierte Observer-POD (axis_stats[17][8] + seg_ns[17]/Pfad B + Meta).
    // Maßgebliche CSV-Quelle: stat_*-Spalten aus unified.axis_stats, seg_*-Spalten aus unified.seg_ns (Pfad B, reale
    // Komposition). Ersetzt den früheren V3-Snapshot + den Pfad-A-Segment-Timer.
    anatomy::ComdareTierObserverSnapshot unified{};
    bool                                 unified_real = false;
    // Achse 2 (INC-3): Lastprofil + Mess-GÜLTIGKEIT (Zwei-Phasen-Cache-Warmup exakt). profile_name leer/"-" =
    // alter fixer Workload (kein Achse-2-Profil). two_phase_valid=false ⇒ Messung UNGÜLTIG (nicht als valide werten).
    std::string profile_name;
    bool        two_phase_valid = false;
    // INC-29.1 (D2): Mess-Status DIESER Zelle (aus PermResult durchgereicht). Failed -> op_lat-Spalten "failed"
    // statt 0 ("Messung nie als Nullen"). Default Ok = gueltige Messung (Zahlen, byte-identisch).
    ::comdare::cache_engine::measurement::SampleStatus sample_status =
        ::comdare::cache_engine::measurement::SampleStatus::Ok;
    // RF-2 (§70.2, D1): ZULASSUNGS-Status dieser Perm -- disjunkt zu sample_status (D2). Gesperrt heisst
    // "auf dieser Maschine nicht zugelassen, deshalb GAR NICHT gemessen" und rendert das eigene D1-Token
    // statt Zahlen; Failed hiesse "gemessen und gescheitert". Default Zugelassen = heutiges Verhalten,
    // byte-identisch. HEUTE SETZT DAS NIEMAND: der Entscheider (simd_release_on_machine) kommt mit A6 --
    // bis dahin ist der Kanal gebaut und unausgeloest (Beleg: test_rf2_admission_marker_inert.cpp).
    ::comdare::cache_engine::measurement::AdmissionStatus admission_status =
        ::comdare::cache_engine::measurement::AdmissionStatus::Zugelassen;
    // A15/FK-1 (Owner-Q4 per Volles-GO 02.08.): BAU-Status dieser Zeile -- die dritte, von sample_status
    // (D2) und admission_status (D1-Zulassung) disjunkte Aussage. NichtGebaut heisst "es gibt fuer diese
    // Permutation gar keine Binary"; bis zu diesem Paket verschwand ein Bau-Fehler nur ins Log und die
    // Auswerte-CSV konnte "nie gebaut" nicht von "nie geplant" unterscheiden. Default Gebaut = heutiges
    // Verhalten, byte-identisch; gesetzt wird es AUSSCHLIESSLICH im Bau-Fehler-Zweig von
    // measure_one_binary -- der gruene Pfad kennt den Wert nie.
    ::comdare::cache_engine::measurement::BuildCellStatus build_status =
        ::comdare::cache_engine::measurement::BuildCellStatus::Gebaut;
    // M3v2-SELEKTION (Task #156): die 5 Lauf-/Selektions-Tags je Zeile (aus LazyRunConfig durchgereicht). Reine
    // Metadaten (kein Mess-Einfluss) → ermöglichen die Trennung Basis vs Per-Achsen-Sweep vs SOTA-Reihe A/B/C
    // sowie die Working-Set-N- und Plattform/Build-Version-Achsen in der Auswertung.
    std::string   series         = "-";          // SOTA-Reihe {A,B,C} oder "-"
    std::string   pruefling_type = "-";          // #171: "full" (Reihe A self-contained) / "abstract" (B/C) / "-"
    std::string   sweep_axis     = "-";          // gesweepte Achse oder "-"
    std::uint64_t working_set_n  = 0;            // N (Record-Zahl) dieser Mess-Zeile (= workload_records)
    std::string   platform       = "win-x86_64"; // Plattform-Tag
    std::string   build_version  = "m3v2";       // Build-Version-Tag
    // GO-5 Fork 6: der Fairness-Modus der Reihe dieser Zeile (aus LazyRunConfig::row_fairness_mode
    // durchgereicht) — "common_denominator" / "native" / "-" (Basis/Sweep/ungesetzt). Reine Metadaten.
    std::string fairness_mode = "-";
    // GO-5 Fork 7: der tool-berechnete H2-Code-Qualitaets-Score der Reihe dieser Zeile (aus
    // LazyRunConfig::row_h2_score durchgereicht) — Dichte-String / "n/a" / "-". Reine Metadaten.
    std::string h2_score = "-";
    // #165-B (P-MD8, 2026-06-20): STATISTISCHER Ausreißer-Flag je Zeile (gate-frei). 1 = ns_per_op dieser Zeile
    // ist ein Ausreißer relativ zum Median der (binary_id, workload/profile_name)-Gruppe (Heuristik s.u.); 0 = nicht.
    // Default 0 (kein Flag) → bestehende Aufrufer unverändert; befüllt OPT-IN durch annotate_quality_flags(rows) VOR
    // der CSV-Emission. WICHTIG (Klarstellung): dies ist NUR der statistische Ausreißer-Flag (rein aus den Mess-Werten
    // ableitbar, ohne Infra/Gate). Die OS-quiesced `system_disturbed`-Provenienz (AP-M1/P-MD2) ist eine GETRENNTE,
    // HELD/Infra-gebundene Sache und NICHT Teil dieser Spalte.
    std::uint32_t quality_flag = 0; // statistischer Ausreißer-Flag (1) / kein Ausreißer (0)
    // #156-De-Risk (2026-06-20): die HW-Performance-Counter (PMC) DIESER Mess-Zeile (aus PermResult::pmc). Default
    // PmcCounters{} = alle 0, available=false → lokal mit NullPmcSource (COMDARE_ENABLE_PMC=OFF) ehrlich 0/available=0;
    // mit Intel-PCM=ON real (Montag Linux+PMC). Die 7 PMC-Felder werden ADDITIV als LETZTE CSV-Spalten emittiert
    // (lazy_csv_header single-source) — bestehende Spalten unberührt → cowfix-v1/tier150-Leser bleiben kompatibel.
    measurement::PmcCounters pmc{};
    // T-15 (09.08.2026) -- DAS DRIFT-URTEIL DIESER ZELLE, aus DriftGatedCellResult durchgereicht.
    //
    // SELBSTCHECK: diese vier Felder sind KEIN Messwert, sondern die PROVENIENZ des Messwerts daneben.
    // Ohne sie waere eine Zelle, die ihr Rerun-Budget erschoepft hat (also bis zuletzt streute), von
    // einer beim ersten Versuch stabilen nicht zu unterscheiden -- kontaminierte Messdaten sind die
    // einzige unheilbare Klasse dieser Arbeit, und ihre Sichtbarkeit haengt genau an diesen Zahlen.
    //
    // drift_reps == 0 heisst "Gate war fuer diese Zelle aus" -> es liegt KEINE Drift-Aussage vor. Das
    // ist die D4-Unterscheidung: nicht "stabil", nicht "instabil" -- keine Aussage.
    std::uint32_t drift_reps       = 0;     // Wiederholungen je Gruppe (0 = Gate aus)
    std::uint32_t drift_reruns     = 0;     // ausgeloeste Reruns dieser Zelle
    double        drift_relative   = 0.0;   // Median-relative Spannweite der angenommenen Gruppe
    bool          drift_bestimmbar = false; // D4: gab es ueberhaupt einen Nenner (Median > 0, n >= 2)?
    bool          drift_stabil     = false; // bestimmbar UND unter der Schwelle
};

// ── (B/C/D/X) EINHEITLICHES CSV-Schema (global + per-Binary identisch) ──────────────────────────────────
//   binary_id;setting;repetition;n_ops;total_ns;ns_per_op;
//   seg_<T0>_ns;…;seg_<T17>_ns;   (18 per-Achsen-Timer-Spalten, kCompositionAxisNames-Reihenfolge)
//   <die 13 differenzierten Observer-Counter>;applied_axes
// (X) Die frühere `na_axes`-Notiz-Spalte ist ENTFALLEN: ALLE 17 Achsen tragen jetzt einen echten per-Achsen-
// Timer (kein „15 Deskriptor-Achsen ohne Timer" mehr). Die 17 seg_*-Spalten = "n/a", wenn das Modul kein
// IMeasurableWorkloadV3 exponiert (seg_real=false) — ehrlich n/a, NICHT 0. Die Spaltennamen werden aus der
// EINEN Single-Source kCompositionAxisNames (axis_path_serialization.hpp) generiert → keine Namens-Drift.
//
// Phase A (2026-06-04) PER-ACHSEN-OBSERVER-SPALTEN: zusätzlich `stat_<achse>_<feld>` je befülltem statistics()-Feld
// (generisch aus der EINEN Single-Source kV3AxisSchema, observable_tier.hpp). CSV-SCHEMA-WAHL = WIDE NAMED COLUMNS
// (nicht long-format), BEGRÜNDUNG: (1) die bestehende CSV ist bereits wide (13 Observer-Counter + 18 seg-Spalten),
// 1 Zeile je Messung — wide bleibt konsistent + direkt vom Thesis-PDF-Pipeline konsumierbar; (2) der V3-POD ist
// generisch [17][8], ABER nur die im Schema BENANNTEN (non-null) Felder werden zu Spalten → die Breite ist gegen
// weitere Felder gedeckelt (Phase B füllt nur leere Schema-Slots, KEINE neue Spalte ausser tatsächlich benannt);
// (3) Schema-Stabilität: kV3AxisSchema IST der Vertrag Schreiber(DLL)↔Spaltenname(Host) → keine Drift. Die
// stat_*-Spalten sind „n/a", wenn die DLL kein Mess-Interface trägt (unified_real=false) — ehrlich n/a, NICHT 0.
//
// SEMANTIK der stat_*-Spalten (Konsistenz-Stand nach #216-H2): PER-MESSUNG, WARMUP-/LOAD-FREI — KEINE
// Cumulative-Absolut-Werte. perm_runner::run_observable_perm ruft tier_clear() VOR der Mess-Last; der Workload-Pfad
// perm_runner::run_workload_perm ruft nach der ungemessenen Load-Phase zusätzlich tier_reset_statistics() (daten-
// erhaltend) VOR PMC/Wall-Clock-Start. Die auto-gekoppelten Instanz-Achsen (T0/T1/T2/T3/T7/T8/T10/T17/T18) werden
// dabei STATISTIK-zurückgesetzt (reset(), nicht daten-clear()) → ihr V3-Wert = NUR die Op-Zähler DIESER Messung/Run-
// Phase. WICHTIG (Defekt-Fix): vor dem Fix riefen T1/T2/T17 nur clear() (= nur Daten, stats_ blieb stehen) und T18/T10
// gar nichts → ihre Zähler akkumulierten über die 3 Wiederholungen je (Binary×Setting); #216-H2 ergänzt den separaten
// daten-erhaltenden Reset nach Load für run_workload_perm. Die Scan-Achsen (T4/T5/T9/T11..T16) sind in
// fill_observer_v3 idempotent (reset()+scan je Observe) → Zustand zum Observe-Zeitpunkt; T0/T6 tragen die
// Container-/Allocator-Zähler seit dem letzten Reset. Damit ist der stat_*-Block pro Zeile konsistent: kein doppeltes
// Zählen über Wiederholungen oder Load+Run hinweg.
//
// A8-S3 (2026-08-04) -- CSV-LEGENDE der stat_*-Achsen-Semantik (Katalog Klasse C, "CSV-Legenden-Semantik").
// Ohne diese Legende liest eine 0 in den folgenden Spalten wie ein Messwert; sie ist aber in jedem der
// Faelle eine DEKLARIERTE Eigenschaft der Achse oder des Mess-Kanons. Wer die Spalten auswertet, braucht
// diese fuenf Saetze:
//   * stat_io_dispatch_*   -- IN-MEMORY-SIMULATION, kein Platten-IO (Entscheid: der Dispatch laeuft ueber das
//                             reale Slot-Backing, nicht ueber ein Geraet). bytes/rounds beschreiben die
//                             Dispatch-ARBEIT, nicht Datentraeger-Durchsatz; align_adjusts bleibt bei
//                             InMemoryOnly ehrlich 0.
//   * stat_migration_policy_* -- DECIDE-ONLY: gemessen wird die ENTSCHEIDUNG, nicht der Umzug. tier_moves ist
//                             genau dann > 0, wenn der echte 2-Ebenen-Schritt (IMigratableTier) gerufen wurde;
//                             bei NoMigration bleibt die ganze Zeile ehrlich 0 (Vergleichs-Nullpunkt).
//   * stat_persistence_target_* -- STAGING, nicht Platte: bytes_staged/records_staged zaehlen die
//                             Rueckschreib-VORBEREITUNG. MemoryOnlyTarget hat keinen Rueckschreib-Pfad ->
//                             bytes_staged ehrlich 0 bei positiven rounds; device_flushes bleibt 0, solange
//                             has_device_writeback_path() false meldet. (Seit A8-S3 traegt die Zeile ueberhaupt
//                             Werte -- vorher war sie strukturell 0, s. fill_observer_pathb_driven_v3.)
//   * stat_concurrency_*   -- MESS-KANON EIN THREAD (Debug=parallel / Messung=1-Thread): contention und
//                             validation_fail sind deshalb 0 BY DESIGN, nicht mangels Instrumentierung.
//                             acquire/release zaehlen die realen Primitiv-Paare.
//   * stat_allocator_*     -- SKALIERUNGS-MODUS: bytes_in_use ist der Wert ZUM OBSERVE-ZEITPUNKT (End-Stand),
//                             kein Peak; budget_reject zaehlt nur bei Pool-Budget-Strategien. Peak und
//                             Fragmentierung sind am Ist NICHT ERHOBEN und stehen als "n/a" in den
//                             Klasse-C-Spalten am Zeilen-Ende (alloc_bytes_in_use_peak / alloc_*_frag_milli).
[[nodiscard]] inline std::string lazy_csv_header() {
    std::string h = "binary_id;setting;repetition;n_ops;total_ns;ns_per_op;";
    // GOAL-L1 (2026-06-12): per-Interface-Funktions-Spalten op_<art>_{n,p50_ns,p99_ns} (Reihenfolge
    // kOpKindNames, single-source perm_runner) — Verarbeitungsdauer je Testdatensatz-Operation GETRENNT
    // je Interface-Funktion (z-Achse der 3D-Diagramme; Ausgabe = Testdaten-Konfig × Tier).
    for (char const* k : kOpKindNames) {
        h += "op_";
        h += k;
        h += "_n;";
        h += "op_";
        h += k;
        h += "_p50_ns;";
        h += "op_";
        h += k;
        h += "_p99_ns;";
    }
    for (std::size_t i = 0; i < kCompositionAxisNames.size(); ++i) { // 17 seg_<axis>_ns-Spalten, single-source
        h += "seg_";
        h += kCompositionAxisNames[i];
        h += "_ns;";
    }
    // P-MD3 (2026-06-18): die kommensurable Coverage-Versöhnung des Pfad-B-Segment-Laufs. seg_framework_ns = benannter
    // Rest (Loop-/Instrumentierungs-Overhead), seg_run_total_ns = äußere Wall-Clock des Segment-Laufs (Coverage-Nenner),
    // seg_coverage = Σseg_ns / seg_run_total_ns (gegen die EIGENE Wall-Clock → ~100%; NICHT mehr gegen die
    // unkommensurable Real-Workload-total_ns, was die irreführende ~33,6%-Quote erzeugte).
    h += "seg_framework_ns;seg_run_total_ns;seg_coverage;";
    h += "search_lookup;hit;miss;insert;erase;peak;bytes_alloc;bytes_in_use;alloc_cnt;dealloc_cnt;fail;"
         "obs_axes;fill;applied_axes;";
    // Phase A: die per-Achsen-Observer-Spalten stat_<achse>_<feld>, generisch aus dem V3-Schema (single-source).
    for (std::size_t t = 0; t < anatomy::kV3AxisCount; ++t) {
        for (std::size_t f = 0; f < anatomy::kV3FieldCount; ++f) {
            char const* fld = anatomy::kV3AxisSchema[t].names[f];
            if (fld == nullptr) continue; // ungenutztes / Phase-B-Feld → keine Spalte
            h += "stat_";
            h += kCompositionAxisNames[t];
            h += '_';
            h += fld;
            h += ';';
        }
    }
    h +=
        "v3_filled_axes;workload;two_phase_valid;"; // Diagnose 17 Achsen befüllt + Achse 2 (Lastprofil + Mess-Gültigkeit)
    // M3v2-SELEKTION (Task #156): die 5 Selektions-/Lauf-Tag-Spalten ANS ENDE (Positionen aller bestehenden
    // Spalten unverändert → header-getriebene Auswertung + Resume-Schema-Vergleich bleiben rückwärtskompatibel).
    // #171 (2026-06-20): pruefling_type GANZ ANS ENDE (additiv, gleiches lazy_csv_header-Muster wie series/sweep_axis):
    // "full" (Reihe A = Originalkonfiguration/self-contained), "abstract" (Reihe B/C = Teilmenge+Host-Fallback),
    // "-" für Basis/Sweep/cowfix-v1 (die alte cowfix-v1-CSV hatte die Spalte nicht → die Auswertung liest sie leer/n-a,
    // Datenerhaltung). Trennt in der Auswertung Original- vs rekombinierte Konfiguration je Messreihe.
    // #165-B (P-MD8, 2026-06-20): quality_flag GANZ ANS ENDE (additiv, gleiches header-getriebenes Muster wie
    // series/pruefling_type). STATISTISCHER Ausreißer-Flag (1/0) — rein aus den Mess-Werten ableitbar, gate-frei.
    // Alte CSVs (z.B. cowfix-v1/tier150) hatten die Spalte NICHT → die header-getriebene Auswertung liest sie dort
    // leer/n-a (Datenerhaltung, kein cowfix-v1-Leser bricht). NICHT zu verwechseln mit der OS-quiesced
    // system_disturbed-Provenienz (AP-M1/P-MD2) — die bleibt HELD/Infra und ist KEINE Spalte hier.
    h += "series;sweep_axis;working_set_n;platform;build_version;pruefling_type;quality_flag";
    // #156-De-Risk (2026-06-20): die 7 PMC/HW-Counter-Spalten GANZ ANS ENDE (additiv, header-getrieben, gleiches
    // Muster wie series/pruefling_type/quality_flag). EXAKT die realen PmcCounters-Feldnamen (pmc_source.hpp) in
    // identischer Reihenfolge zu format_csv_row. Default 0 / pmc_available=0 bei NullPmcSource (COMDARE_ENABLE_PMC=OFF);
    // mit Intel-PCM=ON real (Montag Linux+PMC). Alte CSVs (cowfix-v1/tier150) hatten die Spalten NICHT → die header-
    // getriebene Auswertung liest sie dort leer/n-a (Datenerhaltung, kein cowfix-v1-Leser bricht). KEINE bestehende
    // Spalte umbenannt/verschoben. Schließt die #156-WIDE-Naht (perm_runner→IPmcSource→CSV).
    h += ";pmc_cache_misses_l1;pmc_cache_misses_l2;pmc_cache_misses_l3;pmc_dtlb_misses;"
         "pmc_coherence_invalidations;pmc_energy_micro_joules;pmc_available;container_store_ops";
    // GO-5 Fork 6 (2026-07-12): fairness_mode ALS LETZTE Spalte (additiv, header-getrieben, gleiches Muster
    // wie series/pruefling_type/quality_flag/PMC/container_store_ops). Traegt den deklarierten Thesis-
    // §sec:fairness-Modus der SOTA-Reihe ("common_denominator"/"native") bzw. "-" (Basis/Sweep/ungesetzt).
    // Alte CSVs hatten die Spalte NICHT → die header-getriebene Auswertung liest sie dort leer/n-a
    // (Datenerhaltung, kein Leser bricht). KEINE bestehende Spalte umbenannt/verschoben.
    h += ";fairness_mode";
    // GO-5 Fork 7 (2026-07-12): h2_code_quality_score ALS LETZTE Spalte (additiv, header-getrieben,
    // gleiches Muster wie series/pruefling_type/quality_flag/PMC/fairness_mode). Traegt den TOOL-
    // BERECHNETEN H2-Score der SOTA-Reihe (Akte sota_h2_scores.xml, Thesis-Hypothese H2) bzw. "n/a"
    // (Reihe ohne Akten-Eintrag — honest, kein 0-Phantom) bzw. "-" (Basis/Sweep). Alte CSVs hatten
    // die Spalte NICHT → header-getriebene Auswertung liest sie dort leer/n-a (Datenerhaltung).
    h += ";h2_code_quality_score";
    // A8-S3 / Qualitaets-Katalog KLASSE C (2026-08-04) -- HOST-SEITIGE Spalten-Ereignisse, KEIN Wire-Ereignis.
    // Alle vier Bloecke sind END-Appends nach exakt dem Muster von series/PMC/fairness_mode: keine bestehende
    // Spalte wird umbenannt oder verschoben, alte CSVs lesen sie header-getrieben leer/n-a (Datenerhaltung).
    //
    // (C1) pmc_branch_misses (Katalog P11, Befund B8): die Spalte existiert seit dieser Scheibe additiv.
    //      Sie war zweimal falsch etikettiert: erst als "ERHEBT real" (Zusage ohne Quelle), dann ab
    //      2026-08-06 als honest-0 mit dem Zusatz "eine spaetere echte Quelle (M-3a) fuellt hier". GENAU
    //      DAS IST JETZT GESCHEHEN (M-3a, 2026-08-07): LinuxPerfPmcSource oeffnet PERF_TYPE_HARDWARE /
    //      PERF_COUNT_HW_BRANCH_MISSES als vierten Zaehler -- ein generisches Event, das auf Intel UND AMD
    //      abbildet. Die Zelle rendert seither ueber pmc_zelle(), also mit demselben Ehrlichkeits-Vertrag
    //      wie l2/l3/coherence/energy.
    //
    //      ABWAEGUNG 0 -> "n/a" (2026-08-07, bewusst getroffen und hier festgehalten): die Umstellung
    //      aendert den ZELLINHALT, nicht das Schema -- Spaltenname und Position bleiben, kein Bestands-
    //      Leser verliert eine Spalte. Ein Werkzeug, das die Zelle als Zahl parst, saehe kuenftig "n/a".
    //      Die Konsumenten-Suche (2026-08-07, super/Code + thesis, 5709 Dateien im Nenner, mit
    //      Positiv-Kontrolle ueber "cache_misses_l1") fand fuer branch_misses/pmc_branch_misses KEINEN
    //      Konsumenten ausserhalb des gespiegelten cache-engine-Baums selbst: kein super-eigenes Werkzeug
    //      (03_binary_to_csv liest die Spalte nicht), kein tools/latex_anhang-Treffer, kein Thesis-Treffer,
    //      keine golden-Datei (die golden_fullpilot_*-Dateien tragen binary_ids, keine Mess-Zellen).
    //      Hinzu kommt: sobald der Zaehler real erhoben wird -- und das ist ab hier der Normalfall auf
    //      Linux+PMC -- steht dort ohnehin eine Zahl; "n/a" erscheint nur, wenn das Oeffnen scheitert, und
    //      DANN ist "n/a" die einzig richtige Aussage. Eine ECHTE 0 bleibt "0" (pmc_zelle unterscheidet
    //      das), und die PMC-off-Zeile behaelt ihre 0-Konvention unveraendert.
    //
    //      Die Spalte steht bewusst NICHT im 7er-Block, sondern hier hinten: der Block ist positionsstabil
    //      fuer Bestands-Leser.
    h += ";pmc_branch_misses";
    // (C2) Tail-Perzentile p999 je Op-Art (Katalog Abschnitt 5 "TAIL-PERZENTILE"): aus DENSELBEN IST-Vektoren
    //      wie p50/p99, Reihenfolge kOpKindNames (single-source, identisch zum op_*-Block oben). Kern-Groesse
    //      fuer T6-Alloc-Tail und T16-eager/lazy-Pareto, fuer die p99 zu grob ist.
    for (char const* k : kOpKindNames) {
        h += ";op_";
        h += k;
        h += "_p999_ns";
    }
    // (C3) Die T6-Speicher-EHRLICHKEIT (Katalog Entscheide E2/E9, Befund B7). Drei Groessen, die der
    //      Thesis-Kanon verlangt, fuer die es am Ist aber KEINE erhobene Quelle gibt:
    //        * alloc_bytes_in_use_peak -- das SA-Wire-Schema traegt nur den Momentanwert bytes_in_use
    //          (axis_stats[6][1]); ein Host-Peak braeuchte periodische tier_observe-Zuege, die der Mess-Pfad
    //          heute nicht faehrt (Katalog "ZEITREIHEN-LUECKE"). Ein Momentanwert unter Peak-Etikett verzerrt
    //          CoW-lastige Layouts -- genau die Fehl-Etikettierung B7 in measurement_snapshot.hpp:106-107.
    //        * alloc_external_frag_milli / alloc_internal_frag_milli -- die Fragmentierungs-Felder existieren
    //          in den GATTUNGS-Wire-Formen (Set/Sequence axis_stats[5][5]/[5][6]), NICHT im SA-T6-Schema.
    //      Sie werden deshalb ehrlich als NICHT ERHOBEN gerendert (G3-Regel: n/a statt stiller 0) -- ueber die
    //      EINE D2-Taxonomie sample_status_token(SourceUnavailable) == "n/a", nicht ueber ein neues Vokabular.
    //      Die Spalten existieren, damit die Luecke MASCHINENLESBAR ist statt unsichtbar: sobald eine echte
    //      Quelle da ist (Wire-Slot oder Zeitreihen-Zug), fuellt sie genau hier -- ohne Schema-Bruch.
    h += ";alloc_bytes_in_use_peak;alloc_external_frag_milli;alloc_internal_frag_milli";
    // B-5 Runde 2 (2026-08-08, Lead-Entscheid): der amd_l3-Uncore-L3-Miss-Zaehler als EIGENE Spalte,
    // additiv ganz am Ende (END-Append-Muster wie die A8-S3-Bloecke). Der NAME traegt die Geltung:
    // "uncore_systemweit" sagt, dass hier der ganze CCX gezaehlt wurde und nicht der Pruefling allein.
    // Genau deshalb steht der Wert NICHT in pmc_cache_misses_l3 -- der ist per-Task, und beide unter
    // einer Ueberschrift waeren der Datenbruch aus Ledger :4506, nur quer ueber Vendoren.
    h += ";pmc_l3_miss_uncore_systemweit";
    // T-15 (2026-08-09) -- DIE DRIFT-PROVENIENZ, vier Spalten, END-Append nach demselben Muster wie
    // series/PMC/fairness_mode: keine bestehende Spalte wird umbenannt oder verschoben, alte CSVs
    // lesen sie header-getrieben leer/n-a (Datenerhaltung, kein Bestands-Leser bricht).
    //
    // SELBSTCHECK: die vier Spalten beantworten GENAU EINE Frage -- "wie zuverlaessig ist die Zahl in
    // dieser Zeile?". Sie sind bewusst NICHT in eine einzige Sammel-Spalte gefaltet:
    //   drift_reps       -- Wiederholungen je Gruppe. 0 == das Gate war fuer diese Zelle AUS; dann
    //                       steht in den drei folgenden Spalten der ehrliche n/a-Token und keine 0.
    //   drift_reruns     -- wie oft die Gruppe neu gemessen wurde. > 0 heisst: die Maschine war
    //                       waehrend dieser Zelle unruhig.
    //   drift_relative   -- die Median-relative Spannweite der ANGENOMMENEN Gruppe (Anteil, nicht
    //                       Prozent -- 0.05 == 5 %). n/a, wenn D4 sie fuer unbestimmbar erklaert
    //                       (kein positiver Median): dort gaebe es keinen Nenner, und eine 0 waere
    //                       exakt die Luege, die D4 am 08.08. abgestellt hat.
    //   drift_status     -- "stabil" / "instabil" (Budget erschoepft, nie stabil geworden) /
    //                       "unbestimmbar" / der n/a-Token bei ausgeschaltetem Gate.
    h += ";drift_reps;drift_reruns;drift_relative;drift_status";
    h += "\n";
    return h;
}

/// Extrahiert den repetition_index aus dem setting_label (Segment "repetition.repetition_index=N");
/// "-" wenn die Rep-Dim nicht aktiv ist (kein Segment gefunden).
[[nodiscard]] inline std::string lazy_extract_repetition(std::string const& setting_label) {
    static constexpr char key[] = "repetition_index=";
    std::size_t const     p     = setting_label.find(key);
    if (p == std::string::npos) return "-";
    std::size_t const b = p + (sizeof(key) - 1);
    std::size_t       e = b;
    while (e < setting_label.size() && setting_label[e] != '/') ++e;
    return setting_label.substr(b, e - b);
}

/// Achse 2 (INC-3): extrahiert die workload_id aus dem setting_label (Segment "workload.workload_id=X"); leer
/// wenn die Workload-Dim nicht aktiv ist (kein Segment) → Aufrufer fällt auf den alten fixen Workload zurück.
[[nodiscard]] inline std::string lazy_extract_workload_id(std::string const& setting_label) {
    static constexpr char key[] = "workload_id=";
    std::size_t const     p     = setting_label.find(key);
    if (p == std::string::npos) return {};
    std::size_t const b = p + (sizeof(key) - 1);
    std::size_t       e = b;
    while (e < setting_label.size() && setting_label[e] != '/') ++e;
    return setting_label.substr(b, e - b);
}

/// Formatiert EINE LazyMeasuredRow als CSV-Zeile (Schema lazy_csv_header). ns_per_op = total_ns/(2*n_ops)
/// (insert+lookup). seg_*-Felder: echte ns wenn seg_real, sonst "n/a" (ehrlich, NICHT 0).
[[nodiscard]] inline std::string format_csv_row(LazyMeasuredRow const& row) {
    auto const& o = row.observer;
    std::string out;
    out.reserve(256);
    namespace cem = ::comdare::cache_engine::measurement;
    // A15/FK-1 (Owner-Q4 per Volles-GO 02.08.): der ZEILEN-WEITE Ersatz-Zell-Inhalt. Es gibt zwei Faelle,
    // in denen es fuer diese Zeile ueberhaupt keine Messwerte GEBEN kann und in denen eine 0 eine Luege
    // waere ("Messung nie als Nullen"):
    //   (a) die Binary wurde nie gebaut          -> D1-Token "nicht_gebaut" (BuildCellStatus),
    //   (b) die .so war nicht ladbar / trug kein Mess-Interface -> D2 "n/a" (SourceUnavailable).
    // Beide bekommen EINE Marker-Zeile je Binary, gerendert aus DIESEM Renderer (nie aus einer
    // handgezaehlten Feldliste) -- damit ist die Spaltenzahl per Konstruktion erhalten (Auflage K3).
    // Betroffen ist jede Zelle, deren Wert aus der (nicht stattgefundenen) Messung ODER aus der (mangels
    // DLL unbekannten) dynamischen Belegung kaeme; die Identitaets- und Lauf-Tag-Spalten stammen aus der
    // Lauf-KONFIGURATION und bleiben befuellt, damit die Marker-Zeile dem Lauf zuordenbar bleibt.
    // VORRANG: (a) vor (b) vor gesperrt vor failed -- was nie gebaut wurde, kann weder zugelassen noch
    // gemessen worden sein. Der gruene Pfad (Gebaut + Ok/Failed) laesst zell_ersatz LEER und rendert
    // Zeichen fuer Zeichen wie vor diesem Paket.
    std::string_view zell_ersatz{};
    if (row.build_status == cem::BuildCellStatus::NichtGebaut)
        zell_ersatz = cem::build_cell_status_token(cem::BuildCellStatus::NichtGebaut);
    else if (row.sample_status == cem::SampleStatus::SourceUnavailable ||
             row.sample_status == cem::SampleStatus::NotApplicable)
        zell_ersatz = cem::sample_status_token(row.sample_status); // "n/a", in axis_error.hpp zementiert
    // Wert-Zelle: im Marker-Fall der Ersatz, sonst der gerenderte Wert. EINE Naht -- so kann keine Spalte
    // versehentlich eine Null behalten. `zelle` schreibt ohne Trenner, `zelle_sep` mit.
    auto zelle     = [&](std::string_view v) { out += (zell_ersatz.empty() ? v : zell_ersatz); };
    auto zelle_sep = [&](std::string_view v) {
        zelle(v);
        out += ';';
    };
    out += row.binary_id; // Identitaet: steht IMMER da, auch in der Marker-Zeile
    out += ';';
    // setting/repetition stammen aus der dynamischen Belegung -- ohne geladene DLL gibt es keine.
    zelle_sep(row.setting_label.empty() ? std::string_view{"-"} : std::string_view{row.setting_label});
    zelle_sep(lazy_extract_repetition(row.setting_label));
    zelle_sep(std::to_string(row.n_ops));
    zelle_sep(std::to_string(row.total_ns));
    // GOAL-M1.1 (Audit K2): ns_per_op = total_ns / timed_ops (tatsächlich getimte Einzel-Ops; Workload-Pfad
    // = Σ Samples inkl. Scan-Skips, Legacy = 2*n_ops). Der frühere fixe 2*n_ops-Divisor halbierte alle
    // Lastprofil-Zeilen. timed_ops==0 → 0.
    double const ns_per_op =
        (row.timed_ops != 0) ? (static_cast<double>(row.total_ns) / static_cast<double>(row.timed_ops)) : 0.0;
    {
        char      buf[48];
        int const n = std::snprintf(buf, sizeof(buf), "%.3f", ns_per_op);
        zelle_sep(std::string_view{buf, (n > 0) ? static_cast<std::size_t>(n) : 0});
    }
    // GOAL-L1: per-Interface-Funktions-Latenzen (Reihenfolge identisch zum Header / kOpKindNames).
    // INC-29.1 (D2): eine algo-/mess-fehlerhafte Zelle (SampleStatus::Failed) traegt "failed" (NIE 0/still) —
    // "Messung nie als Nullen"; der Ok-Pfad rendert byte-identisch die Zahlen. Failed setzt perm_runner
    // (gate_failed_result_ / catch-OOM). Gleiche Spaltenzahl (3 je Op-Art) -> CSV-Ausrichtung unveraendert.
    // RF-2 (D1, §70.2): eine ZULASSUNGS-gesperrte Perm traegt ihr EIGENES Token (nie "failed" -- W-4:
    // gesperrt heisst NICHT GEMESSEN, failed heisst GEMESSEN UND GESCHEITERT; die Token-Disjunktheit ist
    // in axis_error.hpp compile-time verwacht). Derselbe Zell-Satz, dieselbe Spaltenzahl wie beim
    // D2-Pendant -> CSV-Ausrichtung unveraendert. Default Zugelassen rendert byte-identisch die Zahlen.
    bool const cell_failed = (row.sample_status == ::comdare::cache_engine::measurement::SampleStatus::Failed);
    bool const cell_gesperrt =
        (row.admission_status == ::comdare::cache_engine::measurement::AdmissionStatus::Gesperrt);
    std::string const gesperrt_zelle{::comdare::cache_engine::measurement::admission_status_token(
        ::comdare::cache_engine::measurement::AdmissionStatus::Gesperrt)};
    for (auto const& ol : row.op_lat) {
        if (!zell_ersatz.empty()) {
            // A15/FK-1: nicht gebaut ODER Quelle nicht da -> derselbe Ersatz wie im Rest der Zeile.
            // Vorrang vor gesperrt/failed: beide setzen eine Binary voraus, die es hier nicht gibt.
            zelle_sep(zell_ersatz);
            zelle_sep(zell_ersatz);
            zelle_sep(zell_ersatz);
        } else if (cell_gesperrt) {
            // D1 hat Vorrang vor D2: was nie gemessen wurde, kann nicht "failed" sein.
            out += gesperrt_zelle;
            out += ';';
            out += gesperrt_zelle;
            out += ';';
            out += gesperrt_zelle;
            out += ';';
        } else if (cell_failed) {
            out += "failed;failed;failed;";
        } else {
            out += std::to_string(ol.n);
            out += ';';
            out += std::to_string(ol.p50_ns);
            out += ';';
            out += std::to_string(ol.p99_ns);
            out += ';';
        }
    }
    // (X) die 17 per-Segment-ns (T0..T16, INC-2d) — echt wenn seg_real, sonst ehrlich n/a (NICHT 0). Geschleift über
    // kCompositionAxisNames.size() in derselben Reihenfolge wie die Header-Spalten — single-source, keine Drift.
    // KONSOLIDIERUNG (I-B): seg_<achse>_ns bevorzugt aus dem EINEN konsolidierten POD = Pfad-B-Timing (reale
    // Komposition, User-Entscheid 2026-06-04). Fallback (additive Übergangsphase, entfällt in I-C): row.seg
    // (Pfad A) für noch nicht migrierte Aufrufer; sonst ehrlich n/a (alte DLL ohne konsolidierte tier_observe).
    // E-6/K-10-QW (2026-07-19): der n/a-Zell-Renderer laeuft ueber die EINE D2-Taxonomie (axis_error.hpp:
    // SampleStatus::SourceUnavailable -> sample_status_token() == "n/a", per static_assert dort zementiert)
    // statt eines rohen Literals. CSV-Bytes UNVERAENDERT (golden-neutral); der volle bool-valid->SampleStatus-
    // Split je Zelle bleibt der separate Klein-Increment (Roadmap K-10).
    auto seg_field = [&](int i) {
        if (row.unified_real)
            zelle_sep(std::to_string(row.unified.seg_ns[i])); // Pfad-B-Timing aus dem EINEN POD
        else
            zelle_sep(cem::sample_status_token(
                cem::SampleStatus::SourceUnavailable)); // "n/a": Quelle fehlt (alte/Nicht-Mess-DLL)
    };
    for (std::size_t i = 0; i < kCompositionAxisNames.size(); ++i) seg_field(static_cast<int>(i));
    // P-MD3 (2026-06-18): die Coverage-Versöhnung. seg_framework_ns/seg_run_total_ns ehrlich n/a, wenn keine Mess-DLL;
    // seg_coverage = Σseg_ns / seg_run_total_ns (gegen die KOMMENSURABLE eigene Wall-Clock des Segment-Laufs → ~1.0,
    // NICHT gegen die unkommensurable Real-Workload-total_ns). seg_run_total_ns==0 → coverage n/a (kein Div-by-0).
    if (row.unified_real) {
        std::int64_t seg_sum = 0;
        for (std::size_t i = 0; i < kCompositionAxisNames.size(); ++i) seg_sum += row.unified.seg_ns[i];
        zelle_sep(std::to_string(row.unified.seg_framework_ns));
        zelle_sep(std::to_string(row.unified.seg_run_total_ns));
        if (row.unified.seg_run_total_ns > 0) {
            double const cov = static_cast<double>(seg_sum) / static_cast<double>(row.unified.seg_run_total_ns);
            char         buf[48];
            int const    nb = std::snprintf(buf, sizeof(buf), "%.6f", cov);
            zelle(std::string_view{buf, (nb > 0) ? static_cast<std::size_t>(nb) : 0});
        } else {
            // G6 (W9.5): Coverage bei seg_run_total_ns==0 nicht berechenbar (die Mess-DLL IST da, aber der Nenner
            // ist 0) -> das ist NICHT SourceUnavailable, sondern SampleStatus::NotApplicable ("Wert fuer diese
            // Zelle sinnlos, KEIN Fehler, NIE 0"). Ueber die EINE D2-Taxonomie geleitet (sample_status_token,
            // per static_assert == "n/a") -> CSV-Bytes UNVERAENDERT (golden-neutral), aber die Semantik ist
            // jetzt praezise NotApplicable statt eines rohen Literals. (Der !unified_real-Zweig unten bleibt
            // SourceUnavailable = keine Mess-DLL -> ebenfalls "n/a".)
            zelle(cem::sample_status_token(cem::SampleStatus::NotApplicable));
        }
        out += ';';
    } else {
        zelle_sep("n/a");
        zelle_sep("n/a");
        zelle_sep("n/a");
    }
    // die 4 differenzierten Observer-Counter (search_algo + allocator) — DELTA je Messung (A).
    //
    // M-1/H-B (06.08.2026): DIESE 13 ZELLEN STANDEN ALS EINZIGE DES OBSERVER-BLOCKS UNGESCHUETZT DA.
    // Ihre Nachbarn -- seg_ns, seg_framework_ns/seg_run_total_ns/seg_coverage, stat_<achse>_<feld>,
    // filled_axes -- rendern seit jeher "n/a", wenn unified_real false ist; diese dreizehn schrieben
    // Zahlen aus DEMSELBEN Snapshot. Solange jede Mess-DLL zwingend mit Observer gebaut war, fiel das
    // nicht auf. Seit D-1 ist eine [wallclock]-Binary ohne COMDARE_CE_ENABLE_STATISTICS baubar, und
    // tier_observe liefert dann einen LEEREN POD -- die dreizehn Zellen wuerden literal 0 schreiben,
    // ununterscheidbar von einer echten Messung mit dem Ergebnis 0.
    // Besonders schwer wiegen die letzten zwei: tier_fill_level und observable_axis_count sind gar kein
    // Mess-Zustand (axis_operability_classification.hpp: "passive Build-/Compile-Konstante"), und
    // fill_level=0 ist bei einem real mit 256 Eintraegen gefuellten Tier schlicht falsch -- gemessen.
    // Die Wall-Clock-Zellen der Zeile (total_ns/ns_per_op/op_lat) bleiben bewusst UNBERUEHRT: eine
    // [wallclock]-Messung ist gueltig, sie hat nur keinen Observer. Ein zeilenweiter zell_ersatz waere
    // deshalb falsch; die Ehrlichkeit ist zellgenau.
    auto obs_zelle = [&](std::uint64_t v) {
        if (row.unified_real)
            zelle_sep(std::to_string(v));
        else
            zelle_sep(cem::sample_status_token(cem::SampleStatus::SourceUnavailable)); // "n/a", NIE 0
    };
    obs_zelle(o.search_lookup_count);
    obs_zelle(o.search_hit_count);
    obs_zelle(o.search_miss_count);
    obs_zelle(o.search_insert_count);
    obs_zelle(o.search_erase_count);
    obs_zelle(o.search_peak_occupancy);
    obs_zelle(o.alloc_bytes_allocated);
    obs_zelle(o.alloc_bytes_in_use);
    obs_zelle(o.alloc_allocation_count);
    obs_zelle(o.alloc_deallocation_count);
    obs_zelle(o.alloc_failure_count);
    obs_zelle(o.observable_axis_count);
    obs_zelle(o.tier_fill_level);
    zelle_sep(std::to_string(row.applied_axis_count)); // applied_axes
    // Phase A: die per-Achsen-Observer-Werte stat_<achse>_<feld> (WIDE, generisch aus kV3AxisSchema). Echt wenn
    // unified_real (Modul trägt das Mess-Interface), sonst ehrlich „n/a" (NICHT 0). Reihenfolge IDENTISCH zum Header.
    for (std::size_t t = 0; t < anatomy::kV3AxisCount; ++t) {
        for (std::size_t f = 0; f < anatomy::kV3FieldCount; ++f) {
            if (anatomy::kV3AxisSchema[t].names[f] == nullptr) continue; // ungenutzt / Phase B → keine Spalte
            if (row.unified_real)
                zelle_sep(std::to_string(row.unified.axis_stats[t][f])); // konsolidierter POD
            else
                zelle_sep("n/a"); // alte/Nicht-Mess-DLL -> ehrlich n/a
        }
    }
    if (row.unified_real)
        zelle(std::to_string(row.unified.filled_axis_count)); // filled_axes
    else
        zelle("n/a");
    out += ';';
    zelle(row.profile_name.empty() ? std::string_view{"-"} : std::string_view{row.profile_name}); // workload (Achse 2)
    out += ';';
    zelle(row.two_phase_valid ? "1" : "0"); // Mess-Gueltigkeit (Cache-Warmup)
    // M3v2-SELEKTION (Task #156): die 5 Selektions-/Lauf-Tags (Reihenfolge IDENTISCH zum Header).
    out += ';';
    out += (row.series.empty() ? std::string{"-"} : row.series);
    out += ';';
    out += (row.sweep_axis.empty() ? std::string{"-"} : row.sweep_axis);
    out += ';';
    out += std::to_string(row.working_set_n);
    out += ';';
    out += (row.platform.empty() ? std::string{"-"} : row.platform);
    out += ';';
    out += (row.build_version.empty() ? std::string{"-"} : row.build_version);
    // #171 (2026-06-20): pruefling_type GANZ ANS ENDE (Reihenfolge IDENTISCH zum Header). "-" für Basis/Sweep.
    out += ';';
    out += (row.pruefling_type.empty() ? std::string{"-"} : row.pruefling_type);
    // #165-B (P-MD8, 2026-06-20): quality_flag ALS LETZTE Spalte (Reihenfolge IDENTISCH zum Header). 0 = kein
    // Ausreißer / nicht annotiert (Default); 1 = statistischer Ausreißer (s. annotate_quality_flags). Gate-frei.
    out += ';';
    zelle(std::to_string(row.quality_flag));
    // #156-De-Risk (2026-06-20): die 7 PMC/HW-Counter ALS LETZTE Spalten (Reihenfolge IDENTISCH zum Header). Mit
    // NullPmcSource (COMDARE_ENABLE_PMC=OFF) sind alle Werte 0 und pmc_available=0 (ehrlich „nicht real gemessen");
    // mit Intel-PCM=ON real. Additiv → cowfix-v1/tier150-Leser unberührt (leere PMC-Spalten dort = n-a).
    // B5/M-2-KORREKTUR-2/3 (2026-08-06, Owner-Auflage nach dem AMD-L3-Befund + Owner-KERN "stiller Rueckfall
    // ist verboten"): vier der sieben Zellen (l2, l3, coherence, energy) tragen NUR DANN eine Zahl, wenn ihr
    // EIGENES Quellen-Flag (PmcCounters::*_source_available) sagt, dass GENAU DIESER Zaehler wirklich
    // geoeffnet/gelesen wurde -- eine Quelle, die es auf dieser Plattform nicht gibt (z.B. cache_misses_l3
    // via PERF_TYPE_HW_CACHE/LL auf AMD Zen5, ENOENT) ODER die ohne Zugriffsrecht fehlschlaegt (RAPL-Energy,
    // root-only seit Linux 5.10), rendert die EINE D2-Taxonomie SourceUnavailable/"n/a" (axis_error.hpp)
    // statt einer erfundenen 0. Eine Zahl, die die Quelle wirklich geliefert hat -- auch eine ECHTE 0 --
    // bleibt unveraendert eine Zahl (kein Token verdeckt einen realen Nullbefund). Gilt NUR wenn die Zeile
    // ueberhaupt `pmc.available` ist; die PMC-off-Zeile (NullPmcSource) behaelt ihre bestehende 0-Konvention
    // unveraendert (kein Verhaltenswechsel im Default-Build, in dem praktisch die gesamte Test-/Golden-Flotte
    // laeuft).
    std::string_view const pmc_na    = cem::sample_status_token(cem::SampleStatus::SourceUnavailable);
    auto                   pmc_zelle = [&](std::uint64_t value, bool source_available) {
        if (!zell_ersatz.empty()) {
            out += zell_ersatz;
        } else if (row.pmc.available && !source_available) {
            out += pmc_na;
        } else {
            out += std::to_string(value);
        }
    };
    // B-5 (2026-08-08): l1 und dtlb gingen als EINZIGE der sieben ueber `zelle` statt `pmc_zelle` -- sie
    // konnten damit strukturell nie "nicht erhoben" sagen und trugen bei fehlender Quelle eine stille 0.
    // Auf prod1 fiel das nie auf, weil beide dort oeffnen; auf WINDOWS ist die 0 dagegen heute schon
    // gelogen (windows_pcm_pmc_source.hpp sagt selbst: "L1 bleibt ehrlich 0", und dtlb wird dort nie
    // gesetzt). Jetzt laufen ALLE SIEBEN durch dieselbe Ehrlichkeit -- die Regel hat keine Ausnahme mehr.
    out += ';';
    pmc_zelle(row.pmc.cache_misses_l1, row.pmc.cache_misses_l1_source_available);
    out += ';';
    pmc_zelle(row.pmc.cache_misses_l2, row.pmc.cache_misses_l2_source_available);
    out += ';';
    pmc_zelle(row.pmc.cache_misses_l3, row.pmc.cache_misses_l3_source_available);
    out += ';';
    pmc_zelle(row.pmc.dtlb_misses, row.pmc.dtlb_misses_source_available);
    out += ';';
    pmc_zelle(row.pmc.coherence_invalidations, row.pmc.coherence_invalidations_source_available);
    out += ';';
    pmc_zelle(row.pmc.energy_micro_joules, row.pmc.energy_micro_joules_source_available);
    out += ';';
    zelle(row.pmc.available ? "1" : "0");
    // CMD-2/#252 (2026-07-11): container_store_ops ALS LETZTE Spalte (Reihenfolge IDENTISCH zum Header). Host-seitige
    // Container-in-SA-Attribution (c1 = lookup+insert+erase; ABI-neutral, 0 neue POD-Spalten). unified_real==false ->
    // "n/a" (Phantom-Schutz, exakt wie stat_*/PMC); additiv -> alte CSVs/Leser unberuehrt.
    out += ';';
    if (row.unified_real)
        zelle(std::to_string(container_attribution(row.unified).store_ops));
    else
        zelle("n/a");
    // GO-5 Fork 6 (2026-07-12): fairness_mode (Reihenfolge IDENTISCH zum Header).
    // "-" fuer Basis/Sweep/ungesetzte Reihen; der Wert kommt aus <sota_series fairness=..> via SotaPass.
    out += ';';
    out += (row.fairness_mode.empty() ? std::string{"-"} : row.fairness_mode);
    // GO-5 Fork 7 (2026-07-12): h2_code_quality_score ALS LETZTE Spalte (Reihenfolge IDENTISCH zum
    // Header). Tool-berechneter H2-Score der Reihe / "n/a" (kein Akten-Eintrag) / "-" (Basis/Sweep).
    out += ';';
    out += (row.h2_score.empty() ? std::string{"-"} : row.h2_score);
    // A8-S3 / KLASSE C (2026-08-04) -- die vier END-Append-Bloecke, Reihenfolge IDENTISCH zum Header.
    // (C1) pmc_branch_misses: M-3a VOLLZOGEN (2026-08-07) -- die Zelle traegt jetzt einen REAL erhobenen
    //      Zaehler und faellt in dieselbe pmc_zelle-Ehrlichkeit wie l2/l3/coherence/energy. Der frueher hier
    //      stehende Satz "der Wert ist IMMER der PmcCounters-Default 0" beschrieb den Zustand VOR diesem
    //      Paket und waere jetzt falsch: LinuxPerfPmcSource oeffnet den Zaehler ueber PERF_TYPE_HARDWARE /
    //      PERF_COUNT_HW_BRANCH_MISSES und meldet das Ergebnis in branch_misses_source_available.
    out += ';';
    pmc_zelle(row.pmc.branch_misses, row.pmc.branch_misses_source_available);
    // (C2) p999 je Op-Art: DIESELBE Ersatz-Kaskade wie der op_*-Block oben (nicht_gebaut > gesperrt > failed >
    //      Zahl) -- eine Tail-Spalte darf nicht als einzige eine stille Null zeigen, wenn die Zeile gar keine
    //      Messung ist.
    for (auto const& ol : row.op_lat) {
        out += ';';
        if (!zell_ersatz.empty()) {
            out += zell_ersatz;
        } else if (cell_gesperrt) {
            out += gesperrt_zelle;
        } else if (cell_failed) {
            out += "failed";
        } else {
            out += std::to_string(ol.p999_ns);
        }
    }
    // (C3) T6-Speicher-Ehrlichkeit: KEINE erhobene Quelle am Ist -> ehrlich n/a (G3-Regel), nie eine 0 und nie
    //      der Momentanwert unter Peak-Etikett (Befund B7). Der Token kommt aus der EINEN D2-Taxonomie.
    {
        std::string_view const nicht_erhoben = cem::sample_status_token(cem::SampleStatus::SourceUnavailable);
        for (int i = 0; i < 3; ++i) {
            out += ';';
            out += (zell_ersatz.empty() ? nicht_erhoben : zell_ersatz);
        }
    }
    // (C4) B-5 Runde 2: der amd_l3-Uncore-Wert -- eigene Groesse, eigene Spalte, dieselbe Ehrlichkeit.
    //      Er laeuft durch pmc_zelle wie die anderen sieben: eine Zahl nur, wenn DIESE Quelle geliefert
    //      hat, sonst der Token. Auf prod1 ist das heute "n/a" (der Uncore ist unter paranoid=1
    //      gesperrt -- errno=13, am Objekt gemessen), auf einer Maschine mit CAP_PERFMON eine Zahl.
    out += ';';
    pmc_zelle(row.pmc.l3_miss_uncore_systemweit, row.pmc.l3_miss_uncore_systemweit_source_available);
    // T-15 (2026-08-09) -- die vier Drift-Provenienz-Zellen, Reihenfolge IDENTISCH zum Header.
    //
    // SELBSTCHECK: hier steht NIE eine 0 fuer "keine Aussage". Die Kaskade ist dieselbe wie im ganzen
    // Rest der Zeile: gibt es die Zeile als Messung gar nicht (nicht gebaut / gesperrt / failed), traegt
    // zell_ersatz; war das Gate aus oder die Drift unbestimmbar, traegt der n/a-Token aus der EINEN
    // D2-Taxonomie. Eine Zahl erscheint ausschliesslich dort, wo sie erhoben wurde.
    {
        std::string_view const keine_aussage = cem::sample_status_token(cem::SampleStatus::SourceUnavailable);
        bool const             gate_aus      = (row.drift_reps == 0);
        // (1) drift_reps: die konfigurierte Gruppen-Groesse. 0 ist hier KEIN Ersatz-Wert, sondern die
        //     Aussage "Gate aus" selbst -- deshalb als Zahl, nicht als Token.
        out += ';';
        if (!zell_ersatz.empty()) {
            out += zell_ersatz;
        } else {
            out += std::to_string(row.drift_reps);
        }
        // (2) drift_reruns
        out += ';';
        if (!zell_ersatz.empty()) {
            out += zell_ersatz;
        } else if (gate_aus) {
            out += keine_aussage;
        } else {
            out += std::to_string(row.drift_reruns);
        }
        // (3) drift_relative -- nur mit Nenner (D4). Ohne bestimmbare Drift gibt es keinen Anteil.
        out += ';';
        if (!zell_ersatz.empty()) {
            out += zell_ersatz;
        } else if (gate_aus || !row.drift_bestimmbar) {
            out += keine_aussage;
        } else {
            out += std::to_string(row.drift_relative);
        }
        // (4) drift_status
        out += ';';
        if (!zell_ersatz.empty()) {
            out += zell_ersatz;
        } else if (gate_aus) {
            out += keine_aussage;
        } else if (!row.drift_bestimmbar) {
            out += "unbestimmbar";
        } else {
            out += (row.drift_stabil ? "stabil" : "instabil");
        }
    }
    out += '\n';
    return out;
}

// ── #165-B (P-MD8, 2026-06-20): annotate_quality_flags — der GATE-FREIE statistische Ausreißer-Flag ────────────
// Heuristik (benannt): MEDIAN-MULTIPLIKATOR-AUSREISSER (eng verwandt mit dem "k×Median"-Robust-Filter; der Median
// ist gegen Ausreißer unempfindlich, anders als das arithmetische Mittel). Eine Mess-Zeile gilt als Ausreißer,
// wenn ihr ns_per_op das kQualityOutlierK-fache des Gruppen-MEDIANS überschreitet. GRUPPE = (binary_id, profile_name)
// — d.h. dieselbe Tier-Binary unter demselben Lastprofil; so wird nur gegen vergleichbare Messpunkte verglichen
// (Wiederholungen + dyn-Settings derselben (Binary×Workload)-Zelle), nicht quer über inkommensurable Workloads.
// k = 3.0 (kQualityOutlierK): grob "3× über dem Median" — robuste, konservative Schwelle (analog zur verbreiteten
// 3-fach-MAD/3-Sigma-Daumenregel, hier aber multiplikativ auf den Median, da Latenzen rechtsschief sind).
//
// REIN STATISTISCH + DATENERHALTEND: setzt ausschließlich das additive row.quality_flag-Feld (0/1), berührt KEINE
// bestehende Spalte/keinen Messwert. ns_per_op wird identisch zu format_csv_row berechnet (total_ns/timed_ops;
// timed_ops==0 → 0, fließt nicht in die Median-Basis ein und wird nie geflaggt). Gruppen mit < kQualityMinGroup
// Messpunkten werden NICHT geflaggt (zu wenig Evidenz für eine Ausreißer-Aussage → konservativ 0).
inline constexpr double      kQualityOutlierK = 3.0; // Median-Multiplikator-Schwelle (benannt/dokumentiert)
inline constexpr std::size_t kQualityMinGroup = 3;   // min. Messpunkte je Gruppe für eine Ausreißer-Aussage

inline void annotate_quality_flags(std::vector<LazyMeasuredRow>& rows) {
    // ns_per_op je Zeile (konsistent mit format_csv_row); -1.0 = nicht messbar (timed_ops==0) → nie Flag/Median.
    auto ns_per_op_of = [](LazyMeasuredRow const& r) -> double {
        return (r.timed_ops != 0) ? (static_cast<double>(r.total_ns) / static_cast<double>(r.timed_ops)) : -1.0;
    };
    // (1) Gruppen-Buckets (binary_id|profile_name) → Indizes der zugehörigen Zeilen sammeln.
    std::map<std::string, std::vector<std::size_t>> groups;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        rows[i].quality_flag = 0; // Reset (idempotent — Mehrfachaufruf sicher)
        groups[rows[i].binary_id + "|" + rows[i].profile_name].push_back(i);
    }
    // (2) je Gruppe: Median der gueltigen ns_per_op -> Zeilen > k*Median flaggen.
    // SELBSTCHECK (D5-1, 2026-08-09)
    //   ZUSICHERT: der Gruppen-Median ist der KANON-Fall q=0.5 (stats::nearest_rank_index) und KEINE
    //              eigene Bauart. Vorher stand hier `mid = vals.size() / 2` -- bei GERADER Gruppengroesse
    //              die OBERE Mitte, also eine fuenfte Median-Definition im selben Repo.
    //   ZUSICHERT NICHT: eine Aussage ueber die Guete der Ausreisser-Heuristik selbst (kQualityOutlierK
    //              bleibt unveraendert); nur die Median-DEFINITION wird vereinheitlicht.
    for (auto const& [key, idxs] : groups) {
        std::vector<double> vals;
        vals.reserve(idxs.size());
        for (std::size_t i : idxs) {
            double const v = ns_per_op_of(rows[i]);
            if (v >= 0.0) vals.push_back(v);
        }
        if (vals.size() < kQualityMinGroup) continue; // zu wenig Evidenz → konservativ kein Flag
        std::size_t const mid = commands::stats::nearest_rank_index(vals.size(), 0.5);
        std::nth_element(vals.begin(), vals.begin() + static_cast<std::ptrdiff_t>(mid), vals.end());
        double const median = vals[mid];
        if (median <= 0.0) continue; // degenerierter Median → kein sinnvoller Multiplikator
        double const thr = kQualityOutlierK * median;
        for (std::size_t i : idxs) {
            double const v = ns_per_op_of(rows[i]);
            if (v >= 0.0 && v > thr) rows[i].quality_flag = 1; // statistischer Ausreißer
        }
    }
}

// ── Ergebnis des Lazy-E2E-Laufs (rein zählend + die Mess-Zeilen; kein ∏-Vektor) ──
struct LazyRunResult {
    std::size_t selected = 0; // selektierte statische Blaetter (== min(max_binaries, view))
    // FESTGEZOGEN (TP1-N3/B-2, NACHGEZOGEN T2-A/F4-BILANZ 2026-08-06): built = BEREITGESTELLT im
    // Batch-Sinn -- gebaut ODER lokal resumiert ODER (im planer-getriebenen Bau) als Lager-Bestand
    // belegt uebersprungen ODER (ebendort) vom PLAN-ZAEHLER als bereits kompiliert gedeckt. built_skip
    // ist die SUMME aller DREI Skip-Quellen (dll_is_current-Resume + Lager-Bestand + Plan-Resume); die
    // beiden planer-getriebenen Quellen stehen zusaetzlich einzeln in bestand_lager_skips und
    // plan_resume_skips (unten), damit die Auswertung differenzieren kann.
    //
    // WARUM DER PLAN-RESUME HIERHER GEHOERT (die geheilte Asymmetrie): die vom Plan-Resume
    // uebersprungenen FUEHRENDEN Faecher erreichen den Slice-Loop nie. Bis zur Heilung buchte sie
    // deshalb NIEMAND -- waehrend die Lager-Skips an derselben Stelle sehr wohl gebucht werden --, und
    // ein VOLLSTAENDIG plan-resumierter provision-only-Lauf endete mit built==0 => provision_ok==false
    // => exit 1, obwohl er genau das getan hatte, was der Plan-Resume verspricht. Die Buchung ruht auf
    // demselben Rang von Beleg wie die Lager-Buchung: dort der Lager-Index, hier der Phasen-Zaehler,
    // der ausschliesslich NACH dem Vollzug eines fehlerfreien Fensters fortgeschrieben wird. Keine der
    // beiden Quellen ist eine Platten-Pruefung; dll_is_current bleibt fuer alles, was doch gebaut wird,
    // die zweite Verteidigungslinie.
    std::size_t                  built       = 0; // erfolgreich bereitgestellte DLLs (s. FESTGEZOGEN oben)
    std::size_t                  built_new   = 0; // davon tatsächlich (neu) kompiliert
    std::size_t                  built_skip  = 0; // davon uebersprungen (die drei Quellen aus FESTGEZOGEN)
    std::size_t                  loaded      = 0; // DLLs, die geladen + als IObservableTier nutzbar waren
    std::size_t                  load_failed = 0; // gebaut, aber nicht ladbar / kein Mess-Interface
    std::size_t                  measured    = 0; // gemessene (Binary × dyn-Setting)-Zeilen, in den Baum ge-ingestet
    std::size_t                  dynamic_settings_total = 0; // Σ dyn-Settings über alle geladenen Binaries
    std::uint64_t                min_free_ram_bytes     = 0; // RAM-Low-Water-Mark des Build-Schritts
    std::vector<LazyMeasuredRow> csv_rows;                   // je gemessene (Binary × dyn-Setting)-Zeile (für CSV)
    BuildStats                   build_stats{};              // Roh-Statistik des Build-Schritts (peak_concurrency, …)
    // Mess-RESUME (#139): Binaries, die per vollständiger+aktueller result.csv übersprungen wurden. Ihre
    // Daten-Zeilen (ohne Header, Schema header-identisch verifiziert) stehen UNVERÄNDERT in resumed_csv_rows —
    // der globale CSV-Schreiber hängt sie VOR den frisch formatierten csv_rows an (Spalten identisch).
    // HINWEIS: resumierte Zeilen werden NICHT erneut in den Experiment-Baum ge-ingestet (die Auswertung läuft
    // über die globale CSV; der Baum trägt nur die in DIESEM Lauf frisch gemessenen Knoten).
    std::size_t resumed_binaries = 0;
    std::string resumed_csv_rows;
    // S3 (§62-B COMDARE_PRUEF_ONLY): Ergebnis der Gate-only-Phase. pruef_ok = .so geladen + Gate bestanden;
    // pruef_failed = .so nicht ladbar ODER Gate durchgefallen. NUR im pruef_only-Lauf > 0 (sonst byte-neutral 0).
    std::size_t pruef_ok     = 0;
    std::size_t pruef_failed = 0;
    // TP1-N3 (B-3, additiv): die als LAGER-BESTAND uebersprungenen Binaries des planer-getriebenen
    // Baus -- die zweite Skip-Quelle neben dll_is_current, EINZELN ausgewiesen (E-04-P1 braucht die
    // Differenzierung: dll_is_current-Resumes == built_skip - bestand_lager_skips). 0 ausserhalb des
    // planer-getriebenen Pfads bzw. ohne Praesenz-Praedikat (byte-neutral).
    std::size_t bestand_lager_skips = 0;
    // T2-A/F4-BILANZ (additiv, Muster von bestand_lager_skips): die Atome der vom PLAN-ZAEHLER
    // vollstaendig gedeckten FUEHRENDEN Faecher -- die DRITTE Skip-Quelle. Sie einzeln zu fuehren ist
    // dieselbe Notwendigkeit wie bei der zweiten: ohne sie waere in built_skip nicht mehr trennbar,
    // was ein lokales Sidecar, was der Lager-Index und was der Plan-Zaehler gedeckt hat
    // (dll_is_current-Resumes == built_skip - bestand_lager_skips - plan_resume_skips). 0 ohne
    // benannte Plan-Ablage und 0 ausserhalb des planer-getriebenen Pfads (byte-neutral).
    std::size_t plan_resume_skips = 0;
    // G-E1 / ABNAHME-6 (A1-Lager-Rest-Welle): die Zahl der beim LAUF-START uebernommenen fremden
    // Reservierungen (bestaetigt released, TP1FK1-B4-Revalidierung). Sie ist der Beleg des
    // Claim-Checks: die Fenster dieser Reservierungen liegen per scope_covers_slice VOLL in der
    // Selektion dieses Laufs und werden deshalb ueber den per-Binary-Miss-Weg real NACHGEBAUT.
    // 0 ausserhalb des planer-getriebenen Baus (dort findet kein Claim-Check statt).
    std::size_t bestand_takeover_uebernommen = 0;
};

// W5 (2026-08-05): der Feld-Schluessel des Resume-Stempel-Schwanzes als EINE benannte Konstante. Er stand
// bis hierher ZWEIMAL als nacktes Literal -- im SCHREIBER (result.csv.stamp) und im LESER
// (lazy_try_resume_binary). Ein drittes Auftreten entsteht mit dem status-Rueck-Leser des Planers, der den
// Stand derselben Datei berichtet; ab drei Kopien ist Format-Drift nur noch eine Frage der Zeit. Die Hebung
// ist STRING-IDENTISCH (kein Byte der Emission bewegt sich); der Wert selbst bleibt "|rows=".
inline constexpr char kLazyResumeRowsKey[] = "|rows=";

// T2-A/F4-NB2 (Codex-Voll-Scope, Befund 4) -- DIE FORMAT-MARKE DER RESUME-ZEILE, GEHOBEN.
//
// Dieselbe Hebung und dieselbe Begruendung wie bei kLazyResumeRowsKey darueber, nur fuer den KOPF statt
// den Schwanz: das Literal stand im Schreiber (lazy_resume_stamp_prefix) und wurde vom Runner ueber den
// Praefix-Vergleich als Ganzes mitgeprueft -- der STATUS-LESER des Planers dagegen prueft den Praefix
// bewusst nicht (er kennt die Lauf-Konfiguration nicht) und nahm deshalb einen ALTEN resume-v5-Stamp als
// gueltigen Messstand an, waehrend der echte Runner ihn korrekt verwirft. Zwei Fortschritts-Wahrheiten
// ueber dieselbe Datei.
//
// Die Marke ist KEINE Lauf-Konfiguration, sondern ein FORMAT-Faktum -- genau die Sorte Wissen, die
// MessFormatFakten von der Fassade zum Leser traegt (csv_header, rows_key, und ab jetzt stamp_format).
// Der Leser bleibt damit konfigurations-blind und wird trotzdem versions-scharf.
inline constexpr char kLazyResumeStampFormat[] = "resume-v6";

// ── Mess-RESUME (#139): Config-Stempel + Vollständigkeits-Prüfung der per-Binary result.csv ───────────────
// Der Stempel kodiert ALLES, was die Mess-Matrix einer Binary bestimmt: BuildVersion (Memento-/Code-Stand der
// DLL — copymem-v1-Ergebnisse sind mit undolog-v1 NICHT mischbar), n_ops/seed/records (Workload-Skala) und
// JEDE dynamische Dimension mit ihrer vollen Werte-Liste (deckt repetition/n_repeats, das Workload-Set aus den
// XML-Lastprofilen und alle Resource-Control-Dims ab). Schema-Drift fängt der Header-Vergleich (lazy_csv_header).
[[nodiscard]] inline std::string lazy_resume_stamp_prefix(LazyRunConfig const&           cfg,
                                                          std::vector<DynamicDim> const& dims) {
    // resume-v2 (Audit K8): zusätzlich zur Skala+dyn-Dims jetzt (a) der INHALT der XML-Lastprofile (op-mix/dist/theta/
    // neg/scan je id) — sonst bliebe ein Lauf mit GLEICHER Profil-id aber GEÄNDERTEM XML-Inhalt fälschlich resume-fähig
    // (stale Messung als gültig übernommen); (b) die env_limits (Resource-Control-Caps) — sonst würde ein Lauf mit anderen
    // Limits stale resumed. Format-Bump v1→v2 invalidiert Alt-Stamps via Prefix-Mismatch → ehrliche Neu-Messung.
    // resume-v3 (Task #156): zusätzlich die m3v2-Selektions-Tags (series/sweep_axis/platform/build_version). Sonst
    // würde ein anderer Sweep-Pass (z.B. sweep_axis=migration_policy vs Basis) ODER eine andere SOTA-Reihe mit
    // GLEICHER binary_id+Skala fälschlich resume-fähig — und die getaggte Zeile aus dem falschen Pass übernommen.
    // working_set_n ist bereits über cfg.workload_records (records) im Stamp; die 4 Tags ergänzen die Selektions-Klasse.
    // resume-v4 (#171, 2026-06-20): zusätzlich pruefling_type (full/abstract). Sonst würden eine "full"- (Reihe A,
    // self-contained) und eine "abstract"-Reihe (B/C, Teilmenge) mit GLEICHER view-binary_id+Skala fälschlich
    // gegenseitig resume-fähig — und die getaggte Zeile aus dem falschen Pruefling-Pass übernommen. Format-Bump
    // v3→v4 invalidiert Alt-Stamps via Prefix-Mismatch → ehrliche Neu-Messung (kein stilles Stale-Resume).
    // resume-v5 (GO-5 Fork 6 + Fork 1, 2026-07-12): zusätzlich (a) fairness_mode — sonst wären eine
    // common_denominator- und eine native-Reihe desselben Lebewesens (GLEICHE view-binary_id bis zur DATEN-gated
    // Kompositions-Pinnung) fälschlich gegenseitig resume-fähig; (b) die <datasets>-Deklarations-Signatur
    // (profile_datasets) — eine geänderte Dataset-Deklaration invalidiert alte Stände KONSERVATIV (der
    // Loader-Mess-Konsum ist lauf-gated; bis dahin ehrliche Neu-Messung statt semantisch stalem Resume).
    // Format-Bump v4→v5 invalidiert Alt-Stamps via Prefix-Mismatch (zusätzlich bricht ohnehin die
    // Header-Identität durch die neue fairness_mode-Spalte) → ehrliche Neu-Messung.
    // resume-v6 (T2-A/F4+K2, 2026-08-06): FORMAT-Bump der Resume-Zeile. Er begleitet die Kopplung des
    // Stamps an den VOLLEN Fingerprint (das "|fpr="-Feld, das der per-Binary-Teil unten anhaengt) und die
    // Kopplung des Resume-Anspruchs an b.skipped. Beides aendert die BEDEUTUNG eines Alt-Stamps: er wurde
    // unter einer schwaecheren Zusage geschrieben ("Config gleich" statt "Config UND Identitaet der DLL
    // gleich"). Ein Alt-Stamp darf deshalb nicht weitergelten -- der Praefix-Mismatch invalidiert ihn
    // EINMAL, und der naechste Lauf misst ehrlich neu. Vor Voll-Bau-4 existiert kein schuetzenswerter
    // Mess-Bestand, das ist der guenstigste Moment fuer genau diesen Bump.
    // ABGRENZUNG (Semantik-Leitplanke, bindend): v5->v6 bewegt AUSSCHLIESSLICH die Stempel-ZEILE der
    // Resume-Ablage. Das 8-Glieder-Fingerprint-PREIMAGE bleibt Byte fuer Byte, wie es ist -- der Resume-
    // Stamp KONSUMIERT den Fingerprint, er geht nicht in ihn ein (der Frozen-Vektor ist unbewegt).
    // T2-A/F4-NB2: die Marke kommt aus der EINEN gehobenen Konstante (kLazyResumeStampFormat) -- der
    // Status-Leser prueft gegen genau dieses Byte-Wort, ein zweites Literal hier waere die Drift.
    std::string s = std::string{kLazyResumeStampFormat} + "|build=" + cfg.build_version + "|series=" + cfg.row_series +
                    "|ptype=" + cfg.row_pruefling_type + "|fair=" + cfg.row_fairness_mode +
                    "|sweep=" + cfg.row_sweep_axis + "|plat=" + cfg.row_platform + "|bv=" + cfg.row_build_version +
                    "|n_ops=" + std::to_string(cfg.n_ops) + "|seed=" + std::to_string(cfg.workload_seed) +
                    "|records=" + std::to_string(cfg.workload_records) + "|datasets=" + cfg.profile_datasets +
                    "|env=" + std::to_string(cfg.env_limits.thread_count) + ',' +
                    std::to_string(cfg.env_limits.prefetch_distance) + ',' +
                    std::to_string(cfg.env_limits.pool_budget_bytes) + ',' + std::to_string(cfg.env_limits.batch_size) +
                    ',' + std::to_string(cfg.env_limits.inline_threshold_bytes) + "|wlcfg=";
    // std::map → deterministische id-Sortierung. Je Profil die mess-bestimmenden XML-INHALTSfelder (NICHT seed/n_ops/
    // key_range — die sind Harness-Skala, schon oben). std::to_string(double) ist deterministisch (6 Nachkommastellen).
    for (auto const& [id, c] : cfg.workload_configs) {
        s += id;
        s += ':';
        s += std::to_string(c.pct_insert) + '/' + std::to_string(c.pct_lookup) + '/' + std::to_string(c.pct_erase) +
             '/' + std::to_string(c.pct_clear) + '/' + std::to_string(c.pct_scan) + '/' + std::to_string(c.pct_rmw) +
             '/' + std::to_string(static_cast<int>(c.key_distribution)) + '/' + std::to_string(c.zipfian_theta) + '/' +
             std::to_string(c.negative_query_pct) + '/' + std::to_string(c.scan_length_max);
        s += ';';
    }
    s += "|dims=";
    for (DynamicDim const& d : dims) {
        s += d.axis;
        s += '.';
        s += d.variable;
        s += ':';
        for (std::size_t i = 0; i < d.values.size(); ++i) {
            if (i) s += ',';
            s += d.values[i];
        }
        s += ';';
    }
    return s;
}

/// Prüft, ob `dir/result.csv` für den aktuellen Lauf VOLLSTÄNDIG + AKTUELL ist (Stamp-Match + Header-Identität
/// + Zeilenzahl), und liefert bei Erfolg die Daten-Zeilen (ohne Header) in *out_rows. Jede Abweichung → false
/// (Binary wird normal gemessen — keine stillen Teil-Übernahmen).
[[nodiscard]] inline bool lazy_try_resume_binary(std::filesystem::path const& dir, std::string const& stamp_prefix,
                                                 std::string* out_rows) {
    std::error_code             ec;
    std::filesystem::path const csv_p   = dir / "result.csv";
    std::filesystem::path const stamp_p = dir / "result.csv.stamp";
    if (!std::filesystem::exists(csv_p, ec) || !std::filesystem::exists(stamp_p, ec)) return false;

    std::ifstream sf{stamp_p};
    std::string   stamp;
    if (!sf || !std::getline(sf, stamp)) return false;
    std::string const rows_key = kLazyResumeRowsKey;
    if (stamp.size() <= stamp_prefix.size() + rows_key.size()) return false;
    if (stamp.compare(0, stamp_prefix.size(), stamp_prefix) != 0) return false;           // Config weicht ab
    if (stamp.compare(stamp_prefix.size(), rows_key.size(), rows_key) != 0) return false; // Format weicht ab
    std::uint64_t expected_rows = 0;
    try {
        expected_rows = std::stoull(stamp.substr(stamp_prefix.size() + rows_key.size()));
    } catch (...) { return false; }
    if (expected_rows == 0) return false;

    std::ifstream cf{csv_p};
    if (!cf) return false;
    std::string header_line;
    if (!std::getline(cf, header_line)) return false;
    std::string expected_header = lazy_csv_header();
    while (!expected_header.empty() && (expected_header.back() == '\n' || expected_header.back() == '\r'))
        expected_header.pop_back();
    while (!header_line.empty() && header_line.back() == '\r') header_line.pop_back();
    if (header_line != expected_header) return false; // Schema-Drift → neu messen

    std::string   rows, line;
    std::uint64_t n = 0;
    while (std::getline(cf, line)) {
        while (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        rows += line;
        rows += '\n';
        ++n;
    }
    if (n != expected_rows) return false; // unvollständig/abgeschnitten
    if (out_rows != nullptr) *out_rows = std::move(rows);
    return true;
}

// TP1-N2 (B-1) / TP1FK1-B2 (Codex-Befund CX-W1): die EINE Form des Bestandslog-Eintragspfades.
// Er ist STORE-RELATIV (relativ zum output_dir, generic '/'-Trenner) -- der Push spiegelt exakt diese
// Struktur in den Objekt-Store, und ein maschinen-LOKALER Absolutpfad im GETEILTEN Dokument waere fuer
// die andere Maschine eine Falschangabe. Unrelativierbar (fremde Wurzel) -> voller Pfad als ehrlicher
// Fallback (nie leer, nie geraten; TP1-F2: auch der Fallback in generic-Form, sonst matcht der
// Praefix-Verwurf auf Windows nie).
//
// EINE Quelle, drei Leser: die Vormerkung (observe, ueber die perm.dll), der Pump-Ausschluss und der
// Mess-Pfad-Ausschluss (beide ueber das bin_dir) bilden den String jetzt mit DERSELBEN Funktion.
// discard_fresh_with_pfad_prefix vergleicht MIT Trenner -- weichen Vormerkung und Verwurf in der Form
// auch nur um ein Byte ab, bleibt ein unbestaetigter Eintrag stehen. Die Kopien waren genau dieses
// Risiko.
[[nodiscard]] inline std::string bestand_eintragspfad(std::filesystem::path const& p,
                                                      std::filesystem::path const& output_dir) {
    std::error_code rel_ec;
    auto const      rel = std::filesystem::relative(p, output_dir, rel_ec);
    return (rel_ec || rel.empty()) ? p.generic_string() : rel.generic_string();
}

// TP1FK1-B2 (Codex-Befund CX-W1): der SYNCHRONE Mess-Pfad-Push MIT Bestandslog-Ausschluss bei Wurf.
//
// Der reale Transport WIRFT bei Push-Fehler (ArtefaktPushFehler). Im MESS-Pfad darf das den Lauf nie
// abreissen -- eine Zelle ist gemessen, die CSV liegt lokal, die naechste Zelle wartet -> klassifiziert
// loggen und WEITERMESSEN. Der Faenger war aber FOLGENLOS: das unbedingte bestandslog_flush() am Ende
// des Mess-Laufs registrierte den vorgemerkten Eintrag trotzdem, obwohl der Store den Satz nie erhielt.
// Unter dem Bau-Filter des Folgelaufs ist das der stille Verlustpfad (er skippt eine nirgends
// existierende Binary) -- exakt der Zustand, den B-1 im Pump-Zweig (failed_dirs ->
// discard_fresh_with_pfad_prefix) bereits ausschliesst. Dieser Zweig ist das fehlende Gegenstueck:
// MESSEN WEITER bleibt, der unbestaetigte Bestand nicht.
//
// lager == nullptr (Bestandslog inaktiv) -> es gibt nichts vorzumerken und nichts auszuschliessen; der
// Zweig loggt dann nur, byte-identisch zum reinen Push-Pfad. Der Verwurf ist thread-sicher (eigener
// Mutex in LagerRunState), darf also auch aus dem Debug-Mess-Pool laufen; die Vormerkung geschah in der
// Bau-Phase davor, der Eintrag ist beim Push also da und kann verworfen werden.
inline void mess_pfad_synchron_push(CachePushFn const& cache_push, std::filesystem::path const& bin_dir,
                                    std::string const& build_version, std::string const& binary_id,
                                    std::filesystem::path const& output_dir, bestandslog::LagerRunState* lager) {
    if (!cache_push) return; // No-Op-Naht: ohne Push-Kanal ist nichts zu tun (byte-identisch zum Ist)
    auto const ausschliessen = [&](std::string_view grund) {
        if (lager == nullptr) return; // kein Bestandslog -> kein Eintrag -> nur die Log-Zeile oben
        std::size_t const verworfen =
            lager->discard_fresh_with_pfad_prefix({bestand_eintragspfad(bin_dir, output_dir)});
        std::cerr << "[bestandslog] warn: " << verworfen << " Eintrag/Eintraege fuer binary_id='" << binary_id
                  << "' NICHT registriert (" << grund
                  << ") -- lokale Kopie bleibt, der Store ist unbestaetigt (kein Skip-Anspruch im Folgelauf)\n"
                  << std::flush;
    };
    try {
        cache_push(bin_dir, build_version); // Ebene B: perm.dll(+Sidecars,+.version) -> Store
    } catch (std::exception const& e) {
        std::cerr << "[" << measurement::infra_error_label(measurement::InfraErrorClass::ArtefaktIo) << "] binary_id='"
                  << binary_id << "' Push in den Objekt-Store fehlgeschlagen: " << e.what()
                  << " -- lokale Kopie bleibt, der Lauf MISST WEITER\n"
                  << std::flush;
        ausschliessen("Push-Fehler");
    } catch (...) {
        std::cerr << "[" << measurement::infra_error_label(measurement::InfraErrorClass::ArtefaktIo) << "] binary_id='"
                  << binary_id
                  << "' Push in den Objekt-Store mit unbekanntem Fehler abgebrochen -- lokale Kopie bleibt, der "
                     "Lauf MISST WEITER\n"
                  << std::flush;
        ausschliessen("unbekannter Push-Fehler");
    }
}

// T2-A/F4-NB2 -- DIE EINE KORN-QUELLE DER PLAN-KETTE. Beide Wege, die einen Plan-Stempel bilden (der
// BAU-Weg in run_planer_driven_provision und die MESS-FRONT am Ende von run_lazy_static_then_dynamic),
// holen ihr Korn hier ab. Vorher fuehrte der Bau-Weg es als Funktions-Parameter und der Mess-Weg schrieb
// die Konstante hart hin: zwei Quellen fuer eine Zahl, die im Stempel steht -- und ein Stempel-Glied, das
// auseinanderlaeuft, macht den Plan fuer seinen eigenen zweiten Lauf unsichtbar.
[[nodiscard]] inline std::size_t plan_slice_korn(LazyRunConfig const& cfg) noexcept {
    return (cfg.batch_plan_korn == 0) ? bestandslog::kBuildSliceGrain : cfg.batch_plan_korn;
}

// T2-A/F4-NB2 (Codex-Voll-Scope, Befund 1) -- DIE VOLLZUGS-PRUEFUNG DES ASYNCHRONEN PUSHES.
//
// Sie drainiert den Push-Kanal bis zum Ende des gerade gebauten Fensters und liefert die Zahl der bis
// dahin FEHLGESCHLAGENEN Pushes. Der Bau-Zaehler fragt sie, BEVOR er ein Fenster fortschreibt.
//
// DER BEFUND, den sie schliesst: der Zaehler lief am Fenster-Ende hoch, der Push-Drain lag hinter der
// GANZEN Schleife (push_pump->close()). Ein Fenster, dessen Artefakte der Store nie erhielt, war damit
// im Zaehler als vollzogen gebucht -- und der Folgelauf uebersprang es als Praefix, OHNE die Binaries je
// gesehen zu haben. Die Registrierung im Bestandslog war gegen genau diesen Fall bereits geschuetzt
// (discard_fresh_with_pfad_prefix nach dem Drain, TP1-N2/B-1); der Zaehler war die zweite Buchhaltung,
// die es nicht war -- eine, die dem Lager-Inventar-Cache das GEGENTEIL seiner Aufgabe beibringt.
//
// LEER (kein Push-Kanal) => nichts zu bestaetigen => 0. Ohne Push beschreibt der Zaehler ohnehin nur den
// LOKALEN Bestand dieser Maschine (dieselbe ehrliche Grenze, die bereits ueber bestandslog_flush steht).
using PushVollzugFn = std::function<std::size_t()>;

// T2-A/F4-NB2 (Befund 3) -- DIE EINE QUELLE DER BAU-IDENTITAET FUER DEN PLAN-STEMPEL. Sie hebt die
// FingerprintFn (binary_id -> 128-hex, die Erwartung, gegen die dll_is_current vergleicht) auf die
// view_index-Ebene, die der Plan spricht -- dieselbe Hebung, die make_index_key_fn fuer die PresenceFn
// vornimmt, nur ohne den Umweg ueber das optional (ein unbekannter Fingerprint ist hier der leere String,
// und ein leerer String ist im Preimage genauso bindend wie jeder andere Wert: er sagt "an dieser
// Position war nichts bekannt").
//
// OHNE PROVIDER liefert sie eine LEERE Funktion -- und damit traegt der Stempel `|bau=ohne-anker`. Das
// ist die ehrliche Aussage: ohne Fingerprint deckt dll_is_current nichts, also darf der Zaehler eines
// Laufs MIT Ankern hier nicht gelten (und umgekehrt).
//
// T2-A/F4-NB3 (Befund 1) NACHGEFUEHRT: dieser ankerlose Stempel wird seit dieser Welle NICHT MEHR ABGELEGT.
// Er war fuer JEDEN ankerlosen Bau-Stand derselbe, und der Zaehler-Leser nahm ihn an -- die beiden Welten
// trugen zwar verschiedene Stempel, INNERHALB der ankerlosen Welt aber alle denselben. Die Aufrufer-Seite
// haelt jetzt fail-closed dagegen (plan_anker_befund, unten): benannte Ablage ohne Anker => inert.
//
// WAS DER STEMPEL BINDET UND WER IHN LIEST: die Identitaets-Funktion liefert je view_index GENAU DEN
// Fingerprint, den dll_is_current an dieser Position als Erwartung vergleichen wuerde; plan_bau_digest
// verdichtet die Folge zum Glied `|bau=` des Plan-Stempels. Gelesen wird dieses Glied von zwei Stellen --
// dem Bau-Weg (read_phasen_zaehler entscheidet daran den Plan-Resume, run_planer_driven_provision) und
// dem Mess-Weg (read_batch_plan entscheidet daran, ob die Mess-Front fortgeschrieben wird). Beide fragen
// dieselbe Frage: "stammt dieser Zaehlerstand aus DEMSELBEN Bau-Stand wie dieser Lauf?".
//
// T2-A/F4-NB3, OWNER-DOKTRIN 2026-08-06 (bindend): KEINE KOSTENKLAMMERN. Hier stand bis zu dieser Welle
// ein zweites Glied `cfg.batch_plan_datei.empty()`, begruendet damit, dass der Stempel ohne Plan-Ablage
// "von keiner Stelle gelesen" werde. Diese Begruendungsform gilt nicht mehr: das System ist gross, und
// eine Naht, die heute niemand liest, wird uebermorgen weiterverwertet -- eine Bedingung, die allein
// Rechenzeit spart, kauft diese Ersparnis mit einer Fallunterscheidung, die jeder spaetere Leser erst
// widerlegen muss. Die Funktion ist ab jetzt bedingungsaermer: sie liefert die Identitaet, SOBALD ein
// Provider da ist -- unabhaengig davon, ob eine Ablage benannt ist. EHRLICH BEZIFFERT, was das kostet:
// ein FingerprintFn-Durchlauf ueber die Selektion, EINMAL je Lauf, auch im heutigen Voll-Bau ohne
// Plan-Ablage. Das ist bewusst in Kauf genommen (Groessenordnung: derselbe Durchlauf, den der Miss-Scan
// des Planers und der Bau-Filter des Consumers ohnehin je einmal zahlen).
//
// LEBENSDAUER: view und cfg werden per Referenz gehalten. Beide Aufruf-Stellen bilden den Stempel
// SOFORT -- die Funktion ueberlebt den Ausdruck nicht, in dem sie entsteht.
[[nodiscard]] inline bestandslog::PlanIdentitaetFn plan_identitaet_of(StaticBinaryView const& view,
                                                                      LazyRunConfig const&    cfg) {
    // DIESE EINE BEDINGUNG IST SICHERHEIT, NICHT SPARSAMKEIT -- sie faellt NICHT unter die
    // Kostenklammer-Doktrin und darf nicht mit dem eben entfernten Pfad-Glied verwechselt werden:
    //
    // FingerprintFn ist ein std::function (build_orchestrator.hpp:188) -- ein LEERES std::function ist
    // ansprechbar, aber nicht aufrufbar. Faellt diese Zeile, entsteht hier eine NICHT-leere
    // PlanIdentitaetFn, deren Rumpf ein leeres std::function ruft; plan_bau_digest
    // (planer_driven_build.hpp:301) haelt sie fuer einen gueltigen Anker, betritt die Schleife, und der
    // ERSTE Index wirft std::bad_function_call -- mitten im Bau, aus einer Buchhaltungs-Naht heraus.
    //
    // PRODUKTIV ERREICHBAR, nicht theoretisch: profile_run_entry.hpp LEERT den Provider ohne das
    // COMDARE_BESTANDSLOG-Opt-in (:436), bei unbekannter Tier-Realversion (:438, T2-C) und beim
    // `na`-Zellwert (:1013, W10-C4) -- waehrend bestandslog_active an keinem der drei haengt. Wer diese
    // Bedingung als "doppelt gemoppelt" streicht, baut genau diesen Wurf ein. Der Biss dagegen steht in
    // test_tp1_planer_filter_iterator, Fall (11l-c).
    //
    // Die AUFRUFER-Seite zieht daraus zusaetzlich die fail-closed-Konsequenz: eine benannte Plan-Ablage
    // OHNE Anker bleibt inert, statt einen ankerlosen Stempel abzulegen (s. plan_anker_befund).
    if (!cfg.bestand_fingerprint_fn) return {};
    return [&view, &cfg](std::size_t i) -> std::string { return cfg.bestand_fingerprint_fn(view[i].binary_id); };
}

// T2-A/F4-NB3 (Opus-Zweit-Review 2026-08-06, BEFUND 1 [HOCH]) -- EINE PLAN-ABLAGE OHNE BAU-ANKER IST
// KEINE PLAN-ABLAGE.
//
// DER BEFUND AM OBJEKT: fehlt der Fingerprint-Provider, liefert plan_identitaet_of die leere Funktion und
// plan_bau_digest stempelt das WORT `ohne-anker` statt eines Digests. Damit tragen ZWEI VERSCHIEDENE
// Bau-Staende denselben Stempel -- und der Zaehler-Leser (read_phasen_zaehler) nimmt ihn an. Der Folgelauf
// ueberspringt dann die vom Alt-Zaehler gedeckten FUEHRENDEN Faecher; ihre .so stammen aus einem anderen
// Bau-Stand, und dll_is_current wird nie gefragt, weil diese Faecher gar nicht erst in den Strom gehen
// (SlicePlanner::run, Schritt (4)). Das ist derselbe Leitsatz-Bruch wie Befund 3 -- "der Zaehler-Resume
// darf nie mehr behaupten, als der Fingerprint deckt" --, nur eben ueber den ANKERLOSEN Weg, und er
// geschieht STILL: keine Zeile sagt es an.
//
// PRODUKTIV ERREICHBAR, nicht theoretisch: cfg.bestand_fingerprint_fn wird an zwei Stellen GELEERT --
// beim `na`-Zellwert (profile_run_entry.hpp, W10-C4) und bei unbekannter Tier-Realversion (ebenda, T2-C).
// Beide Male ist das Leeren richtig ("eine unbestimmte Identitaet traegt keinen Skip"); falsch war allein,
// dass die PLAN-Ebene diese Entscheidung nicht mitvollzogen hat: bestandslog_active haengt nicht am
// Provider, also lief die Ablage weiter und zertifizierte ankerlos.
//
// DERSELBE DEFEKT EINE EBENE TIEFER -- PRO ATOM STATT PRO LAUF (Gegenpruefer-Messung, AUFLAGE 2):
// der produktive Provider liefert fuer eine NICHT MATERIALISIERBARE binary_id absichtlich den LEEREN
// String (lazy_adhoc_source_gen.hpp:367 -- "keine DLL, also kein Fingerprint"). Fuer so ein Atom ist
// dll_is_current per Konstruktion blind (expected_fingerprint leer => return false, es skippt NIE, deckt
// also NICHTS). plan_bau_digest hasht den leeren Wert trotzdem mit, und der Zaehler fuehrt das Atom als
// gedeckt. Sind ALLE Atome so, sind zwei verschiedene Bau-Staende wieder stempel-GLEICH -- und der
// Stempel sieht dabei GUELTIG aus (kein `ohne-anker`), der Leser nimmt den fremden Stand also an. Die
// Anker-Wache allein greift dagegen nicht: der Provider IST ja gesetzt.
//
// DIE HEILUNG IST EIN GATE, NICHT ZWEI: dieselbe fail-closed-Klammer traegt beide Ausfallgruende, und
// beide melden ueber DIESELBE beziffernde Zeile. Ist eine Plan-Ablage benannt, aber (a) kein Provider da
// ODER (b) auch nur EIN Atom ohne pruefbare Identitaet, wird KEIN Plan und KEIN Zaehler geschrieben und
// KEINER gelesen -- die Ebene bleibt inert wie bei leerem Pfad, und der Lauf arbeitet ehrlich. Lieber
// keine Ablage als eine, die ein Atom deckt, das dll_is_current nie sieht.
//
// DIE FORM-WAHRHEIT KOMMT AUS IHRER EINEN HEIMAT: detail::fp_is_hex_128 (fingerprint_sidecar.hpp) --
// dieselbe Wache, die der Mess-Resume seit T2-A/K2-NB benutzt (s.u., `|fpr=`-Glied). KEINE zweite
// Formwache; das war der K2-NB-Entscheid, und er gilt hier genauso.
//
// slice_plan_stamp/plan_bau_digest bleiben UNVERAENDERT: das `ohne-anker`-Glied ist dort die ehrliche
// Selbstauskunft einer Zeichenkette, die niemand zum Zertifizieren benutzen darf -- und genau dieses
// "niemand" wird hier durchgesetzt. Nebenbei stimmt damit erstmals die Injektivitaets-Zusage im Kopf von
// plan_bau_digest ("der Trenner liegt ausserhalb des Hex-Alphabets"): eine Eigenschaft, die bis hierher
// behauptet und nirgends geprueft war.
struct PlanAnkerBefund {
    /// Der Plan-Stempel dieses Laufs. BELEGT NUR, wenn traegt() -- ein nicht tragender Befund hat keinen
    /// Stempel, damit ein Aufrufer ihn nicht versehentlich doch ablegen kann.
    std::string stamp;
    bool        provider_fehlt  = false; ///< cfg.bestand_fingerprint_fn ist leer (Ausfallgrund a)
    std::size_t form_verstoesse = 0;     ///< Atome ohne pruefbare 128-hex-Identitaet (Ausfallgrund b)
    std::size_t erster_verstoss = 0;     ///< view_index des ERSTEN davon (nur mit form_verstoesse > 0)

    [[nodiscard]] bool traegt() const noexcept { return !provider_fehlt && form_verstoesse == 0; }
};

// DIE EINE STELLE, DIE ENTSCHEIDET, OB DIESER LAUF EINE PLAN-ABLAGE BEKOMMT -- und die den Stempel
// gleich mitliefert, damit Entscheidung und Ergebnis nicht auseinanderlaufen koennen.
//
// KOSTEN, ehrlich benannt: die Form-Pruefung sitzt IN der Digest-Schleife, in der jeder Wert ohnehin
// GENAU EINMAL geholt wird -- sie kostet also keinen zusaetzlichen Provider-Aufruf. Bewusst wird NICHT
// beim ersten Verstoss abgebrochen: der Durchlauf ist bereits bezahlt, und ein Abbruch machte die
// gemeldete Zahl zur Halbwahrheit ("mindestens einer"). Eine Ersparnis, die eine Diagnose verstuemmelt,
// ist genau die Sorte Klammer, die die Owner-Doktrin vom 2026-08-06 gefaellt hat.
[[nodiscard]] inline PlanAnkerBefund plan_anker_befund(StaticBinaryView const& view, LazyRunConfig const& cfg,
                                                       std::vector<std::size_t> const& indices, std::size_t grain) {
    PlanAnkerBefund                     befund;
    bestandslog::PlanIdentitaetFn const roh = plan_identitaet_of(view, cfg);
    if (!roh) {
        befund.provider_fehlt = true;
        return befund;
    }
    std::string const stamp =
        bestandslog::slice_plan_stamp(indices, grain, [&roh, &befund](std::size_t i) -> std::string {
            std::string wert = roh(i);
            if (!detail::fp_is_hex_128(wert)) {
                if (befund.form_verstoesse == 0) befund.erster_verstoss = i;
                ++befund.form_verstoesse;
            }
            return wert;
        });
    if (befund.form_verstoesse == 0) befund.stamp = stamp;
    return befund;
}

// Die EINE Ansage dazu (nie stumm, beziffert): beide Wege -- Bau und Mess -- melden denselben Sachverhalt
// mit derselben Zeile, damit ein Betreiber ihn im Log nicht zweimal lernen muss. `weg` benennt die Seite,
// der Befund den Grund -- mit Zahl und erstem Index, wo es einen gibt.
inline void melde_plan_ablage_ohne_anker(LazyRunConfig const& cfg, std::size_t indizes, PlanAnkerBefund const& befund,
                                         char const* weg) {
    std::cerr << "[bestandslog] plan-ablage INERT (" << weg << "): " << cfg.batch_plan_datei.string()
              << " ist benannt, aber ";
    if (befund.provider_fehlt)
        std::cerr << "dieser Lauf fuehrt KEINEN Fingerprint-Anker (bestand_fingerprint_fn leer) -- ein Plan-Stempel "
                     "mit dem Glied |bau="
                  << bestandslog::kPlanOhneAnker << " waere fuer JEDEN Bau-Stand derselbe";
    else
        std::cerr << befund.form_verstoesse << " von " << indizes
                  << " Atomen liefern keine pruefbare Bau-Identitaet (nicht 128-hex; erstes: view_index "
                  << befund.erster_verstoss
                  << ") -- ein Stempel, der sie mitrechnet, deckt Atome, die dll_is_current nie sieht";
    std::cerr << " und wuerde einem Folgelauf ein Praefix zertifizieren, das nie geprueft wurde. Es wird kein Plan "
                 "und kein Zaehler geschrieben und keiner gelesen; die "
              << indizes << " Indizes dieses Laufs werden ehrlich bearbeitet\n"
              << std::flush;
}

// #46b I1b (opt-in): den Planer-getriebenen provision-Bau ausfuehren. Ein async Producer (SlicePlanner) slict die
// SELBEN indices in 4096er-Fenster; der Consumer baut je Fenster NUR DIE FEHLENDEN Binaries (Lager-TP1(B)/G-A2:
// filter_window_for_build ueber die PresenceFn -- LEDGER:3397 "jedes fehlende Binary EINZELN erkannt") und
// akkumuliert den builds-Vektor + die BuildStats; Bestands-Treffer zaehlen als Skips (succeeded+skipped+total_jobs,
// dieselbe Buchung wie ein dll_is_current-Hit -- built_skip bleibt "Anzahl der Vorhandenen"). OHNE Praedikat ist
// das Fenster voll -> IDENTISCH zu EINEM provision_all(alle indices); dll_is_current bleibt fuer alles Gebaute
// die zweite Verteidigungslinie. Je Slice ein Reservierungs-Lifecycle (pro-forma -> Kalibrierung eta_s+avg_size
// -> Done) mit PromiseGuard-Release bei Abbruch -- nur wenn Transport+Doc-Key gesetzt (sonst inert); die
// Reservierung beansprucht weiterhin das GANZE Fenster (der Claim dokumentiert das Fenster, nicht seine Luecken).
//
// Ertuechtigung 26.07.: alle drei Reservierungs-Schreibvorgaenge (pro-forma / Done / Release) laufen durch den EINEN
// gelockten Zyklus store_reservation_locked (N7-D1) und ihr Ergebnis wird AUSGEWERTET (B13: der frueher verworfene
// Rueckgabewert machte jeden Store-Fehler unsichtbar). Die pro-forma-Frist ist eine ECHTE 30min-Frist (B11) --
// make_slice_reservation leitet sie aus EINEM now ab. Ein misslungener Eintrag ist NIE ein Bau-Fehler: das
// Bestandslog ist Buchhaltung, der Bau laeuft weiter und die Zahl der Fehlschlaege steht am Ende als EINE Zeile.
// A5/F5 Baupunkte (2)+(3)+(4): DER LAUFENDE KALIBRIER-KANAL EINES SLICES.
//
// Er liegt zwischen den Build-Workern (die je fertigem Compile eine Dauer liefern) und dem Bestandslog
// (das die ETA traegt, SOLANGE der Slice laeuft -- genau das ist der Beweis-Anker der F5-Spez: "die
// Reservierung traegt eta_s VOR Slice-Ende, nicht erst bei Done").
//
// DREI EIGENSCHAFTEN, die nicht verhandelbar sind:
//  * THREAD-SICHER an der Naht: der Completion-Hook feuert aus mehreren Build-Workern gleichzeitig. Der
//    Mutex schuetzt NUR das Sammeln; der Dokument-Schrieb laeuft AUSSERHALB (er ist I/O und darf die
//    anderen Worker nicht am Buchen hindern).
//  * SHARED_PTR, nicht Referenz: der Hook lebt im BuildOrchestrator und damit LAENGER als der Aufruf, der
//    ihn registriert hat. Eine Referenz auf ein Local waere eine baumelnde Referenz, sobald der
//    Bau-Durchlauf zurueckkehrt. Der geteilte Besitz nimmt diese Frage aus dem Kontrollfluss heraus.
//  * INERT AUSSER IM SLICE: aktiv==false heisst, der Hook laesst alles fallen. Das gilt vor dem ersten
//    Fenster, zwischen den Fenstern und nach dem letzten -- ein Ergebnis, das keinem reservierten Slice
//    zugeordnet ist, darf keine fremde Reservierung fortschreiben.
struct SliceEtaKanal {
    explicit SliceEtaKanal(unsigned n_threads) : kal{n_threads} {}

    std::mutex                     mtx;
    bestandslog::EtaKalibrierung   kal; // ueberlebt die Slices; neuer_block() schneidet je Fenster
    bestandslog::BatchReservierung res; // Kopie der LAUFENDEN Reservierung (Status bleibt offen)
    bool                           aktiv     = false;
    std::uint64_t                  offen     = 0;   // noch nicht fertige Binaries DIESES Fensters
    double                         zuletzt_s = 0.0; // zuletzt VEROEFFENTLICHTE ETA (Abweichungs-Trigger)
    std::size_t                    schriebe  = 0;   // gelungene Mid-Slice-Veroeffentlichungen (Testat)
    std::size_t                    fehler    = 0;   // misslungene (Testat -- nie stumm)

    // EINMAL JE BLOCK VEROEFFENTLICHEN -- und warum das keine Sparsamkeit ist, sondern ein BEFUND:
    //
    // Der Record-Union-Merge (bestandslog_lock.hpp, in B2 EINGEFROREN) loest einen id-Konflikt so auf:
    // hoehere Fortschritts-Stufe gewinnt (offen<released<done); bei GLEICHEM Rang gewinnt die mit
    // gefuellter eta_s, und sonst -- also wenn BEIDE eine eta_s tragen -- stabil das REMOTE-Dokument.
    // Am Objekt nachgemessen (merge_documents(remote{offen,eta=100}, lokal{offen,eta=250})): das
    // Ergebnis traegt 100. Die ZWEITE und jede weitere Fortschreibung DERSELBEN offenen Reservierung
    // wird also verworfen -- lautlos, mit einem store(), das true meldet.
    //
    // FOLGE FUER DIESES PAKET: die vom Ledger geforderte Re-Kalibrierung JE BLOCK (LEDGER:3299)
    // funktioniert vollstaendig -- jeder Slice hat seine EIGENE id (owner_uuid/slice_seq), es gibt
    // also gar keinen Konflikt. Was NICHT geht, ist die periodische Fortschreibung INNERHALB eines
    // Slices (F5-Spez Baupunkt 3). Sie braucht entweder eine geaenderte Konflikt-Aufloesung des
    // eingefrorenen Merge oder ein monotones Ordnungs-Feld am Record -- beides ist ein
    // Draht-/Semantik-Entscheid und wird hier NICHT improvisiert (dieselbe Auflage, die
    // builder_registration.hpp fuer den Takeover-Uhr-Anker stellt: "Befund in der Paketmeldung").
    //
    // Bis dahin wird je Block GENAU EINMAL veroeffentlicht. Ein zweiter Schreibvorgang waere kein
    // Fortschritt, sondern ein Aufruf, der nichts tut und dabei den langen Exklusiv-Lock zieht.
    bool veroeffentlicht = false;
};

[[nodiscard]] inline std::vector<BuildResult>
run_planer_driven_provision(BuildOrchestrator& orch, StaticBinaryView const& view,
                            std::vector<std::size_t> const& indices, LazyRunConfig const& cfg, BuildStats& agg,
                            bestandslog::PresenceFn const& present, std::size_t* bestand_skips_out = nullptr,
                            std::size_t* plan_resume_skips_out = nullptr, PushVollzugFn const& push_vollzug = {}) {
    std::vector<BuildResult> builds;
    // T2-A/F4-BILANZ: das KORN wird gefuehrt, nicht hart hingeschrieben -- slice_plan_stamp,
    // plan_alle_faecher und der SlicePlanner nehmen es alle drei laengst entgegen. Wer ein anderes Korn
    // setzt, bekommt es KONSISTENT (Stempel UND Schnitt aus derselben Zahl -- ein zweites Korn waere ein
    // Plan, den sein eigener Leser ablehnt). Gebraucht wird die Naht fuer den TEILWEISEN Plan-Resume: er
    // ist ein Fach-Phaenomen und bei Korn 4096 nur mit >4096 Binaries auszuloesen -- eine Groesse, die
    // kein Modultest ehrlich bauen kann.
    //
    // T2-A/F4-NB2: die Zahl kommt ab jetzt aus cfg.batch_plan_korn und NICHT mehr aus einem eigenen
    // Funktions-Parameter. Der Parameter war die zweite Quelle desselben Wertes: der Mess-Weg
    // (run_lazy_static_then_dynamic, Mess-Front) hat keinen Zugriff darauf und schrieb deshalb
    // kBuildSliceGrain hart hin -- bei abweichendem Korn divergierten die beiden Stempel. Eine Quelle
    // kann nicht divergieren.
    std::size_t const grain = plan_slice_korn(cfg);
    // T2-A/F4: die Plan-Ablage dieses Bau-Laufs. Der rows_key wird HEREINGEREICHT -- kLazyResumeRowsKey ist
    // hier zu Hause, das Bestandslog darf ihn nicht kennen (Abhaengigkeits-Richtung) und soll ihn erst recht
    // nicht ein zweites Mal als Literal fuehren (W5-Hebung). Leere Datei => aktiv()==false => inert.
    // T2-A/F4-NB2 (Befund 3): der Stempel traegt die BAU-IDENTITAET dieses Laufs mit. Ohne sie war der
    // Zaehler eine von dll_is_current unabhaengige zweite Resume-Autoritaet (s. slice_plan_stamp).
    // T2-A/F4-NB3 (Befund 1 + Auflage 2): die FAIL-CLOSED-Klammer. Der Befund wird UNBEDINGT erhoben --
    // keine Pfad-Klammer davor (Owner-Doktrin "keine Kostenklammern"). Traegt er, bekommt die Ablage ihren
    // Stempel; ein leerer Pfad macht sie ohnehin inert (aktiv()==false), das ist ihr eigenes Gate. Traegt
    // er nicht, gibt es KEINE PlanPersistenz -- weder Schreib- noch Lese-Weg. Die Meldung haengt allein
    // daran, dass ueberhaupt eine Ablage BENANNT ist: ohne Namen gibt es nichts, was inert geworden waere
    // (Aussage-Richtigkeit, keine Ersparnis).
    PlanAnkerBefund const       plan_anker = plan_anker_befund(view, cfg, indices, grain);
    bestandslog::PlanPersistenz plan_persistenz;
    if (plan_anker.traegt())
        plan_persistenz =
            bestandslog::PlanPersistenz{cfg.batch_plan_datei, plan_anker.stamp, std::string{kLazyResumeRowsKey}};
    else if (!cfg.batch_plan_datei.empty())
        melde_plan_ablage_ohne_anker(cfg, indices.size(), plan_anker, "bau-weg");
    bestandslog::SlicePlanQueue queue;
    bestandslog::SlicePlanner   planner(queue, indices, grain, present, plan_persistenz);

    bool const reserve = static_cast<bool>(cfg.bestand_transport.store) &&
                         static_cast<bool>(cfg.bestand_transport.fetch) && !cfg.bestand_doc_key.empty();
    // Der Schreib-Kontext dieses Prozesses: Owner-Token (fail-closed, wenn der Host keines gesetzt hat), die ttl aus
    // ihrer EINEN Heimat (LockRecord-Default) und die System-Uhr als NowFn (die Tests skripten sie an der Naht in
    // builder_registration.hpp, nicht hier).
    bestandslog::LockOwner const me     = bestandslog::make_lock_owner(cfg.bestand_owner_uuid, cfg.bestand_maschine);
    int const                    ttl_s  = bestandslog::default_lock_ttl_s();
    bestandslog::NowFn const     now_fn = bestandslog::NowFn{&bestandslog::system_now_epoch_s};
    // Die EINE Reservierungs-Schreibstelle dieses Laufs (gelockt); `was` benennt die Station im Lifecycle.
    auto store_reservation = [&cfg, &me, ttl_s, &now_fn](bestandslog::BatchReservierung const& r,
                                                         std::string_view                      was) {
        return bestandslog::store_reservation_locked(cfg.bestand_transport, cfg.bestand_doc_key, me, ttl_s, now_fn, r,
                                                     was);
    };
    std::size_t res_fehler           = 0; // nicht persistierte Reservierungs-Schreibvorgaenge (Testat am Ende)
    std::size_t bestand_skips_gesamt = 0; // G-A2: als Bestand uebersprungene Binaries (Testat am Ende)

    // -----------------------------------------------------------------------
    // A5/F5 (2)+(3)+(4): DIE LAUFENDE ETA-KALIBRIERUNG.
    //
    // WAS SICH DAMIT AENDERT -- der Kern des Pakets: bisher wurde apply_calibration AUSSCHLIESSLICH
    // unmittelbar vor mark_done gerufen (unten, unveraendert). Die Reservierung trug ihre ETA also erst,
    // wenn sie nicht mehr offen und damit nie mehr uebernehmbar war -- eine Zahl, die niemand mehr
    // brauchte. Ab hier wird WAEHREND des Fensters kalibriert und geschrieben.
    //
    // DIE SCHWELLE kommt aus effective_build_workers und NICHT aus cfg.build_parallelism: letzteres ist
    // 0, wenn kein Override gesetzt ist ("nimm die Heuristik"), und eine Schwelle von 0 hiesse "nach dem
    // ERSTEN Compile veroeffentlichen" -- genau die Hochrechnung aus zu wenigen Punkten, gegen die dieses
    // Paket gebaut ist. effective_build_workers loest die Heuristik auf und liefert die Zahl, die der
    // Bau-Pool wirklich fahren wird -- das ist der Mini-Batch der Doktrin (LEDGER:3290).
    //
    // FUER EIN KLEINES LETZTES FENSTER faellt die Schwelle NICHT: dort werden schlicht nie genug Punkte
    // erreicht, die Kalibrierung bleibt unbelastbar und schreibt nichts. Das ist die gewollte Richtung --
    // lieber keine Zahl als eine aus drei Punkten.
    unsigned const eta_worker = static_cast<unsigned>(orch.config().effective_build_workers(indices.size()));
    auto const     eta_kanal  = std::make_shared<SliceEtaKanal>(eta_worker);
    if (reserve) {
        // add_ statt set_: der Iterator hat den Completion-Hook bereits mit dem Push-/Bestandslog-Zweig
        // belegt. Ein set_ haette ihn STILL verdraengt (s. BuildOrchestrator::add_on_binary_done).
        orch.add_on_binary_done([eta_kanal, &cfg, me, ttl_s, now_fn](BuildResult const& b) {
            bestandslog::BatchReservierung zu_schreiben;
            {
                std::lock_guard<std::mutex> lk(eta_kanal->mtx);
                if (!eta_kanal->aktiv) return;

                // (1) SAMMELN -- IMMER, auch nach der Veroeffentlichung. Die spaeteren Punkte tragen das
                // Block-Testat am Fenster-Ende und die Untergrenze des naechsten Blocks.
                // Der Rest-Zaehler laeuft fuer JEDES Ergebnis herunter (auch Skips und Fehlschlaege): er
                // zaehlt die Fenster-ARBEIT, nicht die Messpunkte.
                if (eta_kanal->offen > 0) --eta_kanal->offen;
                // KEIN Compile == KEIN Messpunkt. dauer_s ist genau deshalb ein optional (s. BuildResult).
                if (!b.dauer_s) return;
                std::optional<std::uint64_t> bytes;
                if (b.ok() && !b.skipped) {
                    std::error_code ec;
                    if (std::filesystem::exists(b.output, ec))
                        bytes = static_cast<std::uint64_t>(std::filesystem::file_size(b.output, ec));
                }
                eta_kanal->kal.beobachte(*b.dauer_s, bytes);

                // (2) VEROEFFENTLICHEN -- genau EINMAL je Block. Ein zweiter Schreibvorgang auf DIESELBE
                // offene Reservierung wird vom eingefrorenen Merge verworfen und zoege dabei den langen
                // Exklusiv-Lock; Herleitung und Messung stehen bei SliceEtaKanal::veroeffentlicht.
                if (eta_kanal->veroeffentlicht) return;
                auto const a = eta_kanal->kal.rest_projektion(eta_kanal->offen);
                if (!bestandslog::eta_neu_schreiben(eta_kanal->zuletzt_s, a)) return;
                eta_kanal->zuletzt_s       = a.eta_s;
                eta_kanal->veroeffentlicht = true;
                zu_schreiben               = eta_kanal->res;
                // uebertrage_kalibrierung statt apply_calibration: hier kann die ZEIT tragen, waehrend
                // noch keine Binary fertig ist -- dann bleibt avg_size_bytes leer statt "0" zu behaupten.
                bestandslog::uebertrage_kalibrierung(zu_schreiben, a);
            }
            // AUSSERHALB des Mutex: der gelockte Dokument-Zyklus ist I/O (ABNAHME-1 nennt ihn den einzigen
            // langen Exklusiv-Fall). Ihn unter dem Sammel-Mutex zu fahren wuerde die anderen Build-Worker
            // fuer seine ganze Dauer am Buchen hindern.
            bool const ok = bestandslog::store_reservation_locked(cfg.bestand_transport, cfg.bestand_doc_key, me, ttl_s,
                                                                  now_fn, zu_schreiben, "ETA-Kalibrierung");
            std::lock_guard<std::mutex> lk(eta_kanal->mtx);
            if (ok)
                ++eta_kanal->schriebe;
            else
                ++eta_kanal->fehler;
        });
    }

    std::size_t slice_seq = 0;
    // T2-A/F4: der Bau-Zaehler DIESES Laufs gegen den persistierten Plan. plan_praefix_intakt haelt fest, dass
    // der Zaehler ein PRAEFIX beschreibt: sobald ein Fenster mit Bau-Fehlern durchlaeuft, darf keine spaetere
    // Zahl mehr hochlaufen -- sonst wuerde der Folgelauf ueber das kaputte Fenster hinweg aufsetzen.
    std::uint64_t plan_kompiliert     = 0;
    std::uint64_t plan_gemessen       = 0;
    std::size_t   plan_faecher        = 0;
    bool          plan_praefix_intakt = true;
    // T2-A/F4-BILANZ: der VORGEFUNDENE Zaehlerstand, getrennt von plan_kompiliert festgehalten -- letzterer
    // laeuft im Fenster-Takt hoch, dieser bleibt die Zahl der Atome, die dieser Lauf GEERBT hat. Genau sie
    // ist die Bilanz-Groesse (plan_resume_skips) und genau sie fehlte bis hierher in den BuildStats.
    std::uint64_t plan_resume_atome = 0;
    // DIE PLAN-WERTE WERDEN GENAU EINMAL GELESEN -- aber an ZWEI Stellen abgeholt, weil es ZWEI gleichwertige
    // Belege dafuer gibt, dass der Planer sie gesetzt hat (s. SlicePlanner::plan_faecher_zahl: "ein Konsument,
    // der bereits ein Fach gezogen hat ODER die geschlossene Queue gesehen hat, liest sie fertig"): das erste
    // gezogene Fach -- und das Ende der Schleife, das nichts anderes bedeutet als eine geschlossene Queue.
    // DER ZWEITE BELEG IST DER GRUND DIESER HEBUNG: ein VOLLSTAENDIG plan-resumierter Lauf zieht kein Fach
    // mehr, betritt den Schleifenrumpf also nie -- und liess bis hierher alle drei Werte auf 0 stehen, samt
    // der geerbten Atome, die er zu Recht als bereitgestellt fuehrt.
    bool plan_werte_gelesen = false;
    auto lies_plan_werte    = [&] {
        if (plan_werte_gelesen || !plan_persistenz.aktiv()) return;
        plan_werte_gelesen = true;
        plan_resume_atome  = planner.resume_atome();
        plan_kompiliert    = plan_resume_atome;
        // T2-A/F4-NB (NAMENS-PIN): BILANZ, nicht Front. Dieser Wert wird ausschliesslich UNVERAENDERT
        // zurueckgeschrieben (s. unten) -- er darf NIE als Praefix-Front konsumiert werden; der
        // Resume-Punkt dieses Bau-Laufs ist allein plan_resume_atome (die Bau-Front).
        plan_gemessen = planner.vorgefundene_mess_front();
        plan_faecher  = planner.plan_faecher_zahl();
    };
    while (auto plan = queue.pop()) {
        lies_plan_werte(); // Beleg 1: ein gezogenes Fach
        bestandslog::BatchReservierung           res;
        std::optional<bestandslog::PromiseGuard> guard;
        if (reserve) {
            // TP1FK1-B1 (Codex-Befund CX-W2): das Fenster als INTERVALL, das die (evtl. luecken-behaftete)
            // Index-MENGE dieses Slices VOLL enthaelt -- begin=min, count=Spanne (slice_window_bounds). Fuer
            // zusammenhaengende Fenster ist das byte-identisch zum frueheren (front(), size()); eine gappy
            // Menge wird KONSERVATIV weiter gefasst, damit scope_covers_slice sie nie faelschlich freigibt.
            auto const fenster_bounds = bestandslog::slice_window_bounds(plan->view_indices);
            if (fenster_bounds.count != plan->view_indices.size())
                // Nie-stumm: die konservative Weitung ist eine ENTSCHEIDUNG und keine stille Eigenschaft.
                // Im Betrieb (dichte Selektion) feuert die Zeile nie.
                std::cerr << "[bestandslog] warn: Slice-Fenster ist NICHT zusammenhaengend (indizes="
                          << plan->view_indices.size() << " spanne=" << fenster_bounds.count << " ab "
                          << fenster_bounds.begin
                          << ") -- die Reservierung beansprucht konservativ die ganze Spanne (kein Draht-Bump)\n"
                          << std::flush;
            res = bestandslog::make_slice_reservation(cfg.bestand_owner_uuid, slice_seq, cfg.bestand_maschine,
                                                      static_cast<unsigned>(cfg.build_parallelism),
                                                      fenster_bounds.begin, fenster_bounds.count, now_fn());
            if (!store_reservation(res, "pro-forma-Reservierung")) ++res_fehler;
            // PromiseGuard: bei Abbruch (Exception/early-return) wird die Reservierung released -> Takeover moeglich.
            // Best-effort im Abbau-Pfad: das Ergebnis ist hier nicht mehr zaehlbar (der Zaehler lebt kuerzer als der
            // Wurf), die Testat-Zeile schreibt der gelockte Zyklus selbst.
            guard.emplace([&store_reservation, res]() mutable {
                bestandslog::mark_released(res);
                (void)store_reservation(res, "Release-Reservierung");
            });
        }

        // G-A2 (Lager-TP1(B)): der BAU-FILTER -- gebaut werden NUR die im Bestand fehlenden Indizes
        // des Fensters (per-Binary einzeln erkannt); ohne Praedikat ist zu_bauen == das volle Fenster
        // (byte-identisch zum Ist-Pfad).
        auto const gefiltert = bestandslog::filter_window_for_build(plan->view_indices, present);
        bestand_skips_gesamt += gefiltert.bestand_uebersprungen;

        // E-04-P1 (Marker-Familie v2, Teil 1a): der Fenster-Token dieses Slices. Die Werte sind GLOBALE
        // StaticBinaryView-Indizes (die Selektion des Prozesses ist bereits das COMDARE_GOLDEN_N_RANGE-
        // Fenster) -- byte-gleich zur emittierten Shell-Koordinate "${START}:${COUNT}". Damit keyt der
        // Aggregator (zelle, fenster) ueber BEIDE Sichten hinweg auf denselben Schluessel.
        std::string const fenster = marker_fenster(
            plan->view_indices.empty() ? std::size_t{0} : plan->view_indices.front(), plan->view_indices.size());
        // VOR dem Bau: das SOLL des Fensters. Beantwortet den Owner-KERN "wie viele davon noch offen"
        // woertlich -- zu_bauen ist die Zahl der real fehlenden Binaries dieses Fensters.
        std::cerr << marker_kopf(kMarkePlanTestat, cfg.marker_kontext, bestandslog::now_utc_iso(), "bau", fenster)
                  << " gesamt=" << indices.size() << " lager=" << gefiltert.bestand_uebersprungen
                  << " zu_bauen=" << gefiltert.zu_bauen.size() << "\n"
                  << std::flush;

        // A5/F5 (4): RE-KALIBRIERUNG JE BLOCK (LEDGER:3299). Der Block-Schnitt liegt VOR dem Bau des
        // Fensters: die Zeiten des vorigen Fensters fallen (eine warm gelaufene oder gedrosselte Maschine
        // soll sich in der naechsten Zahl zeigen), die Untergrenze longest_seen bleibt. zuletzt_s faellt
        // ebenfalls -- jedes Fenster veroeffentlicht seine erste belastbare Zahl unbedingt.
        if (reserve) {
            std::lock_guard<std::mutex> lk(eta_kanal->mtx);
            eta_kanal->kal.neuer_block();
            eta_kanal->res             = res;
            eta_kanal->offen           = static_cast<std::uint64_t>(gefiltert.zu_bauen.size());
            eta_kanal->zuletzt_s       = 0.0;
            eta_kanal->veroeffentlicht = false; // neuer Block, neue Reservierungs-id -> kein Merge-Konflikt
            eta_kanal->aktiv           = true;
        }

        auto const               t0 = std::chrono::steady_clock::now();
        BuildStats               slice_stats;
        std::vector<BuildResult> part =
            orch.provision_all(view, std::span<const std::size_t>{gefiltert.zu_bauen}, &slice_stats);
        auto const t1 = std::chrono::steady_clock::now();

        // Der Kanal ist ab hier stumm: was jetzt noch feuern wuerde, gehoert zu keinem laufenden Fenster.
        bestandslog::EtaAuskunft eta_schluss;
        std::size_t              eta_schriebe = 0;
        std::size_t              eta_fehler   = 0;
        if (reserve) {
            std::lock_guard<std::mutex> lk(eta_kanal->mtx);
            eta_kanal->aktiv = false;
            eta_schluss      = eta_kanal->kal.block_auskunft();
            eta_schriebe     = eta_kanal->schriebe;
            eta_fehler       = eta_kanal->fehler;
        }
        // Die Wall-Clock des Fensters wird jetzt UNABHAENGIG vom Reservierungs-Zweig gebraucht (die
        // Bilanz-Zeile traegt sie auch ohne aktives Bestandslog) -- dieselbe Zahl, die apply_calibration
        // unten als eta_s in die Reservierung schreibt (deshalb KEIN zweites, redundantes eta_s-Feld).
        double const wall_s = std::chrono::duration<double>(t1 - t0).count();

        // BuildStats aggregieren (jede Binary genau einmal -> Summe == EINE-provision_all-Statistik).
        // Bestands-Skips buchen wie dll_is_current-Hits (ok + skipped + gezaehlter Job): built_skip
        // bleibt damit "Anzahl der Vorhandenen", built_new "Anzahl der Fehlenden" -- nur die QUELLE
        // des Skips ist eine andere (Lager statt lokales Sidecar).
        agg.total_jobs += slice_stats.total_jobs + gefiltert.bestand_uebersprungen;
        agg.peak_concurrency = (std::max)(agg.peak_concurrency, slice_stats.peak_concurrency);
        agg.succeeded += slice_stats.succeeded + gefiltert.bestand_uebersprungen;
        agg.failed += slice_stats.failed;
        agg.skipped += slice_stats.skipped + gefiltert.bestand_uebersprungen;
        agg.built += slice_stats.built;
        agg.min_free_ram_bytes = (std::min)(agg.min_free_ram_bytes, slice_stats.min_free_ram_bytes);

        // NACH dem Bau: das IST des Fensters (E-04-P1, Teil 1a). Quelle sind AUSSCHLIESSLICH die
        // slice_stats dieses Fensters + die Filter-Zahl -- der Treiber ist die einzige Zaehler-Quelle
        // des Kanals (Single-Source; die Shell zaehlt nichts nach).
        std::cerr << marker_kopf(kMarkeBilanzTestat, cfg.marker_kontext, bestandslog::now_utc_iso(), "bau", fenster)
                  << " gebaut_neu=" << slice_stats.built << " sidecar_skip=" << slice_stats.skipped << " lager_skip="
                  << gefiltert.bestand_uebersprungen
                  // T2-A/F4-BILANZ: EINE Grammatik fuer die ganze Marke -- die dritte Skip-Quelle steht in
                  // JEDER [BILANZ-TESTAT]-Zeile, auch wo sie 0 ist. Ein Fenster, das den Loop erreicht hat,
                  // ist per Definition NICHT plan-resumiert; die 0 ist hier die Wahrheit, keine Ersatzzahl.
                  << " plan_skip=0"
                  << " fehl=" << slice_stats.failed << " dauer_s=" << bestandslog::format_seconds(wall_s) << "\n"
                  << std::flush;

        if (reserve) {
            std::vector<std::uint64_t> sizes; // avg_size ueber die FRISCH gebauten Binaries dieses Blocks
            std::error_code            ec;
            for (auto const& b : part)
                if (b.ok() && !b.skipped && std::filesystem::exists(b.output, ec))
                    sizes.push_back(static_cast<std::uint64_t>(std::filesystem::file_size(b.output, ec)));
            bestandslog::apply_calibration(res, bestandslog::EtaResult{wall_s, bestandslog::average_size_bytes(sizes)});
            bestandslog::mark_done(res);
            if (!store_reservation(res, "Done-Reservierung")) ++res_fehler;
            if (guard) guard->commit(); // erfolgreicher Slice -> kein Release

            // A5/F5: das Kalibrier-Testat DIESES Fensters. Eigene Zeile im [bestandslog]-Kanal -- die
            // Marker-Grammatiken ([PLAN-TESTAT]/[BILANZ-TESTAT]) werden NICHT angefasst: sie werden
            // maschinell gelesen, und ein zusaetzliches Feld darin waere eine Grammatik-Aenderung.
            // Die VORHERSAGE (block_auskunft) steht neben der MESSUNG (wall_s) -- so ist die Guete der
            // Schaetzung im Log selbst nachrechenbar und nicht nur behauptet. Ohne belastbare Grundlage
            // steht dort "n/a", nie eine Zahl.
            // Das Fenster heisst hier BEWUSST slice= und nicht fenster=: `fenster=` ist ein Pflichtfeld
            // der Marker-Familie, und Wachen zaehlen sein Vorkommen ueber den ganzen Log-Strom
            // (test_tp1_planer_filter_iterator: " fenster=0:8 " genau zweimal = einmal je Marker-Zeile).
            // Wer sich aus einer Grammatik heraushaelt, haelt sich auch aus ihren Feldnamen heraus --
            // sonst faelscht eine Zusatz-Zeile die Zaehlung einer fremden Wache.
            std::cerr << "[bestandslog] eta-kalibrierung: slice=" << fenster
                      << " vorhersage_s=" << bestandslog::eta_text(eta_schluss)
                      << " gemessen_s=" << bestandslog::format_seconds(wall_s) << " punkte=" << eta_schluss.punkte
                      << "/" << eta_schluss.noetige_punkte << " schriebe=" << eta_schriebe << " fehl=" << eta_fehler
                      << "\n"
                      << std::flush;
        }
        // T2-A/F4: DER ZAEHLER LAEUFT NACH DEM VOLLZUG HOCH, NICHT DAVOR. Gezaehlt werden die ATOME des
        // Fensters (seine view_indices) -- Lager-Treffer eingeschlossen, denn auch fuer sie gilt die Aussage
        // "hier ist nichts mehr zu bauen". Ein Fenster mit auch nur EINEM Bau-Fehler bricht das Praefix: die
        // Zahl bleibt stehen, und der Folgelauf faengt genau dort wieder an. Ein misslungener Zaehler-Schrieb
        // ist -- wie die Reservierungs-Buchhaltung -- KEIN Bau-Fehler, aber er ist nie stumm.
        //
        // T2-A/F4-NB2 (Befund 1) -- "VOLLZUG" HEISST AB JETZT AUCH: DIE ARTEFAKTE SIND IM STORE. Der
        // asynchrone Push lief bis hierher NEBEN dem Zaehler her und wurde erst hinter der GANZEN Schleife
        // drainiert (push_pump->close()); ein Fenster, dessen Pushes samt und sonders warfen, stand
        // trotzdem als vollzogen im Zaehler -- und der Folgelauf uebersprang es, ohne dass irgendwo eine
        // Binary lag. push_vollzug() drainiert den Kanal bis zum Fenster-Ende und meldet die Fehl-Pushes;
        // ein einziger bricht das Praefix wie ein Bau-Fehler. Die Zahl ist KUMULATIV (der Pump zaehlt ueber
        // den Lauf) -- das ist bewusst konservativ: nach dem ersten Fehl-Push wandert kein Fenster mehr in
        // den Zaehler, auch wenn ein spaeteres fuer sich gelungen waere. Fail-closed in die richtige
        // Richtung: zu wenig Resume kostet einen Nachbau, zu viel kostet ein Loch in der Matrix.
        if (plan_persistenz.aktiv() && plan_praefix_intakt) {
            std::size_t const push_fehl = push_vollzug ? push_vollzug() : std::size_t{0};
            if (slice_stats.failed == 0 && push_fehl == 0) {
                plan_kompiliert += static_cast<std::uint64_t>(plan->view_indices.size());
                if (!bestandslog::write_phasen_zaehler(plan_persistenz.zaehler_datei(), plan_persistenz.stamp,
                                                       bestandslog::PhasenZaehler{plan_kompiliert, plan_gemessen},
                                                       plan_faecher, plan_persistenz.rows_key))
                    std::cerr << "[bestandslog] warn: Bau-Zaehler des Batch-Plans nicht persistiert ("
                              << plan_persistenz.zaehler_datei().string()
                              << ") -- der Bau ist unberuehrt, der Folgelauf setzt frueher auf\n"
                              << std::flush;
            } else if (slice_stats.failed != 0) {
                plan_praefix_intakt = false;
                std::cerr << "[bestandslog] plan-zaehler: Fenster mit " << slice_stats.failed
                          << " Bau-Fehlern -- der Zaehler bleibt bei " << plan_kompiliert
                          << " Atomen stehen (Praefix-Resume, kein Ueberspringen des Fehl-Fensters)\n"
                          << std::flush;
            } else {
                // Nie-stumm, und mit der EIGENEN Begruendung: gebaut wurde alles, nur angekommen ist es
                // nicht. Wer diese Zeile sieht, sucht am Store-Transport und nicht am Compiler.
                plan_praefix_intakt = false;
                std::cerr << "[bestandslog] plan-zaehler: Fenster fehlerfrei GEBAUT, aber " << push_fehl
                          << " Push-Fehler bis hierher -- der Zaehler bleibt bei " << plan_kompiliert
                          << " Atomen stehen (die Artefakte sind NICHT im Store; ein Zaehler-Schritt haette "
                             "dem Folgelauf Binaries versprochen, die es dort nicht gibt)\n"
                          << std::flush;
            }
        }
        for (auto& b : part) builds.push_back(std::move(b));
        ++slice_seq;
    }
    lies_plan_werte(); // Beleg 2: die Schleife ist zu Ende == die Queue ist geschlossen (s. oben)

    // ---------------------------------------------------------------------------------------------
    // T2-A/F4-BILANZ: DIE GEERBTEN ATOME BUCHEN.
    // Die vom Plan-Zaehler gedeckten FUEHRENDEN Faecher haben den Slice-Loop nie erreicht, also hat oben
    // niemand ihre Atome gebucht. Hier geschieht das -- mit EXAKT der Buchung, die die Lager-Skips im
    // Loop erhalten (succeeded + skipped + total_jobs, `built` unberuehrt, weil nichts NEU kompiliert
    // wurde). Damit haelt die Definition von LazyRunResult::built wieder ueber alle drei Skip-Quellen,
    // und ein vollstaendig plan-resumierter provision-only-Lauf ist das, was er ist: ein gueltiger Lauf.
    agg.total_jobs += static_cast<std::size_t>(plan_resume_atome);
    agg.succeeded += static_cast<std::size_t>(plan_resume_atome);
    agg.skipped += static_cast<std::size_t>(plan_resume_atome);

    // DAS ZUGEHOERIGE TESTAT (Nie-stumm; 0-Fall schweigt, wie das G-A2-Testat darunter). Ohne diese Zeile
    // wuerde eine Bilanz aufgehen, die niemand nachrechnen kann -- und der VOLL resumierte Lauf haette im
    // planer-getriebenen Pfad ueberhaupt KEINE [BILANZ-TESTAT]-Zeile, waehrend er zugleich exit 0 meldet.
    // Das Fenster ist exakt bestimmbar: der Plan schneidet `indices` in Reihenfolge, die gedeckten Faecher
    // sind sein PRAEFIX -- also die ersten plan_resume_atome Indizes ab indices.front(). dauer_s fehlt aus
    // demselben Grund wie in der Fallback-Zeile: dieser Lauf hat an diesen Atomen keine Zeit verbracht,
    // und eine erfundene waere schlimmer als keine.
    if (plan_resume_atome > 0) {
        std::string const resume_fenster = marker_fenster(indices.empty() ? std::size_t{0} : indices.front(),
                                                          static_cast<std::size_t>(plan_resume_atome));
        std::cerr << marker_kopf(kMarkeBilanzTestat, cfg.marker_kontext, bestandslog::now_utc_iso(), "bau",
                                 resume_fenster)
                  << " gebaut_neu=0 sidecar_skip=0 lager_skip=0 plan_skip=" << plan_resume_atome << " fehl=0\n"
                  << std::flush;
        std::cerr << "[bestandslog] plan-bilanz: " << plan_resume_atome << " Atome aus " << plan_faecher
                  << " geplanten Faechern vom Phasen-Zaehler gedeckt -- als bereitgestellt gebucht (dieselbe "
                     "Buchung wie ein Lager-Skip; der Beleg ist der Zaehler, nicht die Platte)\n"
                  << std::flush;
    }
    // B13-Testat: die Fehlschlaege der Buchhaltung EINMAL beziffert (je Slice zwei planmaessige Schreibvorgaenge).
    // Kein Erfolgs-Haken ohne Ausgabe -- und kein Bau-Abbruch: die Binaries dieses Laufs sind davon unberuehrt.
    if (res_fehler > 0)
        std::cerr << "[bestandslog] warn: " << res_fehler << " von " << (2 * slice_seq)
                  << " Reservierungs-Schreibvorgaengen nicht persistiert (Zeilen oben) -- der Bau ist unberuehrt\n"
                  << std::flush;
    // G-A2-Testat (Nie-stumm): der Bau-Filter weist seine Skips EINMAL beziffert aus. 0-Fall schweigt
    // (Normalbetrieb ohne Bestand bzw. ohne Praedikat bleibt zeilen-identisch zum Ist).
    if (bestand_skips_gesamt > 0)
        std::cerr << "[bestandslog] bau-filter: " << bestand_skips_gesamt
                  << " Binaries als Bestand uebersprungen (per-Binary-Miss, LEDGER:3397) -- dll_is_current bleibt "
                     "zweite Verteidigungslinie\n"
                  << std::flush;
    // TP1-N3 (B-3): die Lager-Skip-Zahl reist zum Aufrufer (LazyRunResult.bestand_lager_skips) --
    // builds traegt nur die GEBAUTEN, die Skips waeren sonst im Ergebnis unsichtbar.
    if (bestand_skips_out != nullptr) *bestand_skips_out = bestand_skips_gesamt;
    // T2-A/F4-BILANZ: dieselbe Reise fuer die dritte Skip-Quelle (LazyRunResult.plan_resume_skips) --
    // aus demselben Grund: builds traegt sie nicht, und in built_skip allein waere sie nicht trennbar.
    if (plan_resume_skips_out != nullptr) *plan_resume_skips_out = static_cast<std::size_t>(plan_resume_atome);
    return builds; // Producer-Thread joined im SlicePlanner-dtor (RAII)
}

/// run_lazy_static_then_dynamic — DIE EINE fehlende Host-Treiber-Funktion. Verdrahtet die volle Lazy-Kette:
/// (1) Haupt/statisch-Iterator über view+selection → BuildOrchestrator (STATISCHE Kompilierung, resumierbar/RAM-gated),
/// (2) je DLL load → IObservableTier + IResourceControllableTier,
/// (3) gefiltert-dynamisch-Iterator (RuntimeVariableLoop über tree.dynamic_filter()) → messen → ingest.
/// `sel` liefert die endlichen View-Indizes (z.B. select_explicit(first N) / select_one_wise(view)); es wird auf
/// die ersten cfg.max_binaries gekappt. compile/gen/ram werden injiziert (Engine-agnostisch wie BuildOrchestrator).
[[nodiscard]] inline LazyRunResult run_lazy_static_then_dynamic(ExperimentTree& tree, BuildSelection const& sel,
                                                                CompileFn compile, SourceGenFn gen, FreeRamFn ram,
                                                                LazyRunConfig const& cfg, AlgoSigFn algo_sig = {}) {
    LazyRunResult                 result;
    StaticBinaryView const        view     = tree.static_binary_view();
    std::vector<DynamicDim> const dyn_dims = tree.dynamic_filter(); // der dynamische Sub-Filterbaum (LAZY-Quelle)

    // ── Selektion auf die ersten N kappen (lazy: NIE die ganze ∏-View materialisieren) ──
    std::vector<std::size_t> indices = sel.indices;
    if (indices.size() > cfg.max_binaries) indices.resize(cfg.max_binaries);
    result.selected = indices.size();
    if (indices.empty()) return result;

    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    // (1) HAUPT- / STATISCH-ITERATOR: je selektiertes Blatt LAZY view[i] → DLL bauen (STATISCHE Kompilierung).
    //     BuildOrchestrator dekodiert je Blatt genau EINE BinarySpec on-demand, generiert die REALE Anatomie-
    //     Source (injizierte SourceGenFn) + kompiliert sie (injizierte CompileFn) — multithreaded, RAM-gated,
    //     resumierbar (.version-Sidecar). results-Vektor ist O(K=|indices|), NICHT O(∏) (L-73).
    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    BuildConfig bcfg;
    bcfg.cores_per_build         = cfg.cores_per_build;
    bcfg.source_dir              = cfg.source_dir;
    bcfg.output_dir              = cfg.output_dir;
    bcfg.build_version           = cfg.build_version;
    bcfg.ram_per_build_bytes     = cfg.ram_per_build_bytes;
    bcfg.ram_safety_margin_bytes = cfg.ram_safety_margin_bytes;
    bcfg.per_binary_subdirs      = cfg.per_binary_subdirs; // (E): je Tier-Binary ein eigener Unterordner
    bcfg.build_parallelism       = cfg.build_parallelism;  // W6 (§32-F7): expliziter Bau-Pool-Worker-Override (0=heute)
    bcfg.build_variant_sig       = cfg.build_variant_sig;  // A7-B: Treiber-Mengen-Signatur; leer = Variant-Gate AUS

    // Storage #51 / S2 (#46a) — PULL-HOOK (AKTIV): die BATCH-Warm-Cache-Hydrierung (minio->local) laeuft GENAU HIER in
    // Phase A, VOR dem Bau. Sie zieht den ganzen Objekt-Store-Praefix DIESER Perm rekursiv ins output_dir (unter
    // <stem>/perm.dll(+ die optionalen Sidecars +.version)), sodass der Orchestrator sie je Binary via dll_is_current
    // als FINGERPRINT-aktuell erkennt und den Rebuild ueberspringt. Korrektheit entscheidet danach AUSSCHLIESSLICH lokal
    // dll_is_current (ein False-/Teil-Pull => kein/kein passendes `.fingerprint` => Neubau). [NACHGEFUEHRT 2026-08-05,
    // A2-Eichung, HISTORIK: "als versions-aktuell ... kein/kein passendes .version/.algos". Der Anker reist ueber
    // kOptionalTierSidecars mit -- ohne ihn skippt hier nichts mehr, fail-closed.] Gilt in BEIDEN Modi
    // (BAU + MESS: die Perm muss ohnehin materialisiert sein). Leer (Default) => No-Op => golden/CI byte-identisch; scharf
    // nur, wenn der Host cfg.cache_pull belegt (Env/CI). Nur mit per_binary_subdirs (der <stem>/-Layout-Konvention, die
    // dll_is_current UND der Push teilen). EIN mc-Prozess (nicht x|Binaries| Spawns; Dossier Option (a)).
    if (cfg.cache_pull && cfg.per_binary_subdirs) cfg.cache_pull(cfg.output_dir, cfg.build_version);

    // #46b I1 (opt-in): das Bestandslog VOR dem Bau konsultieren -- den vorbestehenden Lager-Bestand laden (Dedup-
    // Basis fuer die per-Binary-Klassifikation + Merge-Basis fuer die Registrierung). Aktiv NUR, wenn Transport
    // (fetch+store) + Doc-Key + Key-Provider gesetzt sind; sonst No-Op => byte-neutral (Muster wie cache_pull).
    bool const bestandslog_active = static_cast<bool>(cfg.bestand_transport.fetch) &&
                                    static_cast<bool>(cfg.bestand_transport.store) &&
                                    static_cast<bool>(cfg.bestand_key_of) && !cfg.bestand_doc_key.empty();
    bestandslog::LagerRunState lager;
    if (bestandslog_active) lager.load(cfg.bestand_transport, cfg.bestand_doc_key);
    // G-E3: der Messwert-Genus-Zustand. EIGENES Gate (eigener doc_key + eigener Key-Provider), damit
    // ein Host das Binary-Lager fahren kann, ohne das Messwert-Lager zu fuehren -- und umgekehrt.
    bool const mess_bestandslog_active =
        static_cast<bool>(cfg.bestand_transport.fetch) && static_cast<bool>(cfg.bestand_transport.store) &&
        static_cast<bool>(cfg.mess_bestand_key_of) && !cfg.mess_bestand_doc_key.empty();
    bestandslog::MesswertRunState mess_lager;
    if (mess_bestandslog_active) mess_lager.load(cfg.bestand_transport, cfg.mess_bestand_doc_key);

    // G-E1 (ABNAHME-6): der TAKEOVER-SWEEP am Lauf-Start -- fremde offene Reservierungen pruefen,
    // verfallene uebernehmen (released; die unverzeichnete Arbeit baut dieser Lauf ueber den
    // per-Binary-Miss-Weg). Unter DEMSELBEN Gate wie die Reservierungs-Schreibvorgaenge selbst
    // (planer_driven_active): Reservierungen sind ein Bau-Batch-Konzern, und im Mess-Lauf haette der
    // Roundtrip in der 1-Thread-Exklusivitaet nichts verloren. Ein Fehlschlag ist NIE ein Bau-Fehler
    // (Buchhaltung; Testat-Zeilen schreibt der Sweep selbst).
    //
    // TP1FK1-B1: der Sweep ist an DIESEN Lauf GEKOPPELT. Er urteilt nur ueber Reservierungen vom Typ
    // `tier` -- denselben Typ, den run_planer_driven_provision hier schreibt (make_slice_reservation)
    // -- und nur ueber Fenster, die die Selektion `indices` dieses Laufs voll abdeckt. Beides folgt
    // aus derselben Ueberlegung: ein Release ist nur dann korrekt, wenn dieser Lauf die freigegebene
    // Arbeit ueber den per-Binary-Miss-Weg auch wirklich aufnimmt. Ein planer_block (CEB-Compile) und
    // ein Fenster ausserhalb von `indices` erfuellen das nicht -> stehen lassen.
    //
    // G-E1 / 2.4-(8) (A1-Lager-Rest-Welle): der Sweep ist der CLAIM-CHECK. Sein Ergebnis wird nicht
    // mehr verworfen, sondern (a) im Lauf-Ergebnis gefuehrt (bestand_takeover_uebernommen) und
    // (b) mit einer eigenen Testat-Zeile belegt, die die zweite Haelfte der Uebernahme AUSSPRICHT:
    // die uebernommenen Fenster liegen per scope_covers_slice VOLL in `indices` -- dieser Lauf baut
    // sie also ueber den per-Binary-Miss-Weg real nach. Ohne diese Zeile war "uebernommen" eine
    // Buchung ohne sichtbare Arbeitsaufnahme. 0-Fall schweigt (Normalbetrieb bleibt zeilen-identisch).
    //
    // Beifang Section 75-(23), NUR DOKUMENTIERT (Fix gehoert in den Aufraeumpass): der Aufruf steht
    // ohne try/catch. takeover_expired_reservations wirft nach heutigem Stand nicht (alle Ausgaenge
    // sind Rueckgabewerte), aber die Transport-Lambdas des Hosts koennen es -- ein Wurf hier risse
    // den BAU ab, obwohl das Bestandslog Buchhaltung ist. Der Faenger ist bewusst nicht Teil dieser
    // Scheibe (kein Scope-Creep), sondern benannt.
    //
    // planer_block-REAPER bleibt DEKLARIERT OFFEN (TP1FK1-B1-Folgebeobachtung): der Tier-Sweep laesst
    // planer_block-Reservierungen ausdruecklich stehen (typ_fremd). Ein eigener Reaper wird erst
    // sinnvoll, wenn der Zweit-Planer-Konsument (profile_run_facade.hpp:248) real wird -- vorher
    // gaebe es niemanden, der die freigegebene CEB-Arbeit aufnimmt.
    if (bestandslog::planer_driven_active(bestandslog_active, cfg.provision_only)) {
        auto const claim = bestandslog::takeover_expired_reservations(
            cfg.bestand_transport, cfg.bestand_doc_key,
            bestandslog::make_lock_owner(cfg.bestand_owner_uuid, cfg.bestand_maschine),
            bestandslog::default_lock_ttl_s(), bestandslog::NowFn{&bestandslog::system_now_epoch_s},
            bestandslog::make_sweep_scope(bestandslog::BatchTyp::tier, std::span<std::size_t const>{indices}));
        result.bestand_takeover_uebernommen = claim.uebernommen;
        if (claim.offene_fremde > 0 || claim.uebernommen > 0)
            std::cerr << "[bestandslog] claim-check: " << claim.offene_fremde
                      << " fremde offene Reservierung(en) geprueft, " << claim.uebernommen
                      << " uebernommen -- ihre Fenster liegen voll in der Selektion dieses Laufs (" << indices.size()
                      << " Indizes) und werden ueber den per-Binary-Miss-Weg NACHGEBAUT\n"
                      << std::flush;
    }

    // Bauplan §8: die AlgoSigFn wird dem Orchestrator mitgegeben -> je Binary wird die Organ-Signatur (algo_sig)
    // berechnet, ins .algos-Sidecar geschrieben und in BuildResult.algo_sig getragen (fuer den Mess-Resume unten).
    // Leer = Organ-Gate aus (byte-neutral).
    BuildOrchestrator orch{bcfg, std::move(compile), std::move(gen), std::move(ram), std::move(algo_sig)};
    // I2: den Fingerprint-Provider (Lager-Anker je Binary) durchreichen. Leer = kein .fingerprint-Sidecar (byte-neutral).
    orch.set_fingerprint_provider(cfg.bestand_fingerprint_fn);

    // W11 (Ledger §43.c): der BAU-MODUS async Push-Pump. NUR im provision_only-Bau (der Mess-Modus bleibt STRIKT
    // synchron -- er baut hier NICHT mit cache_push, sondern pusht per-Binary im 1-Thread-Mess-Loop unten). Gated auf
    // cache_push + per_binary_subdirs; ohne COMDARE_STORAGE_CACHE ist cache_push leer => kein Pump => byte-neutral.
    // Der Completion-Hook feuert aus den Build-Workern (Completion-Reihenfolge) und reiht jede fertige ok()-perm.dll
    // ein; der EINE Push-Thread serialisiert die mc-Aufrufe und UEBERLAPPT sie mit dem weiterlaufenden Bau. Die index-
    // geordnete progress_sink-/Determinismus-Naht (W6/§38) bleibt UNBERUEHRT (unten, 1-Thread, reihenfolge-stabil).
    std::unique_ptr<AsyncPushPump> push_pump;
    if (cfg.provision_only && cfg.cache_push && cfg.per_binary_subdirs)
        push_pump = std::make_unique<AsyncPushPump>(cfg.cache_push, cfg.build_version, cfg.partial_marker_sink,
                                                    cfg.chunk_part_size);
    // #46b I1: EIN kombinierter Completion-Hook -- W11-Push-Enqueue (UNVERAENDERT) + opt-in Bestandslog-Registrierung.
    // Feuert aus den Build-Workern (Completion-Reihenfolge); lager.observe ist thread-sicher. Wird NUR gesetzt, wenn
    // Push ODER Bestandslog aktiv ist -- sonst kein Hook (byte-identisch zum Ist). Der Bestandslog-Zweig ist zusaetzlich
    // gegated (bestandslog_active) -> ist NUR Push aktiv, bleibt das Verhalten des Push-Pfades byte-genau erhalten.
    if (push_pump || bestandslog_active) {
        orch.set_on_binary_done([&push_pump, &lager, &cfg, bestandslog_active](BuildResult const& b) {
            // S1: b.skipped (dll_is_current-Hit) NICHT re-pushen -- eine fingerprint-aktuelle Binary kam entweder aus dem
            // Warm-Cache-Pull (liegt bereits im Store) oder aus einem frueheren Lauf; ein erneuter mc-Push ist reine
            // Redundanz (last-writer-wins, identische Bytes). Nur FRISCH gebaute ok()-Binaries wandern in die Queue.
            if (push_pump && b.ok() && !b.skipped && !b.output.parent_path().empty())
                push_pump->enqueue(b.output.parent_path());
            // #46b I1: die frisch gebaute Binary im Bestandslog registrieren/klassifizieren. Key = Host-Provider ueber
            // den Ausgabepfad (Option A; nullopt => keine Registrierung dieser Binary). Nur ok()+FRISCH (nicht skipped).
            if (bestandslog_active && b.ok() && !b.skipped) {
                auto const          key = cfg.bestand_key_of(b.output);
                std::error_code     ec;
                std::uint64_t const bytes = std::filesystem::exists(b.output, ec)
                                                ? static_cast<std::uint64_t>(std::filesystem::file_size(b.output, ec))
                                                : 0;
                // TP1-N2 (B-1) / TP1-F2: der Eintrags-Pfad kommt aus der EINEN Form-Quelle
                // (bestand_eintragspfad) -- store-relativ, generic, mit ehrlichem Fallback. Dieselbe
                // Funktion bildet die Praefixe der beiden Verwurf-Wege; nur so treffen sie diesen Eintrag.
                std::string pfad = bestand_eintragspfad(b.output, cfg.output_dir);
                lager.observe(key.value_or(std::string{}), cfg.bestand_zelle, std::move(pfad), bytes, b.algo_sig,
                              bestandslog::now_utc_iso());
            }
        });
    }

    // #46b I1b (opt-in): im BEREITSTELLUNGS-Lauf mit aktivem Bestandslog treibt der Planer den Bau slice-weise (je
    // Slice Reservierung + avg_size; seit Lager-TP1(B) mit per-Binary-BAU-FILTER, s.u.). Das Gate ist DOPPELT
    // (planer_driven_active, B5): im MESS-Lauf wuerden die Reservierungen je Fenster zwei Dokument-Roundtrips mit je
    // mehreren mc-Spawns in die 1-Thread-Messung legen -- verboten (Batch-Typen nie mischen, Mess-Exklusivitaet).
    // Inaktiv => der EINE provision_all => byte-identisch.
    //
    // G-E2 (Lager-TP1(B)): die PresenceFn wird HIER real belegt -- lager_contains (der beim Start geladene
    // Bestandslog-Index) gebunden ueber den Fingerprint-Provider (dieselbe I2-Quelle, aus der das .fingerprint-
    // Sidecar entsteht: binary_id -> 128-hex, VOR dem Bau berechenbar) und die Zell-Koordinaten dieses Laufs.
    // Host-Injektion (cfg.bestand_present) behaelt Vorrang (Test-Naht). Ohne Fingerprint-Provider bleibt die
    // Praedikat-Funktion leer -> konservativer Miss ueberall -> volles Fenster (byte-identisch, wie bisher).
    // Baum-agnostisch: die Bindung haengt am Dokument-Interface (LagerRunState/lager_contains) -- der kuenftige
    // Baum-Vollausbau tauscht die Traeger-Schicht darunter, nicht diese Naht.
    bestandslog::PresenceFn bestand_present = cfg.bestand_present;
    if (!bestand_present && bestandslog_active && cfg.bestand_fingerprint_fn) {
        bestandslog::BinaryIdKeyFn const by_id =
            [fp = cfg.bestand_fingerprint_fn](std::string const& binary_id) -> std::optional<std::string> {
            std::string hex = fp(binary_id);
            if (hex.empty()) return std::nullopt; // kein Fingerprint bekannt -> konservativer Miss
            return hex;
        };
        bestand_present =
            bestandslog::make_lager_presence(lager, bestandslog::make_index_key_fn(view, by_id), cfg.bestand_zelle);
    }
    std::vector<BuildResult> builds;
    // E-04-P1 (Marker-Familie v2, Teil 1a-ii): der Fenster-Token DIESER Invocation. Die Selektion ist
    // bereits das COMDARE_GOLDEN_N_RANGE-Fenster, front() also der globale Fenster-Anfang -- dieselbe
    // Koordinate, die der planer-getriebene Pfad je Slice fuehrt.
    std::string const lauf_fenster = marker_fenster(indices.empty() ? std::size_t{0} : indices.front(), indices.size());
    bool const        planer_getrieben = bestandslog::planer_driven_active(bestandslog_active, cfg.provision_only);
    // T2-A/F4-NB2 (Befund 1): die Vollzugs-Pruefung des Push-Kanals. Sie existiert NUR, wenn es einen Pump
    // gibt; ohne ihn bleibt sie leer und der Zaehler-Pfad ist zeilen- wie verhaltensidentisch zum Ist. Die
    // Lambda haelt den Pump per Referenz -- sie lebt ausschliesslich innerhalb des Aufrufs darunter, also
    // kuerzer als der unique_ptr.
    PushVollzugFn push_vollzug;
    if (push_pump)
        push_vollzug = [&push_pump]() -> std::size_t {
            push_pump->drain(); // Barriere: alles bis hierher Eingereihte ist abgearbeitet
            return push_pump->failed_count();
        };
    if (planer_getrieben)
        builds = run_planer_driven_provision(orch, view, indices, cfg, result.build_stats, bestand_present,
                                             &result.bestand_lager_skips, &result.plan_resume_skips, push_vollzug);
    else {
        // FALLBACK-KANAL: ohne aktives Bestandslog gibt es keinen Slice-Loop -- die Invocation IST das
        // eine Fenster. Ohne diese beiden Zeilen waere der Live-Kanal in genau den Laeufen stumm, in
        // denen der Maschinen-Kanal (Bestandslog-Done-Records) ebenfalls schweigt (storage-INERT).
        // Die Plan-Zeile steht VOR dem Bau: lager=0/zu_bauen==gesamt, weil ohne Praedikat nicht
        // gefiltert wird (keine erfundene Lager-Zahl).
        //
        // GELTUNGSBEREICH (bewusst, nicht versehentlich): planer_driven_active haengt an provision_only,
        // also faehrt AUCH der pruef_only-Lauf und der Mess-Lauf durch diesen Zweig -- beide fuehren
        // real einen provision_all (im Resume-Modus) aus, und phase=bau benennt genau den. Der Nutzen
        // ist diagnostisch: im pruef_only-Lauf MUSS gebaut_neu=0 stehen. Steht dort eine Zahl > 0, hat
        // der Pruef-Prozess Binaries NEU gebaut, die der Lager-Bau-Filter zuvor uebersprungen hatte --
        // die stille Negierung des Lager-Skips (G-A2-Wechselwirkung) waere damit im Trace SICHTBAR,
        // statt erst am ETA-Ende aufzufallen.
        std::cerr << marker_kopf(kMarkePlanTestat, cfg.marker_kontext, bestandslog::now_utc_iso(), "bau", lauf_fenster)
                  << " gesamt=" << indices.size() << " lager=0"
                  << " zu_bauen=" << indices.size() << "\n"
                  << std::flush;
        builds = orch.provision_all(view, std::span<const std::size_t>{indices}, &result.build_stats);
    }

    result.built              = result.build_stats.succeeded;
    result.built_new          = result.build_stats.built;
    result.built_skip         = result.build_stats.skipped;
    result.min_free_ram_bytes = result.build_stats.min_free_ram_bytes;
    // E-04-P1: die Fallback-Bilanz macht built_new/built_skip zu KONSUMENTEN (Aufraeumpass-Kandidat (1)
    // der 75er-Liste entschaerft: built_new hatte bereits den SOTA-Bruecken-Leser, built_skip war
    // leserlos). lager_skip UND plan_skip bleiben 0, weil dieser Pfad weder einen Bau-Filter noch einen
    // Plan fuehrt -- 0 ist hier die WAHRHEIT, keine Ersatzzahl. dauer_s fuehrt nur der Slice-Pfad (dort
    // wird die Wall-Clock je Fenster fuer die ETA ohnehin gemessen); hier gaebe es keine ehrliche Zeit.
    if (!planer_getrieben)
        std::cerr << marker_kopf(kMarkeBilanzTestat, cfg.marker_kontext, bestandslog::now_utc_iso(), "bau",
                                 lauf_fenster)
                  << " gebaut_neu=" << result.built_new << " sidecar_skip=" << result.built_skip
                  << " lager_skip=" << result.bestand_lager_skips << " plan_skip=" << result.plan_resume_skips
                  << " fehl=" << result.build_stats.failed << "\n"
                  << std::flush;

    // #46b I1 (opt-in): die frisch gebauten Binaries EINMAL ins Binary-Bestandslog mergen (single-threaded,
    // deterministisch, store_document_merged = Union statt blinder Ersetzung, GELOCKT ueber den einen Zyklus). No-Op
    // => byte-neutral, wenn inaktiv. Bleibt der Schreibvorgang aus (nullopt), bleiben die Eintraege vorgemerkt
    // (Re-Queue) und die Zeile sagt es -- die Details stehen in der Testat-Zeile des Zyklus darueber.
    // Die cerr-Diagnose ist golden-/CSV-neutral (kein Mess-Datum, kein binary_id-Byte -- Muster wie die pruef-Zeilen).
    //
    // TP1-N2 (B-1): NUR NOCH ALS HELFER DEFINIERT -- gerufen wird er in JEDEM Ausgangs-Zweig NACH dem
    // jeweiligen Push-Abschluss (provision_only: nach push_pump->close() + Fehl-Push-Ausschluss;
    // pruef_only/Mess-Pfad: an deren Ende -- dort pushen die Pfade synchron bzw. gar nicht). Ein flush
    // VOR dem Push-Drain registrierte Eintraege, deren Store-Objekt noch gar nicht existiert -- unter
    // dem Bau-Filter der stille Verlustpfad (der Folgelauf skippte eine nirgends existierende Binary).
    // EHRLICHE GRENZE: ohne Push-Kanal (cache_push leer) beschreibt ein Eintrag nur den LOKALEN
    // Bestand dieser Maschine -- der Betriebs-Kontrakt koppelt COMDARE_BESTANDSLOG an den Storage-Push.
    auto const bestandslog_flush = [&]() {
        if (!bestandslog_active) return;
        auto const reg = lager.flush(cfg.bestand_transport, cfg.bestand_doc_key, bestandslog::now_utc_iso(),
                                     bestandslog::make_lock_owner(cfg.bestand_owner_uuid, cfg.bestand_maschine));
        std::cerr << "[bestandslog] lager=" << lager.lager_size() << " hits=" << lager.lager_hits()
                  << " neu=" << (reg ? *reg : std::size_t{0}) << (reg ? "" : " (nicht persistiert)") << "\n"
                  << std::flush;
    };

    // G-E3: derselbe Abschluss fuer das MESSWERT-Genus -- eigener Helfer, eigenes Dokument, eigene
    // Testat-Zeile. Er laeuft NUR am Ende des Mess-Pfades (die Bau-Ausgaenge messen nichts, dort
    // waere die Zeile ein Rauschen mit neu=0).
    auto const messwert_flush = [&]() {
        if (!mess_bestandslog_active) return;
        auto const reg = mess_lager.flush(cfg.bestand_transport, cfg.mess_bestand_doc_key, bestandslog::now_utc_iso(),
                                          bestandslog::make_lock_owner(cfg.bestand_owner_uuid, cfg.bestand_maschine));
        std::cerr << "[bestandslog] messwert-lager=" << mess_lager.lager_size() << " hits=" << mess_lager.lager_hits()
                  << " neu=" << (reg ? *reg : std::size_t{0}) << (reg ? "" : " (nicht persistiert)") << "\n"
                  << std::flush;
    };

    // ── Welle 5 (E-W5-2, §38-Fortschritts-Rueck-Kanal): Zustand + Feuerungs-Helfer. Der Kanal reist SYNCHRON aus
    //    dem 1-Thread-Loop (nie aus den Build-Workern) und in StaticBinaryView-Ordnung: builds[j] entspricht
    //    view[builds[j].index] (provision_core fuellt results[] positions-treu zur UEBERGEBENEN Index-Liste).
    //    KORREKTUR TP1-N3 (B-2c): "builds[j] == view[indices[j]]" gilt seit dem Bau-Filter NICHT mehr -- im
    //    planer-getriebenen Bau ist builds die TEILMENGE der wirklich gebauten Indizes (Lager-Treffer fehlen
    //    darin); massgeblich ist ausschliesslich builds[j].index. progress_prev
    //    haelt die zuletzt gemeldete mixed-radix-Ziffernfolge (leer => erste Meldung = Voll-Konfiguration). Alles
    //    No-Op, wenn kein progress_sink gesetzt ist => byte-neutral. ──
    std::vector<std::size_t> progress_prev;
    auto                     fire_progress = [&](std::size_t view_index, std::size_t cursor) {
        if (!cfg.progress_sink) return; // No-Op-Naht (byte-neutral)
        std::vector<std::size_t> const cur = view.variant_tuple(view_index);
        cfg.progress_sink(progress_delta_between(progress_prev, cur, cursor));
        progress_prev = cur;
    };
    auto fire_progress_done = [&](std::size_t total) {
        if (!cfg.progress_sink) return; // No-Op-Naht (byte-neutral)
        ProgressDelta term;             // §38.b-Fertig-Signal: done genau EINMAL am Fensterende (changed leer)
        term.cursor = total;
        term.done   = true;
        cfg.progress_sink(term);
    };
    // TP1FK1-B5 (Codex-Befund): der CURSOR ist laut Vertrag der FENSTER-RELATIVE Perm-Index
    // (progress_delta.hpp) -- also die Position in der Selektion `indices` DIESES Laufs. Gefeuert wurde
    // aber die Laufvariable j ueber `builds`. Seit dem Bau-Filter (TP1-N3/B-2c) ist builds die TEILMENGE
    // der wirklich gebauten Indizes: nach einem Lager-Skip rutschen alle folgenden Cursor um die Zahl der
    // Skips nach vorn (Cursor-KOMPRESSION), waehrend das done-Delta die VOLLE Menge meldet -- der
    // Rueck-Kanal-Konsument bekam Positionen, die es im Fenster so nie gab.
    //
    // Die Abbildung ist deshalb view_index -> Position in `indices`. In den beiden NICHT planer-getriebenen
    // Pfaden (pruef_only + Mess-Merge) ist builds positions-treu zur uebergebenen Liste, dort liefert sie
    // beweisbar wieder j -> jene Pfade bleiben byte-identisch. EINE Regel statt zweier, damit ein kuenftiger
    // Filter an anderer Stelle diesen Defekt nicht erneut erzeugt. Doppelte Indizes in `indices` waeren eine
    // kaputte Selektion; die erste Position gewinnt, und der Fallback (die Laufvariable) greift nur, wenn ein
    // gebauter Index gar nicht in der Selektion stand -- ein Widerspruch, der dann wenigstens nicht abstuerzt.
    std::map<std::size_t, std::size_t> fenster_pos;
    for (std::size_t k = 0; k < indices.size(); ++k) fenster_pos.emplace(indices[k], k);
    auto fenster_cursor_of = [&fenster_pos](std::size_t view_index, std::size_t rueckfall) {
        auto const it = fenster_pos.find(view_index);
        return (it == fenster_pos.end()) ? rueckfall : it->second;
    };

    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    // INC-G6 (Ledger 33/34, 2026-07-19, BAUPLAN Abschnitt 2): PROVISION-ONLY. Nach der STATISCHEN Kompilierung
    // (DLLs + .version/.algos-Sidecars stehen versions-aktuell) VOR der Lade-/Mess-Phase zurueckkehren: KEIN
    // DLL-Laden, NICHTS gemessen, KEINE CSV-Mess-Zeilen. result.built traegt die Zahl bereitgestellter Binaries.
    // Entkoppelt den ~8h-Materialisierungs-Bau vom mehrtaegigen Messlauf (Storage-cachebar). Byte-identisch fuer
    // provision_only==false (Default) -- der Zweig wird dann nie betreten.
    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    if (cfg.provision_only) {
        // W11 (Ledger §43.c): den async Push-Pump DRAINEN -- alle waehrend des Baus ueberlappend eingereihten Pushes
        // fertig abarbeiten + den Push-Thread joinen -- BEVOR wir zurueckkehren. Der (CI-seitige) Whole-Chunk-Marker
        // erscheint erst NACH diesem vollen Drain (Vollstaendigkeits-Garantie bleibt: Marker => alle DLLs im Store).
        // Ist kein Pump aktiv (cache_push leer / kein per_binary_subdirs), war auch nichts zu pushen => byte-neutral.
        // Der frueherer Batch-Push-Loop (perm.dll+.version je ok()-Binary NACH provision_all) ist damit ersetzt: die
        // Push-MENGE ist identisch (jede ok()-Binary genau einmal), nur zeitlich mit dem Bau ueberlappt.
        if (push_pump) {
            push_pump->close();
            // TP1-N2 (B-1): Push-Fehler von der Registrierung AUSSCHLIESSEN. failed_dirs() sind die
            // Verzeichnisse, deren Push warf (die einzige Fehler-Sicht der void-CachePushFn); ihre
            // vorgemerkten Eintraege fliegen aus fresh_, BEVOR geflusht wird -- sonst stuende im
            // geteilten Dokument ein Bestand, den der Store nie erhielt, und der Bau-Filter des
            // Folgelaufs skippte eine nirgends existierende Binary. Die lokale Kopie bleibt;
            // dll_is_current deckt sie auf DIESER Maschine weiter.
            if (bestandslog_active) {
                auto const fehl = push_pump->failed_dirs();
                if (!fehl.empty()) {
                    std::vector<std::string> praefixe;
                    praefixe.reserve(fehl.size());
                    // DIESELBE Form-Quelle wie die Vormerkung (bestand_eintragspfad) -- ein abweichend
                    // gebildeter Praefix liesse den unbestaetigten Eintrag stehen.
                    for (auto const& d : fehl) praefixe.push_back(bestand_eintragspfad(d, cfg.output_dir));
                    auto const verworfen = lager.discard_fresh_with_pfad_prefix(praefixe);
                    std::cerr << "[bestandslog] warn: " << verworfen << " Eintraege wegen " << fehl.size()
                              << " Push-Fehler(n) NICHT registriert -- lokale Kopie bleibt, der Store ist "
                                 "unbestaetigt (kein Skip-Anspruch im Folgelauf)\n"
                              << std::flush;
                }
            }
        }
        // TP1-N2 (B-1): der flush laeuft NACH dem Push-Drain -- registriert wird nur, was den Store
        // real erreichen konnte.
        bestandslog_flush();
        // Welle 5 (E-W5-2): der Paragraf-38-Rueck-Kanal feuert AUCH im provision_only-Lauf -- je GEBAUTE Binary EIN
        // Delta (in StaticBinaryView-Ordnung; Lager-Treffer haben keine Konfigurations-Aenderung zu melden).
        // TP1-N3 (B-2b): das done-Delta traegt die VOLLE bereitgestellte Menge (gebaut + Lager-Bestand) --
        // builds.size() allein unterzaehlte seit dem Bau-Filter die Bereitstellung dieses Fensters.
        // T2-A/F4-BILANZ: derselbe Satz gilt fuer die dritte Quelle -- die plan-resumierten Atome sind
        // bereitgestellt, also gehoeren sie in dieselbe Summe. Sonst meldete ein plan-resumierter Lauf
        // dem Paragraf-38-Kanal weniger fertig, als er im selben Atemzug als `built` fuehrt.
        for (std::size_t j = 0; j < builds.size(); ++j)
            fire_progress(builds[j].index, fenster_cursor_of(builds[j].index, j)); // TP1FK1-B5: Fenster-Index
        fire_progress_done(builds.size() + result.bestand_lager_skips + result.plan_resume_skips);
        return result;
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    // S3 (§62-B COMDARE_PRUEF_ONLY): NUR das Konformitaets-Gate je bereits gebauter .so -- KEINE Messung. provision_all
    // lief im Resume-Modus (versions-aktuelle .so uebersprungen -> KEIN Neubau im provision->pruef-Fluss); dieser Zweig
    // laedt+gated jede ok()-Binary (run_so_conformance_gate, Herausloesung aus measure_one_binary) und kehrt VOR der
    // Mess-Phase zurueck. Log-sichtbar je Zelle via S1-ProgressHeartbeat (zeit-gated) + je Fail eine geflushte Zeile.
    // pruef_ok/pruef_failed -> der Entry (run_profile) setzt daraus den Exit-Code (!=0 bei Gate-Fail).
    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    if (cfg.pruef_only) {
        if (push_pump) push_pump->close(); // toter Schutz: der Pump entsteht nur im provision_only-Zweig
        bestandslog_flush();               // TP1-N2 (B-1): auch dieser Ausgang registriert erst am Ende
        ProgressHeartbeat pruef_hb{"pruef-zelle", builds.size()};
        for (BuildResult const& b : builds) {
            if (!b.ok()) {
                ++result.load_failed;
                ++result.pruef_failed;
                std::cerr << "[pruef-fail] binary_id='" << b.binary_id << "' Bau-Fehler (status=" << b.status
                          << ") -> nicht pruefbar\n"
                          << std::flush;
                pruef_hb.tick();
                continue;
            }
            pruef_dock::PruefOutcome const oc = pruef_dock::run_so_conformance_gate(b.output, cfg.erwartete_mess_zeile);
            if (!oc.loaded) {
                ++result.load_failed;
                ++result.pruef_failed;
                std::cerr << "[pruef-fail] binary_id='" << b.binary_id
                          << "' .so nicht ladbar/kein Mess-Interface: " << b.output.string() << "\n"
                          << std::flush;
            } else if (!oc.mess.passed()) {
                // M-1/D-2: die Binary laedt und ist funktional pruefbar -- aber sie ist NICHT DIE, die diese
                // CEB gebaut hat (oder sie deklariert gar nichts). Eigener Zweig VOR dem Funktions-Gate,
                // damit die Meldung den Identitaets-Bruch benennt statt ihn als Gate-Fail zu tarnen.
                ++result.loaded;
                ++result.pruef_failed;
                std::cerr << "[pruef-fail] binary_id='" << b.binary_id << "' "
                          << pruef_dock::mess_konsistenz_meldung(oc.mess) << " -> " << b.output.string() << "\n"
                          << std::flush;
            } else if (oc.gate.passed()) {
                ++result.loaded;
                ++result.pruef_ok;
            } else {
                ++result.loaded;
                ++result.pruef_failed;
                std::cerr << "[pruef-fail] binary_id='" << b.binary_id << "' Gate " << oc.gate.cases_passed << "/"
                          << oc.gate.cases_total << " first_fail=" << oc.gate.first_fail << "\n"
                          << std::flush;
            }
            pruef_hb.tick();
        }
        pruef_hb.done();
        // E-04-P1 (Marker-Familie v2): die Gate-Bilanz DIESER Invocation -- dieselbe Grammatik, dieselben
        // Pflichtfelder, derselbe Aggregator-Key (zelle, fenster) wie im Bau-Kanal. Heute faehrt der
        // Pruef-Aufruf genau EIN Fenster je Invocation (die Emission setzt COMDARE_GOLDEN_N_RANGE); E-04-P2
        // verschiebt spaeter nur die GRANULARITAET dieses Fensters, nicht die Zeilen-Form.
        std::cerr << marker_kopf(kMarkePruefBilanz, cfg.marker_kontext, bestandslog::now_utc_iso(), "pruef",
                                 lauf_fenster)
                  << " ok=" << result.pruef_ok << " fehl=" << result.pruef_failed
                  << " faelle=" << (result.pruef_ok + result.pruef_failed) << "/" << builds.size() << "\n"
                  << std::flush;
        for (std::size_t j = 0; j < builds.size(); ++j)
            fire_progress(builds[j].index, fenster_cursor_of(builds[j].index, j)); // TP1FK1-B5: Fenster-Index
        fire_progress_done(builds.size());
        return result;
    }

    RuntimeVariableLoop const loop{cfg.env_limits};

    // #156-De-Risk (2026-06-20): die EINE PMC-Quelle für den GANZEN Treiber-Lauf (Strategy+Factory; build-/OS-abhängig:
    // Windows-Intel-PCM unter COMDARE_ENABLE_PMC, sonst NullPmcSource → available=false). EINMAL erzeugt — NICHT je Op,
    // NICHT je Binary — und per Referenz in den WIDE-Mess-Pfad (run_observable_perm/run_workload_perm) gereicht; dort
    // klammert begin()/end() nur den getimten Batch. Schließt die #156-Naht: die WIDE-CSV trägt jetzt reale PMC-Counter
    // (lokal 0/available=0 mit NullPmcSource; Montag Linux+PMC=ON real). Identisches Muster wie f15_compare/main.cpp:252.
    // #45 (§16.2-M1/§61-MODI): der PMC-Source wird PRO Mess-Worker einmal erzeugt (make_pmc_source in collect_ordered's
    // Ctx-Factory) -- NICHT je Op/Binary und NICHT geteilt (ein realer LinuxPerfPmcSource ist nicht thread-safe teilbar).
    // Sequentiell (measure/release/default) => genau EIN Source fuer den ganzen Lauf (byte-identisch zum Ist).

    // Mess-RESUME (#139): EIN Config-Stempel je Lauf (BuildVersion + Skala + volle dyn-Dimensions-Signatur).
    std::string const resume_stamp_prefix = lazy_resume_stamp_prefix(cfg, dyn_dims);

    // #45: das Ergebnis EINER Mess-Zelle -- worker-LOKAL gesammelt (kein geteilter Baum/kein Lock), danach in
    // KANONISCHER builds-Reihenfolge in `result` gemerged (deterministische CSV). Default-konstruierbar (Pool-Slot).
    struct CellOutcome {
        std::size_t                  measured               = 0;
        std::size_t                  loaded                 = 0;
        std::size_t                  load_failed            = 0;
        std::size_t                  dynamic_settings_total = 0;
        std::size_t                  resumed_binaries       = 0;
        std::string                  resumed_csv_rows;
        std::vector<LazyMeasuredRow> rows;
        bool                         progress_eligible =
            false; // fire_progress NUR fuer geladene/gemessene Zellen (wie das Ist; Skips feuern nie)
        // T2-A/F4-NB2 (Codex-Voll-Scope, Befund 2) -- "DIESE ZELLE IST ZERTIFIZIERT GEMESSEN".
        //
        // Das Praedikat der MESS-FRONT, und zwar EXAKT das des per-Binary-Resume-Stempels: gesetzt wird es
        // an genau den zwei Stellen, an denen diese Ablage einen Resume-ANSPRUCH traegt -- beim geglueckten
        // Schreiben von result.csv.stamp und beim geglueckten Resume aus einem vorgefundenen Stamp. Es ist
        // NICHT `measured > 0`: gemessene Zeilen entstehen auch dort, wo der Stamp bewusst ausbleibt
        // (unvollstaendige Settings, eine ungueltige Zwei-Phasen-Zeile, |fpr=-Formverstoss, gescheiterter
        // CSV-Write) -- eine solche Zelle HAT Zeilen, aber sie hat keinen Stand, auf den ein Folgelauf
        // aufsetzen darf. Genau diese Verwechslung war der Befund.
        //
        // EHRLICHE GRENZE, die aus der Kopplung an den Stamp folgt: OHNE per_binary_subdirs gibt es keine
        // per-Binary-Ablage und damit kein Zertifikat -- das Praedikat bleibt dann fuer JEDE Zelle false
        // und die Mess-Front dieses Laufs ist 0. Das ist die fail-closed-Richtung und keine Luecke: ohne
        // per-Binary-Ablage kann ein Folgelauf ohnehin nichts resumieren, eine Front waere dort eine Zahl
        // ohne Deckung. Der produktive Pfad setzt per_binary_subdirs unbedingt (profile_run_entry.hpp).
        bool mess_front_faehig = false;
    };

    // A15/FK-1 (Auflage K3): die Identitaets-/Lauf-Tags einer MARKER-Zeile stammen ausschliesslich aus der
    // Lauf-KONFIGURATION -- es gab keine Messung, aus der etwas anderes stammen koennte. Es sind exakt
    // dieselben Felder, die measure_under_setting einer gemessenen Zeile gibt (Single-Source-Naehe: wer dort
    // ein Tag ergaenzt, sieht hier die Luecke). Alles Uebrige bleibt Default und wird vom Renderer ueber
    // zell_ersatz ersetzt -- eine Marker-Zeile traegt daher NIE eine Null.
    auto make_marker_row = [&](std::string const& binary_id) {
        LazyMeasuredRow row;
        row.binary_id      = binary_id;
        row.series         = cfg.row_series;
        row.pruefling_type = cfg.row_pruefling_type;
        row.sweep_axis     = cfg.row_sweep_axis;
        row.working_set_n  = cfg.workload_records;
        row.platform       = cfg.row_platform;
        row.build_version  = cfg.row_build_version;
        row.fairness_mode  = cfg.row_fairness_mode;
        row.h2_score       = cfg.row_h2_score;
        return row;
    };

    // #45: die per-Zelle Mess-Funktion -- OHNE geteilten Zustand. Der Baum-Round-Trip (ingest_result_line+node_value)
    // ist durch parse_result_line_to_node_value (worker-lokal, rein) ersetzt -> byte-identischer NodeValue (setting_id
    // == die id in pr.line). `result`/`tree`/`fire_progress` werden NICHT beruehrt; alles landet in `oc`. Fehler ->
    // geloggt, kein throw (die Zelle liefert ein Outcome, der Loop misst weiter). Per-bin_dir-Schreib-/Push-Naehte sind
    // je Binary isoliert (eigener Unterordner). Wird sequentiell ODER aus dem Worker-Pool aufgerufen.
    auto measure_one_binary = [&](BuildResult const& b, measurement::IPmcSource* cell_pmc) -> CellOutcome {
        CellOutcome oc;
        // TP1FK1-B10 (Codex-Befund, BLOCK): der BAU-FEHLER-ZWEIG STEHT VOR DEM RESUME-KURZSCHLUSS.
        //
        // Frueher lief der Resume-Check zuerst -- ausdruecklich "auch bei Build-Fehler, sonst stille
        // CSV-Luecke". Seit A15/FK-1 gibt es die Luecke nicht mehr: der Fehlerzweig schreibt eine ehrliche
        // nicht_gebaut-Marker-Zeile. Damit kehrte sich die Reihenfolge ins Gegenteil um: eine Binary, die
        // sich HEUTE NICHT MEHR BAUEN LAESST, uebernahm die ERFOLGS-CSV eines frueheren Laufs und der
        // Marker kam nie zustande. Der naechste Lauf faende dieselbe CSV samt gueltigem Stamp erneut --
        // ein Bau-Fehler konnte so beliebig lange hinter alten Messwerten verschwinden. (Ein reiner
        // Wiedereinstieg ist davon nicht betroffen: eine versions-aktuelle Binary wird vom Orchestrator
        // als b.ok()+skipped gemeldet und laeuft weiter durch den Resume-Zweig unten.)
        if (!b.ok()) {
            // d1-log (D1): den Bau-Fehler KLASSIFIZIERT deklarieren (Infra ODER Compiler-Compiler), Harness MISST WEITER.
            namespace cm               = ::comdare::cache_engine::measurement;
            cm::BuildError const   err = b.outcome.has_value()
                                             ? cm::BuildError{cm::CompilerCompilerErrorClass::CompileKombination}
                                             : b.outcome.error();
            std::string_view const prefix =
                cm::error_domain(err) == cm::ErrorDomain::Infra ? "Infra-Fehler" : "Compiler-Compiler-Fehler";
            std::cerr << "[" << prefix << ": " << cm::build_error_label(err) << "] binary_id='" << b.binary_id
                      << "' status=" << b.status << " log=" << b.output.string() << ".cxx.log\n";
            // A15/FK-1 (Owner-Q4 per Volles-GO 02.08., Auflage K3): der Bau-Fehler verschwindet nicht mehr
            // NUR ins Log. Genau EINE Marker-Zeile je nicht gebauter Binary (nicht je Setting -- ohne
            // geladene DLL ist die dynamische Belegung schlicht unbekannt, jede Setting-Aufspaltung waere
            // erfunden). Spaltenzahl-erhaltend ueber den EINEN Renderer; oc.measured bleibt 0 (es wurde
            // nichts gemessen) und progress_eligible bleibt false (die Zelle erreicht die Mess-Naht nie).
            LazyMeasuredRow marker = make_marker_row(b.binary_id);
            marker.build_status    = cm::BuildCellStatus::NichtGebaut;
            // TP1FK1-B10: die per-Binary-Ablage wird INVALIDIERT, BEVOR der Resume-Check des NAECHSTEN
            // Laufs sie faende. Der Stamp faellt (kein Resume-Anspruch mehr) und die result.csv traegt
            // genau die Marker-Zeile, die auch in die globale CSV geht -- statt der Erfolgs-Zeilen eines
            // Laufs, dessen Binary es nicht mehr gibt. Sie weiter als gueltigen Messstand zu fuehren
            // waere die Luege. Der Fehlschlag beim Schreiben ist selbst ein Befund und wird beziffert,
            // nie verschluckt.
            //
            // F-B10 Owner-Default (b) (02.08.), Doktrin "Messdaten nie loeschen": der alte Stand wird
            // NICHT ueberschrieben, sondern ZUERST nach result.csv.stale UMBENANNT und bleibt damit als
            // Rohdatum erhalten (ent-wertet, nicht entfernt -- der Resume-Check liest ausschliesslich
            // result.csv + result.csv.stamp, ein .stale hat also keinen Resume-Anspruch). Erst danach
            // schreibt dieser Zweig die nicht_gebaut-Marker-Zeile als NEUE result.csv.
            // RANDFALL (deklariert): ein bereits vorhandenes result.csv.stale wird dabei ersetzt --
            // fs::rename ueberschreibt. Das ist gewollt: beide Staende beschreiben DIESELBE, in diesem
            // wie im vorherigen Lauf nicht baubare Binary; die Ablage traegt genau einen ent-werteten
            // Vorgaenger-Stand statt einer unbegrenzt wachsenden Kette.
            if (cfg.per_binary_subdirs) {
                std::filesystem::path const fehl_dir = b.output.parent_path();
                if (!fehl_dir.empty()) {
                    std::error_code fec;
                    std::filesystem::create_directories(fehl_dir, fec);
                    // Stamp ZUERST weg (write-ZULETZT-Disziplin): ein Abbruch mitten drin darf nie eine
                    // Marke zuruecklassen, die auf eine halb geschriebene CSV zeigt.
                    //
                    // TP1FK1-B10 (Codex-Befund CX-W4): das Entfernen wird GEPRUEFT. Frueher lief remove()
                    // mit dem wiederverwendeten fec, und weder der error_code noch der bool-Rueckgabewert
                    // wurden angesehen -- schlug es fehl (Rechte, gesperrte Datei, ro-Mount, Verzeichnis
                    // statt Datei), blieb der ALTE Erfolgs-Stamp liegen, waehrend die Diagnose woertlich
                    // "der Stamp ist entfernt" behauptete. Genau daraus entsteht die Wiedereinstiegs-Luege:
                    // ein Alt-Stamp, der die frisch geschriebene nicht_gebaut-Marker-Zeile als gueltigen
                    // Messstand zertifiziert. Geurteilt wird ueber den EIGENEN error_code UND das Ist --
                    // remove() setzt bei blosser Nicht-Existenz keinen Fehler (dann ist nichts zu entfernen,
                    // und exists() sagt genau das), und scheitert exists() selbst, entscheidet der
                    // remove-Fehler. Eine unbelegte Entfernungs-Behauptung gibt es damit nicht mehr.
                    std::error_code stamp_ec;
                    std::filesystem::remove(fehl_dir / "result.csv.stamp", stamp_ec);
                    std::error_code stamp_ist_ec;
                    bool const      stamp_bleibt = static_cast<bool>(stamp_ec) ||
                                                   std::filesystem::exists(fehl_dir / "result.csv.stamp", stamp_ist_ec);
                    if (stamp_bleibt) {
                        // Die INVALIDIERUNG ist gescheitert. Dann wird die Ablage auch NICHT ersetzt: mit
                        // liegendem Alt-Stamp waere jede frisch geschriebene result.csv (Marker-Zeile oder
                        // halber Stand) fuer den Resume-Check des Folgelaufs ein zertifizierter Messstand.
                        // Der Alt-Stand bleibt unangetastet zu seinem Stamp (in sich stimmig, wenn auch
                        // ueberholt), der Befund reist als Marker-Zeile in die globale CSV, und die
                        // Bereinigung ist eine benannte Hand-Arbeit statt einer stillen Annahme.
                        std::cerr << "[" << prefix << ": " << cm::build_error_label(err) << "] binary_id='"
                                  << b.binary_id << "' result.csv.stamp NICHT entfernt ("
                                  << (stamp_ec ? stamp_ec.message() : std::string{"liegt nach dem Entfernen noch"})
                                  << ") in " << fehl_dir.string()
                                  << " -- die per-Binary-Ablage bleibt daher UNVERAENDERT (ein Alt-Stamp darf "
                                     "keine nicht_gebaut-Marker-Zeile als Messstand zertifizieren); der Ordner "
                                     "MUSS von Hand geraeumt werden\n"
                                  << std::flush;
                    } else {
                        // DANN der Alt-Stand zur Seite (nie loeschen): fehlt die Datei, gibt es nichts zu
                        // sichern -- der Marker wird dann direkt geschrieben.
                        //
                        // TP1FK1-B10 (Review-Befund Z-01/GA-02): das Sichern wird GEPRUEFT, nach demselben
                        // Muster wie die stamp_bleibt-Wache darueber. Frueher lief das rename mit einem
                        // error_code, den danach niemand mehr ansah, und unmittelbar darauf oeffnete der
                        // Marker-Write DIESELBE Datei mit std::ios::trunc: scheiterte das rename (ein
                        // result.csv.stale als Verzeichnis, Rechte, ro-Mount, gesperrte Datei), LOESCHTE
                        // der trunc genau die Alt-Mess-CSV, die er gerade nicht sichern konnte -- der
                        // Bruch der Doktrin "Messdaten nie loeschen", ausgeloest vom Sicherungs-Versuch
                        // selbst. Geurteilt wird deshalb ueber den EIGENEN error_code UND das Ist: ein
                        // gelungenes rename laesst die Quelle NICHT liegen, also ist eine danach noch
                        // vorhandene result.csv der Beleg des Fehlschlags (und deckt auch den Fall ab, in
                        // dem eine Implementierung stillschweigend nichts tut). Im Zweifel wird fail-closed
                        // NICHTS ersetzt: kein trunc, kein Marker-Write.
                        std::filesystem::path const alt_csv = fehl_dir / "result.csv";
                        std::error_code             alt_ist_ec;
                        bool const                  alt_stand_da = std::filesystem::exists(alt_csv, alt_ist_ec);
                        std::error_code             rec;
                        if (alt_stand_da) std::filesystem::rename(alt_csv, fehl_dir / "result.csv.stale", rec);
                        std::error_code alt_nach_ec;
                        bool const      alt_stand_bleibt =
                            alt_stand_da && (static_cast<bool>(rec) || std::filesystem::exists(alt_csv, alt_nach_ec));
                        if (alt_stand_bleibt) {
                            // Die SICHERUNG ist gescheitert. Dann wird die Ablage auch NICHT ersetzt: der
                            // Alt-Mess-Stand bleibt unangetastet an seinem Platz (Rohdatum, nicht ersetzbar),
                            // der Resume-Anspruch ist mit dem gefallenen Stamp bereits weg (ein Folgelauf
                            // misst also neu, statt den Alt-Stand zu uebernehmen), der Befund reist als
                            // Marker-Zeile in die globale CSV, und die Bereinigung ist eine benannte
                            // Hand-Arbeit statt eines stillen Datenverlusts.
                            std::cerr << "[" << prefix << ": " << cm::build_error_label(err) << "] binary_id='"
                                      << b.binary_id << "' result.csv NICHT nach result.csv.stale gesichert ("
                                      << (rec ? rec.message() : std::string{"liegt nach dem Umbenennen noch"})
                                      << ") in " << fehl_dir.string()
                                      << " -- die per-Binary-Ablage bleibt daher UNVERAENDERT (ein nicht "
                                         "gesicherter Alt-Mess-Stand wird nicht ueberschrieben: Messdaten nie "
                                         "loeschen); der Ordner MUSS von Hand geraeumt werden\n"
                                      << std::flush;
                        } else {
                            bool marker_geschrieben = false;
                            {
                                std::ofstream mf{alt_csv, std::ios::trunc};
                                if (mf) {
                                    mf << lazy_csv_header() << format_csv_row(marker);
                                    mf.flush();
                                    marker_geschrieben = mf.good();
                                }
                            }
                            // Auch die Diagnose sagt nur, was BELEGT ist: eine result.csv.stale gibt es hier
                            // genau dann, wenn ueberhaupt ein Alt-Stand zu sichern war.
                            if (!marker_geschrieben)
                                std::cerr << "[" << prefix << ": " << cm::build_error_label(err) << "] binary_id='"
                                          << b.binary_id << "' nicht_gebaut-Marker NICHT in " << alt_csv.string()
                                          << " geschrieben -- der Stamp ist entfernt"
                                          << (alt_stand_da ? " und der Alt-Stand liegt als result.csv.stale"
                                                           : " (ein Alt-Stand war nicht vorhanden)")
                                          << ", ein Folgelauf misst also neu\n"
                                          << std::flush;
                        }
                    }
                }
            }
            oc.rows.push_back(std::move(marker));
            return oc; // Build-Fehler -> KEINE Messung, aber ein sichtbarer Datensatz (Zeile + Ablage)
        }

        // Bauplan Section 8: der PER-BINARY Resume-Stamp = Config-Prefix + additive Organ-Signatur
        // (leer => == Prefix, Ist).
        //
        // T2-A/K2 (Codex-Befund, SCHWER) -- DER STAMP TRAEGT AB HIER DEN VOLLEN FINGERPRINT, NICHT NUR DIE
        // ALGO-SIGNATUR. Der Befund woertlich: "Mess-Resume nicht an den neuen Fingerprint gekoppelt --
        // Stamp bleibt resume-v5, prueft nur algo_sig -> g++-16 16.0.1->16.3 baut die DLL neu, uebernimmt
        // aber ALTE Messwerte aus result.csv". Das ist GENAU der 'neue DLL / alte Messwerte'-Bug, den der
        // Neuanker heilen soll -- und er ueberlebte den Neuanker, weil der Resume-Stamp die einzige Stelle
        // war, die von der neuen Identitaet nichts wusste: algo_sig deckt die ORGAN-Achsen, kein einziges
        // Toolchain-, System- oder bvset-Glied. Zwei Baue, die sich in Compiler-Realversion, opt-Flags,
        // Zellwerten oder Enable-Menge unterscheiden, hatten denselben Stamp.
        //
        // DIE QUELLE IST DIESELBE WIE FUER DAS SKIP-GATE -- und ab T2-A/K2-NB ist das nicht mehr nur
        // DIESELBE FUNKTION, sondern DERSELBE GELESENE WERT: b.fingerprint traegt genau die Zahl, die
        // provision_core EINMAL je Job beim Provider geholt, als Skip-Erwartung verglichen und als
        // `.fingerprint`-Sidecar geschrieben hat (Muster b.algo_sig). Vorher rief diese Stelle
        // cfg.bestand_fingerprint_fn ein ZWEITES Mal -- dieselbe std::function garantiert keinen
        // identischen Rueckgabewert; ein zustandsabhaengiger Provider konnte mit X pruefen und mit Y
        // stempeln. Es gibt jetzt EINEN Lese-Punkt, nicht mehr zwei, die man gleich HALTEN muss.
        //
        // DIE FORM WIRD GEPRUEFT (T2-A/K2-NB, Codex-Haertung (b)): 128 Hex-Zeichen, ueber die EINE
        // Lese-Wahrheit des Sidecars (detail::fp_is_hex_128, fingerprint_sidecar.hpp -- dieselbe
        // Zeichenmenge, die auch key_from_hex akzeptiert). OHNE diese Wache reiste der Wert ROH in eine
        // Ein-Zeilen-Datei: ein '\n' darin haette sie aufgetrennt, und die erste Zeile waere fuer einen
        // KUERZEREN Fingerprint ein gueltiger Stamp gewesen (belegt am Objekt im Biss, s. Test (6f)).
        // FAIL-CLOSED bei Verstoss: kein Resume-Anspruch fuer diese Binary UND kein frischer Stamp -- der
        // Alt-Stamp wird stattdessen entfernt. Nie stumm: die Zeile benennt Binary und Form-Verstoss.
        //
        // Feld-Ordnung: der Stamp behaelt seine kLazyResumeRowsKey-Ordnung -- das "|fpr="-Feld haengt am
        // PRAEFIX, "|rows=" bleibt der Schwanz (Leser und Schreiber teilen den Praefix, die Ordnung bleibt
        // damit strukturell gedeckt).
        // OHNE PROVIDER (byte-neutraler Default, oder T2-C fail-closed bei unbekannter Tier-Realversion)
        // gibt es kein Feld -- und dann traegt kein Lauf einen Skip-Anspruch, weil dll_is_current ohne
        // Erwartung IMMER false liefert: die Binary wird neu gebaut und faellt unten an der b.skipped-Wache
        // ohnehin aus dem Resume. Die beiden Wachen greifen also ineinander, nicht nebeneinander.
        //
        // WARUM ERST HIER (nach dem Bau-Fehler-Zweig): ein Stamp ist die Zusage "dieser Messstand gilt fuer
        // GENAU diese Binary". Fuer eine Binary, die es nicht gibt, gibt es die Zusage nicht -- der
        // Fehler-Zweig oben ENTFERNT den Stamp, er bildet keinen. Die Stelle steht damit dort, wo ihr Wert
        // gebraucht wird, und die Form-Wache meldet nur ueber Binaries, die real gebaut wurden.
        std::string binary_resume_stamp = resume_stamp_prefix;
        if (!b.algo_sig.empty()) binary_resume_stamp += "|algos=" + b.algo_sig;
        bool fpr_form_verletzt = false;
        if (!b.fingerprint.empty()) {
            if (detail::fp_is_hex_128(b.fingerprint)) {
                binary_resume_stamp += "|fpr=" + b.fingerprint;
            } else {
                fpr_form_verletzt = true;
                std::cerr << "[" << measurement::LogAndContinueInfraPolicy::log_prefix() << ": "
                          << measurement::infra_error_label(measurement::InfraErrorClass::ArtefaktIo) << "] binary_id='"
                          << b.binary_id << "' Fingerprint ist nicht 128-hex (Laenge " << b.fingerprint.size()
                          << ") -- der Mess-Resume dieser Binary ist DEAKTIVIERT und es wird kein Stamp "
                             "geschrieben (ein Wert ausserhalb des Hex-Alphabets kann die Ein-Zeilen-Ablage "
                             "auftrennen und einen fremden Stand zertifizieren)\n"
                          << std::flush;
                // UND DER ALT-STAMP FAELLT SOFORT, nicht erst im Schreib-Block unten: dieser Aufruf kann
                // vorher aussteigen (nicht ladbare .so, kein Mess-Interface -> return), und dann bliebe
                // eine Marke liegen, die ein Lauf mit einem KUERZEREN Fingerprint als gueltig liest --
                // genau die Luecke, die diese Wache schliesst. Muster und Begruendung wie im
                // Bau-Fehler-Zweig (TP1FK1-B10/CX-W4): geurteilt wird ueber den EIGENEN error_code UND
                // das Ist, eine unbelegte Entfernungs-Behauptung gibt es nicht.
                if (cfg.per_binary_subdirs && !b.output.parent_path().empty()) {
                    std::filesystem::path const alt_stamp = b.output.parent_path() / "result.csv.stamp";
                    std::error_code             fpr_ec;
                    std::filesystem::remove(alt_stamp, fpr_ec);
                    std::error_code fpr_ist_ec;
                    if (static_cast<bool>(fpr_ec) || std::filesystem::exists(alt_stamp, fpr_ist_ec))
                        std::cerr << "[" << measurement::LogAndContinueInfraPolicy::log_prefix() << ": "
                                  << measurement::infra_error_label(measurement::InfraErrorClass::ArtefaktIo)
                                  << "] binary_id='" << b.binary_id << "' result.csv.stamp NICHT entfernt ("
                                  << (fpr_ec ? fpr_ec.message() : std::string{"liegt nach dem Entfernen noch"})
                                  << ") in " << alt_stamp.parent_path().string()
                                  << " -- der Resume-Anspruch dieser Ablage ist damit NICHT zurueckgezogen; "
                                     "der Ordner MUSS von Hand geraeumt werden\n"
                                  << std::flush;
                }
            }
        }
        // Mess-RESUME (#139 + Audit K8): Vollstaendig+aktuell => Binary uebersprungen, ihre Zeilen unveraendert
        // uebernommen. Stale => Neu-Messung. TP1FK1-B10: erreicht wird der Zweig nur noch fuer b.ok()-Binaries
        // (gebaut ODER versions-aktuell uebersprungen) -- fuer eine EXISTIERENDE Binary ist der resumierte
        // Stand die Wahrheit, fuer eine nicht herstellbare war er es nie.
        // [NACHGEFUEHRT 2026-08-06, T2-A/F4: die Klammer "(gebaut ODER versions-aktuell uebersprungen)" ist
        //  HISTORIK -- der GEBAUTE Fall faellt ab jetzt heraus, s. die b.skipped-Kopplung direkt darunter.
        //  Der Satz davor bleibt richtig und bleibt deshalb stehen: der Bau-FEHLER-Fall war und ist vorher
        //  abgefangen; die Verschaerfung betrifft ausschliesslich die erfolgreich NEU gebaute Binary.]
        //
        // T2-A/F4 (Owner-KERN Zaehler-Resume, abend-10) -- DER RESUME-ANSPRUCH IST AN b.skipped GEKOPPELT.
        // Ein Resume ist die Aussage "an dieser Binary hat sich nichts geaendert, ihre Messwerte gelten
        // weiter". Diese Aussage darf nur treffen, wer die Binary NICHT ANGEFASST hat. b.skipped ist genau
        // dieses Praedikat: der Orchestrator meldet es fuer eine DLL, die er ueber dll_is_current als
        // fingerprint-aktuell vorgefunden hat -- sie ist Byte fuer Byte die, die die alten Zeilen erzeugt
        // hat. Ist b.skipped FALSCH, wurde in DIESEM Lauf real kompiliert; die entstandene .so ist ein
        // anderes Artefakt als das gemessene, und alte Messwerte auf sie zu buchen ist die Luege, gegen die
        // der ganze Neuanker gebaut ist. Der Stamp-Vergleich allein reichte dafuer nicht: er faengt den
        // Fall erst, wenn der Fingerprint sich AENDERT -- ein Neubau aus anderem Grund (fehlende DLL,
        // geraeumter Ordner, fehlgeschlagener Vorlauf) traegt denselben Fingerprint und waere durchgerutscht.
        // Zwei unabhaengige Wachen, die dieselbe Frage aus verschiedenen Richtungen stellen.
        //
        // T2-A/K2-NB, DRITTE Klammer: ein Fingerprint, der die Form verletzt, entzieht den Anspruch. Der
        // Stamp, gegen den hier verglichen wuerde, traegt dann NICHT das |fpr=-Feld -- er waere der Stamp
        // einer SCHWAECHEREN Zusage und koennte auf einen fremden Stand passen. Fail-closed: neu messen.
        //
        // F8 (OFFENE OWNER-FRAGE, Ledger 06.08. mittag-12) -- DIE ERWARTUNG, DIE HIER FESTGESCHRIEBEN IST:
        // "gleicher Fingerprint => Messwerte uebertragbar, AUCH nach einem Neubau". Der Fingerprint IST die
        // Bau-Identitaet (Organ-Achsen-Versionen + Toolchain-Realversion + opt/ext/gate/atomic128 + bvset +
        // Zellwerte). Ist er unveraendert, ist die neu gebaute DLL aequivalent zur alten, und die alten
        // Zeilen gelten weiter -- das ist die DOKTRIN, nicht ein Leck. Konkret: faellt die DLL weg und wird
        // mit UNVERAENDERTEM Fingerprint neu gebaut, misst DIESER Lauf neu (b.skipped==false, korrekt); ein
        // SPAETERER Lauf findet DLL + Sidecar + Stamp vor und resumiert -- gewollt. Waere das falsch, waere
        // nicht b.skipped zu haerten, sondern der Fingerprint unvollstaendig (und dann traefe der ganze
        // Neuanker nicht). Der einzige Fall, in dem es weh taete, ist ein NICHT-deterministischer Bau --
        // das waere ein Determinismus-Posten, kein Resume-Posten. Test (6g) schreibt die Erwartung fest.
        if (cfg.resume_completed_binaries && cfg.per_binary_subdirs && b.skipped && !fpr_form_verletzt) {
            std::string resumed_rows;
            if (lazy_try_resume_binary(b.output.parent_path(), binary_resume_stamp, &resumed_rows)) {
                oc.resumed_csv_rows = std::move(resumed_rows);
                oc.resumed_binaries = 1;
                // T2-A/F4-NB2 (Befund 2): ein geglueckter Resume IST der Zertifikats-Fall -- der Stamp
                // dieser Ablage hat gerade gegen den vollen Praefix (inkl. |fpr=) bestanden.
                oc.mess_front_faehig = true;
                return oc; // Resume-Skip: kein Progress (wie das frueherer continue-vor-fire_progress)
            }
        }

        // (2) LADEN: DLL -> IAnatomyBase* -> Sub-Interfaces via Dock-Vertrag. AnatomyModuleLoader::load ist thread-safe
        //     fuer verschiedene .so (dlopen glibc-serialisiert, kein statischer Loader-Zustand) -> Pool-unbedenklich.
        anatomy_loader::AnatomyModuleHandle handle;
        int const                           st = anatomy_loader::AnatomyModuleLoader::load(b.output, handle);
        if (st != anatomy_loader::status_ok) {
            oc.load_failed = 1;
            // A15/FK-1: bis hier war der Lade-Fehler ein ZEILEN-NICHTS ohne jedes Fehlerklassen-Log -- nur
            // der Aggregat-Zaehler load_failed wusste davon. Jetzt: klassifiziertes D2-Log (Praefix aus der
            // EINEN Behandlungs-Politik, Etikett aus der EINEN Taxonomie) + eine ehrliche Zeile. Die Binary
            // EXISTIERT -- es fehlt die Mess-QUELLE; deshalb SourceUnavailable ("n/a") und NICHT
            // nicht_gebaut. Beide Aussagen bleiben so unterscheidbar.
            std::cerr << "[" << measurement::FailedCellD2Policy::log_prefix() << ": "
                      << measurement::sample_status_label(measurement::SampleStatus::SourceUnavailable)
                      << "] binary_id='" << b.binary_id << "' .so nicht ladbar (loader_status=" << st << ") -> "
                      << b.output.string() << "\n"
                      << std::flush;
            LazyMeasuredRow marker = make_marker_row(b.binary_id);
            marker.sample_status   = measurement::SampleStatus::SourceUnavailable;
            oc.rows.push_back(std::move(marker));
            return oc;
        }
        // (2b) M-1/D-2 -- DER MESS-VERTRAG CEB <-> TIER-BINARY (LEDGER:3319, Owner-KERN F2/F6).
        //      Die Binary hat geladen. Bevor irgendetwas an ihr gemessen wird, muss sie DIESELBE
        //      Mess-Ausstattung DEKLARIEREN, die diese CEB einkompiliert hat. Bis M-1 las die Deklaration
        //      (measurement_line/measurement_entries) NIEMAND -- das Tier durfte behaupten, was es wollte.
        //
        //      WARUM HIER UND NICHT IM LOADER: der Loader ist ein reiner dlopen-Wrapper (bewusst entkoppelt,
        //      Doku 24 Paragraf 8.6) und kennt die CEB-Erwartung nicht. Der Vertrag gehoert ans PRUEFDOCK --
        //      genau das sagt LEDGER:3319, und genau dort steht er jetzt.
        //
        //      WARUM VOR acquire_search_algorithm_drive: die Antriebs-Beschaffung ist bereits die erste
        //      Beruehrung der Mess-Flaeche. Eine Binary, die den Identitaets-Vertrag bricht, wird gar nicht
        //      erst angefasst.
        //
        //      FEHLERKLASSE: die Binary EXISTIERT und laedt -- was fehlt, ist ihre Zulassung als Mess-Quelle
        //      dieses Laufs. Das ist dieselbe Lage wie "kein Mess-Interface am Dock" (SourceUnavailable,
        //      ehrliche n/a-Zeile), NICHT "nicht gebaut" und NICHT ein stiller Skip. Fehlende Zeilen sind
        //      der Ehrlichkeits-Doktrin nach als solche zu schreiben, nicht als 0.
        if (auto const mk = pruef_dock::pruefe_mess_konsistenz(handle, cfg.erwartete_mess_zeile); !mk.passed()) {
            oc.load_failed = 1;
            std::cerr << "[" << measurement::FailedCellD2Policy::log_prefix() << ": "
                      << measurement::sample_status_label(measurement::SampleStatus::SourceUnavailable)
                      << "] binary_id='" << b.binary_id << "' " << pruef_dock::mess_konsistenz_meldung(mk) << " -> "
                      << b.output.string() << "\n"
                      << std::flush;
            LazyMeasuredRow marker = make_marker_row(b.binary_id);
            marker.sample_status   = measurement::SampleStatus::SourceUnavailable;
            oc.rows.push_back(std::move(marker));
            return oc;
        }

        pruef_dock::SearchAlgorithmDrive drive;
        if (pruef_dock::acquire_search_algorithm_drive(handle, drive) != pruef_dock::dock_status_ok) {
            oc.load_failed = 1;
            // A15/FK-1: dieselbe Naht eine Stufe weiter -- die DLL laedt, aber der Dock-Vertrag liefert kein
            // Mess-Interface. Auch das ist eine fehlende Mess-Quelle, kein Bau- und kein Zulassungs-Fall.
            std::cerr << "[" << measurement::FailedCellD2Policy::log_prefix() << ": "
                      << measurement::sample_status_label(measurement::SampleStatus::SourceUnavailable)
                      << "] binary_id='" << b.binary_id << "' kein Mess-Interface am Dock -> " << b.output.string()
                      << "\n"
                      << std::flush;
            LazyMeasuredRow marker = make_marker_row(b.binary_id);
            marker.sample_status   = measurement::SampleStatus::SourceUnavailable;
            oc.rows.push_back(std::move(marker));
            return oc;
        }
        auto* obs            = drive.obs;
        auto* ctrl           = drive.ctrl;
        auto* rbk            = drive.rbk;
        auto* scn            = drive.scn;
        oc.loaded            = 1;
        oc.progress_eligible = true; // geladen -> erreicht die Mess-Naht -> feuert (im Merge) Progress, wie das Ist

        std::string const           binary_id = b.binary_id;
        std::filesystem::path const bin_dir   = b.output.parent_path();
        std::string                 per_binary_csv;
        std::size_t                 per_binary_rows      = 0;
        std::size_t                 per_binary_settings  = 0;
        bool                        per_binary_all_valid = true;

        auto measure_under_setting = [&](RuntimeSetting const& s) {
            ++oc.dynamic_settings_total;
            ++per_binary_settings;
            std::string const setting_id  = s.setting_label.empty() ? binary_id : (binary_id + "#" + s.setting_label);
            std::string const workload_id = lazy_extract_workload_id(s.setting_label);
            // T-15 (2026-08-09) -- DIE KLAMMER. Bis hierher stand an dieser Stelle GENAU EIN
            // Mess-Aufruf je Einstellung, und der Drift-Detektor aus #197 hatte im ganzen Repo NULL
            // produktive Aufrufer (Register S5-06 / Ledger 09.08.: "kein einziger realer Messwert
            // laeuft heute durch das Drift-Gate"). Jetzt laeuft die Zeitnahme je Zelle durch das Gate:
            // cfg.drift_gate.reps Wiederholungen, Schwelle und Rerun-Budget AUS DEM PROFIL-XML.
            //
            // SELBSTCHECK: der Mess-Aufruf selbst ist UNVERAENDERT -- dieselben Argumente, dieselbe
            // Verzweigung workload_id.empty(). Was sich aendert, ist ausschliesslich, WIE OFT er laeuft
            // und wer darueber urteilt. Bei drift_gate.reps < 2 ruft die Klammer ihn exakt einmal; die
            // Zeile ist dann byte-identisch zum Stand vor dieser Scheibe.
            //
            // Die beiden Marker klammern den Mess-Aufruf fuer die Verdrahtungs-Wache in
            // test_t15_drift_gate_messschleife: sie prueft, dass es im ganzen Iterator keinen
            // Mess-Aufruf AUSSERHALB dieser Klammer gibt. Wer sie verschiebt, muss die Wache mitnehmen.
            auto const zelle = run_cell_with_drift_gate(
                cfg.drift_gate,
                [&]() -> PermResult {
                    // [T-15-KLAMMER]
                    return workload_id.empty() ? run_observable_perm(*obs, setting_id, cfg.n_ops, cell_pmc)
                                               : run_workload_perm(*obs, rbk, scn, setting_id, workload_id, cfg.n_ops,
                                                                   cfg.workload_seed, cfg.workload_records,
                                                                   &cfg.workload_configs, cell_pmc);
                    // [T-15-KLAMMER-ENDE]
                },
                [](PermResult const& p) { return p.total_ns; }, &std::cerr, setting_id);
            PermResult const& pr = zelle.payload;
            // #45: worker-lokaler Parse statt geteiltem Baum-Round-Trip -- identischer NodeValue, byte-identisch zum Ist.
            if (auto parsed = parse_result_line_to_node_value(pr.line)) {
                ++oc.measured;
                NodeValue const nv = parsed->second;
                LazyMeasuredRow row;
                row.binary_id          = binary_id;
                row.setting_label      = s.setting_label;
                row.setting_id         = setting_id;
                row.observer           = nv.observer;
                row.applied_axis_count = s.applied_axis_count;
                row.total_ns           = pr.total_ns;
                row.n_ops              = pr.n_ops;
                row.timed_ops          = pr.timed_ops;
                row.op_lat             = pr.op_lat;
                row.unified            = pr.unified;
                // M-1/H-B: unified_real ist die Konjunktion aus "die Messung lief" (perm_runner) UND "diese
                // Binary ist ueberhaupt mit Observer gebaut" (cfg). Der zweite Faktor ist neu; ohne ihn
                // schrieb eine [wallclock]-Zeile literal 0 in Zellen, die es nicht wissen konnte. Nur
                // abwertend -- true && false == false, true && true == der bisherige Wert.
                row.unified_real    = pr.unified_real && cfg.mess_observer_ausstattung;
                row.profile_name    = pr.profile_name;
                row.two_phase_valid = pr.two_phase_valid;
                row.sample_status   = pr.sample_status;
                row.pmc             = pr.pmc;
                row.series          = cfg.row_series;
                row.pruefling_type  = cfg.row_pruefling_type;
                row.sweep_axis      = cfg.row_sweep_axis;
                row.working_set_n   = cfg.workload_records;
                row.platform        = cfg.row_platform;
                row.build_version   = cfg.row_build_version;
                row.fairness_mode   = cfg.row_fairness_mode;
                row.h2_score        = cfg.row_h2_score;
                // T-15: die Provenienz der Zahl reist MIT der Zahl. drift_reps == 0 heisst "Gate war
                // aus" -> die Zeile behauptet dann keine Drift-Aussage (D4-Unterscheidung), statt eine
                // 0 zu zeigen, die wie "stabil gemessen" aussaehe.
                row.drift_reps       = zelle.gate_aktiv ? cfg.drift_gate.reps : 0u;
                row.drift_reruns     = static_cast<std::uint32_t>(zelle.reruns);
                row.drift_relative   = zelle.verdict.relative_drift;
                row.drift_bestimmbar = zelle.verdict.bestimmbar;
                row.drift_stabil     = zelle.stable;
                per_binary_all_valid = per_binary_all_valid && row.two_phase_valid;
                if (cfg.per_binary_subdirs) {
                    per_binary_csv += format_csv_row(row);
                    ++per_binary_rows;
                }
                oc.rows.push_back(std::move(row));
            }
        };

        if (ctrl != nullptr && !dyn_dims.empty()) {
            loop.run(*ctrl, dyn_dims, measure_under_setting);
        } else {
            RuntimeSetting s{}; // leeres Label -> setting_id == binary_id
            measure_under_setting(s);
        }

        // (E): per-Binary-Ergebnis-CSV schreiben (isoliert je bin_dir) + Resume-Stempel nur nach verifiziertem Write.
        if (cfg.per_binary_subdirs && !per_binary_csv.empty() && !bin_dir.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(bin_dir, ec);
            bool csv_write_ok = false;
            {
                std::ofstream pf{bin_dir / "result.csv", std::ios::trunc};
                if (pf) {
                    pf << lazy_csv_header() << per_binary_csv;
                    pf.flush();
                    csv_write_ok = pf.good();
                }
            }
            if (csv_write_ok && per_binary_rows == per_binary_settings && per_binary_rows > 0 && per_binary_all_valid &&
                !fpr_form_verletzt) {
                std::ofstream sf{bin_dir / "result.csv.stamp", std::ios::trunc};
                if (sf) {
                    sf << binary_resume_stamp << kLazyResumeRowsKey << per_binary_rows << "\n";
                    // T2-A/F4-NB2 (Befund 2): die MESS-FRONT haengt an DIESEM Schreibvorgang, nicht an der
                    // Absicht dazu. Geprueft wird nach dem expliziten close() -- ein Fehler beim letzten
                    // Flush (voller Datentraeger, Quota, I/O) faellt erst dort an, und ohne die Pruefung
                    // zaehlte die Front eine Zelle, deren Zertifikat nie auf der Platte ankam. Dieselbe
                    // Ueberlegung wie bei schreibe_atomar (batch_planner.hpp), nur eine Ebene hoeher.
                    sf.flush();
                    sf.close();
                    oc.mess_front_faehig = sf.good();
                }
            } else {
                // Der frische Stand ist NICHT zertifizierbar (Write gescheitert, Zeilen unvollstaendig, eine
                // Zwei-Phasen-Zeile ungueltig ODER -- T2-A/K2-NB -- der Fingerprint verletzt die 128-hex-Form
                // und darf deshalb in keine Ein-Zeilen-Ablage) -- also faellt der Resume-Anspruch. Die
                // Mess-Zeilen selbst sind davon unberuehrt: sie reisen in oc.rows in die globale CSV, und
                // die per-Binary-CSV dieses Laufs steht bereits geschrieben (nichts wird geloescht).
                //
                // Review-Befund Z-01/GA-02 (Geschwister-Stelle von TP1FK1-B10/CX-W4): das Entfernen wird
                // GEPRUEFT, mit EIGENEM error_code statt des wiederverwendeten ec und gegen das Ist. Frueher
                // sah niemand Rueckgabewert oder Fehler an: blieb der ALTE Stamp liegen (Rechte, ro-Mount,
                // Verzeichnis statt Datei), zertifizierte er im Folgelauf den frischen, gerade als nicht
                // zertifizierbar erkannten Stand als gueltigen Messstand -- lautlos. Der Resume-Leser faengt
                // das nur ab, solange Konfig-Praefix oder Zeilenzahl abweichen; stimmen beide zufaellig
                // ueberein (gleiche Konfiguration, gleiche Zeilenzahl, nur eine Zeile ungueltig), greift
                // keine seiner Wachen. Deshalb ist der Fehlschlag hier selbst ein Befund: beziffert,
                // geflusht, mit benannter Hand-Arbeit -- nie stumm.
                std::error_code stamp_ec;
                std::filesystem::remove(bin_dir / "result.csv.stamp", stamp_ec);
                std::error_code stamp_ist_ec;
                bool const      stamp_bleibt =
                    static_cast<bool>(stamp_ec) || std::filesystem::exists(bin_dir / "result.csv.stamp", stamp_ist_ec);
                if (stamp_bleibt)
                    std::cerr << "[" << measurement::LogAndContinueInfraPolicy::log_prefix() << ": "
                              << measurement::infra_error_label(measurement::InfraErrorClass::ArtefaktIo)
                              << "] binary_id='" << binary_id << "' result.csv.stamp NICHT entfernt ("
                              << (stamp_ec ? stamp_ec.message() : std::string{"liegt nach dem Entfernen noch"})
                              << ") in " << bin_dir.string()
                              << " -- der Resume-Anspruch dieser Ablage ist damit NICHT zurueckgezogen (ein "
                                 "Alt-Stamp darf keinen unvollstaendigen/ungueltigen Stand als Messstand "
                                 "zertifizieren); der Ordner MUSS von Hand geraeumt werden\n"
                              << std::flush;
            }
        }

        // Storage #51 (per-bin_dir isoliert: eigener Ordner/mc-Prozess je Binary -> im Parallel-Debug thread-safe, da
        // unabhaengige Ziele). Der SYNCHRON-Kontrakt bleibt zellintern (kein async/detached); nur die ZELLEN ueberlappen
        // und NUR im Debug (keine Mess-Timing-Garantie, §61-MODI). Fehler behandelt der Client (MESSEN WEITER).
        if (cfg.per_binary_subdirs && !bin_dir.empty()) {
            // TP1FK1-B2 (Codex-Befund CX-W1): synchron pushen; wirft der Push, klassifiziert loggen (MESSEN
            // WEITER) UND den vorgemerkten Bestandslog-Eintrag dieser Binary verwerfen. Der Store hat den Satz
            // nie erhalten -- ein Eintrag im geteilten Dokument waere unter dem Bau-Filter des Folgelaufs ein
            // stiller Verlustpfad. Vorher fing dieser Zweig den Wurf NUR mit std::cerr, und das unbedingte
            // bestandslog_flush() am Lauf-Ende registrierte den unbestaetigten Bestand doch (der sichtbare,
            // aber folgenlose Faenger). Ohne aktives Bestandslog wird nullptr gereicht -> reines Loggen.
            mess_pfad_synchron_push(cfg.cache_push, bin_dir, cfg.build_version, binary_id, cfg.output_dir,
                                    bestandslog_active ? &lager : nullptr);
            if (cfg.measurement_sink) {
                std::error_code             sec;
                std::filesystem::path const rcsv = bin_dir / "result.csv";
                if (std::filesystem::exists(rcsv, sec)) // Ebene C: result.csv -> measure-drop (Baum/<stem>/result.csv)
                    cfg.measurement_sink(rcsv, bin_dir.filename().string() + "/result.csv");
            }
            // G-E3 (A1-Lager-Rest-Welle): die MESS-RUECKSCHRIEB-NAHT. Je GEMESSENER Zelle einen
            // BestandEintrag ins measurement-Genus vormerken -- thread-sicher (MesswertRunState haelt
            // einen Mutex; im Debug-Mess-Pool ueberlappen die Zellen). Registriert wird NUR, wenn die
            // result.csv wirklich existiert: ein Messwert-Eintrag ohne Messdatum waere unter einem
            // kuenftigen Mess-Filter derselbe stille Verlustpfad, den CX-W1 fuer Binaries schloss.
            if (mess_bestandslog_active) {
                std::error_code             mec;
                std::filesystem::path const rcsv = bin_dir / "result.csv";
                if (std::filesystem::exists(rcsv, mec)) {
                    auto const      mkey = cfg.mess_bestand_key_of(bin_dir);
                    std::error_code gec;
                    auto const      bytes = std::filesystem::file_size(rcsv, gec);
                    (void)mess_lager.observe(mkey.value_or(std::string{}), cfg.bestand_zelle,
                                             bestand_eintragspfad(bin_dir, cfg.output_dir),
                                             gec ? std::uint64_t{0} : static_cast<std::uint64_t>(bytes), binary_id,
                                             bestandslog::now_utc_iso(), cfg.mess_bestand_versions);
                }
            }
        }
        return oc; // handle: RAII entlaedt die DLL beim Verlassen der Zelle
    };

    // #45: DISPATCH. measure_parallelism<=1 => sequentiell (EIN pmc, in Reihenfolge -> byte-identisch zum Ist,
    // Measure/Release/Default). >1 (nur Debug) => Worker-Pool, JE Worker ein eigener pmc; collect_ordered liefert die
    // Outcomes in INDEX-Ordnung (results[j]) -> deterministisch unabhaengig von der Ausfuehrungsreihenfolge.
    // S1 (§62-B Log-Flush, Befund 6h-stumm): geflushtes Mess-Fortschritts-Testat je fertiger Zelle (zeit-gated,
    // thread-sicher -- im Debug-Pool feuert genau EIN Worker je Intervall). Rein auf std::cerr -> golden/CSV-NEUTRAL.
    ProgressHeartbeat        measure_hb{"mess-zelle", builds.size()};
    std::size_t              observed_max_concurrency = 0;
    std::vector<CellOutcome> outcomes                 = collect_ordered<CellOutcome>(
        builds.size(), cfg.measure_parallelism, [] { return make_pmc_source(); },
        [&](std::unique_ptr<measurement::IPmcSource>& ctx, std::size_t j) {
            CellOutcome oc = measure_one_binary(builds[j], ctx.get());
            measure_hb.tick(); // S1: je fertig gemessener/geladener Zelle ein geflushtes Fortschritts-Testat
            return oc;
        },
        &observed_max_concurrency);
    measure_hb.done(); // S1: Mess-Fenster abgeschlossen -- genau eine geflushte Abschluss-Zeile
    if (cfg.measure_parallelism > 1)
        std::cerr << "[#45] paralleler Mess-Loop (Debug): pool=" << cfg.measure_parallelism
                  << " zellen=" << builds.size() << " beobachtete-Spitze=" << observed_max_concurrency << "\n";

    // #45: MERGE in KANONISCHER builds-Reihenfolge -> deterministische CSV (kein interleaved Append; Sortierung wie der
    // sequentielle Loop). Progress an der Per-Binary-Naht SEQUENTIELL im Merge (progress_prev ist reihenfolge-abhaengig
    // -> genau EIN Sequenzierungs-Thread). this_progress_cursor == j (die fenster-relative Perm-Position, wie im Ist).
    std::uint64_t plan_gemessene_atome = 0; // T2-A/F4: die Mess-Front dieses Laufs (Zellen, nicht Zeilen)
    // T2-A/F4-NB2 (Codex-Voll-Scope, Befund 2) -- DIE MESS-FRONT IST EIN PRAEFIX, KEINE BILANZ.
    //
    // DER BEFUND WOERTLICH: die Zahl lief bei JEDER irgendwo erfolgreichen Zelle hoch. Fuer [Fehler,
    // Erfolg, Erfolg] stand `gemessen=2`, obwohl das gedeckte Praefix 0 ist -- die erste Zelle hat keine
    // Zeilen. Wer diese Zahl als Front liest (und genau dazu ist eine Ablage da, die "wo stand ich"
    // beantworten soll), ueberspringt die kaputte Zelle. Der BAU-Zaehler nebenan war seit T2-A/F4
    // praefix-treu; zwei Felder derselben Zeile mit zwei Semantiken sind genau die Sorte zweiter Wahrheit,
    // gegen die diese Ablage gebaut ist.
    //
    // DIE ORDNUNG TRAEGT: outcomes ist die builds-Ordnung, builds ist im Mess-Pfad positions-treu zu
    // `indices` (der Bau-Filter laeuft nur im planer-getriebenen provision-Zweig, den dieser Lauf nicht
    // betritt) -- ein Praefix ueber outcomes IST damit ein Praefix ueber die Plan-Atome.
    bool mess_praefix_intakt = true;
    for (std::size_t j = 0; j < outcomes.size(); ++j) {
        CellOutcome& oc = outcomes[j];
        if (mess_praefix_intakt) {
            if (oc.mess_front_faehig)
                ++plan_gemessene_atome;
            else
                mess_praefix_intakt = false; // ab hier deckt die Front nichts mehr -- wie beim Bau-Zaehler
        }
        result.resumed_csv_rows += oc.resumed_csv_rows;
        result.resumed_binaries += oc.resumed_binaries;
        result.load_failed += oc.load_failed;
        result.loaded += oc.loaded;
        result.dynamic_settings_total += oc.dynamic_settings_total;
        result.measured += oc.measured;
        for (auto& row : oc.rows) result.csv_rows.push_back(std::move(row));
        if (oc.progress_eligible)
            fire_progress(builds[j].index, fenster_cursor_of(builds[j].index, j)); // TP1FK1-B5: Fenster-Index
    }

    // T2-A/F4 -- DIE MESS-FRONT DES PLANS (die zweite Haelfte des Owner-KERN "kompiliert/SEPARAT gemessen").
    //
    // Dieser Lauf ist der MESS-Lauf; der planer-getriebene Bau-Pfad haengt an provision_only und ist hier
    // gar nicht gelaufen. Er schreibt deshalb ausschliesslich das Feld, das ihm gehoert, und laesst das
    // Bau-Feld unangetastet stehen (`kompiliert` kommt aus dem gelesenen Stand).
    //
    // ER IST GROB, NICHT ZWEITE AUTORITAET: der feinkoernige Mess-Resume ist und bleibt die per-Binary
    // result.csv+stamp-Naht aus T2-A/K2 -- sie arbeitet je Binary und kennt den vollen Fingerprint. Der
    // Plan-Zaehler beantwortet die GROBE Frage des Planers ("wie weit ist dieser Plan gemessen") und wird
    // deshalb EINMAL am deterministischen Ende der Mess-Phase geschrieben, nicht je Zelle im Pool.
    //
    // T2-A/F4-NB2 (Befund 2): und er ist ein PRAEFIX. Geschrieben wird die Zahl der FUEHRENDEN
    // zertifizierten Zellen dieses Laufs -- OHNE max() gegen den Alt-Stand. Das ist die Umkehr einer
    // frueheren Bequemlichkeit und bewusst so: waechst die Front nur noch, behauptet sie nach einem
    // Rueckschlag (Zertifikat einer fuehrenden Zelle weg, Neu-Messung misslungen) eine Deckung, die es
    // nicht mehr gibt. Sinkt sie, kostet das im schlimmsten Fall einen erneuten Blick auf Zellen, die
    // ihren per-Binary-Stamp ohnehin noch haben und dort in Sekunden resumieren. Die teure Richtung ist
    // die andere.
    // EHRLICHE GRENZE: die Zahl beschreibt DIESEN Lauf ueber DIESE Selektion. Zwei Prozesse, die
    // gleichzeitig verschiedene Teile derselben Selektion messen, wuerden einander ueberschreiben -- das
    // ist heute ausgeschlossen (Mess-Exklusivitaet, EIN CEB je Zelle) und waere sonst ein eigener Posten.
    //
    // FAIL-CLOSED: passt der Plan nicht (anderer Stempel, andere Selektion, kein Plan da), wird NICHTS
    // geschrieben -- lieber keine Zahl als eine, die gegen einen fremden Plan zaehlt. Der Fall sagt es
    // literal, damit niemand eine stumme Ablage fuer eine leere haelt.
    // Die aeussere Klammer ist die ABWESENHEIT DES GEGENSTANDS, keine Ersparnis: ohne benannte Ablage gibt
    // es kein Dokument, an das eine Mess-Front geschrieben werden koennte -- also auch keine Frage.
    if (!cfg.batch_plan_datei.empty()) {
        // T2-A/F4-NB3 (Befund 1 + Auflage 2): dieselbe FAIL-CLOSED-Klammer wie im Bau-Weg, und zwar VOR
        // dem Lesen. Ohne Anker traegt der Stempel das Wort `ohne-anker` und wuerde JEDEN ankerlosen Plan
        // annehmen, egal aus welchem Bau-Stand; mit formlosen Atomen sieht er gueltig aus und deckt
        // trotzdem Atome, die dll_is_current nie sieht. Beides heisst hier: kein Plan-Zaehler.
        //
        // T2-A/F4-NB2 (Korn-Divergenz): das Korn kommt aus DERSELBEN Quelle wie im Bau-Weg
        // (plan_slice_korn). Hier stand bis hierher bestandslog::kBuildSliceGrain als Literal -- der
        // Mess-Lauf bildete damit bei abweichendem Korn einen ANDEREN Stempel als der Bau-Lauf, der den
        // Plan geschrieben hat, und meldete "kein passender Batch-Plan" fuer einen Plan, der danebenlag.
        // T2-A/F4-NB2 (Befund 3): dieselbe Bau-Identitaets-Bindung wie im Bau-Weg -- der Mess-Lauf findet
        // den Plan seines Bau-Laufs nur, wenn er GEGEN DENSELBEN BAU-STAND misst. Genau das soll er.
        PlanAnkerBefund const mess_anker = plan_anker_befund(view, cfg, indices, plan_slice_korn(cfg));
        if (!mess_anker.traegt()) {
            melde_plan_ablage_ohne_anker(cfg, indices.size(), mess_anker, "mess-weg");
        } else {
            std::string const plan_stamp    = mess_anker.stamp;
            std::string const plan_rows_key = kLazyResumeRowsKey;
            auto const        faecher = bestandslog::read_batch_plan(cfg.batch_plan_datei, plan_stamp, plan_rows_key);
            if (!faecher.has_value()) {
                std::cerr << "[bestandslog] plan-zaehler: kein zu dieser Selektion passender Batch-Plan unter "
                          << cfg.batch_plan_datei.string() << " -- die Mess-Front wird NICHT fortgeschrieben\n"
                          << std::flush;
            } else {
                std::filesystem::path const z_datei{cfg.batch_plan_datei.string() + ".zaehler"};
                auto const alt = bestandslog::read_phasen_zaehler(z_datei, plan_stamp, *faecher, plan_rows_key);
                std::uint64_t const kompiliert = alt.has_value() ? alt->kompiliert : 0;
                // Die Front ueberholt die Bau-Front nie: gemessen werden kann nur, was gebaut ist. Diese
                // eine Klammer haelt die Invariante, die der Leser spaeter erzwingt (parse_phasen_zaehler:
                // g > k ist unglaubwuerdig). Die frueher hier stehende max()-Klammer ist mit der
                // Praefix-Semantik gefallen -- s. den Absatz darueber.
                std::uint64_t gemessen = plan_gemessene_atome;
                if (gemessen > kompiliert) gemessen = kompiliert;
                if (!bestandslog::write_phasen_zaehler(z_datei, plan_stamp,
                                                       bestandslog::PhasenZaehler{kompiliert, gemessen},
                                                       faecher->size(), plan_rows_key))
                    std::cerr << "[bestandslog] warn: Mess-Front des Batch-Plans nicht persistiert ("
                              << z_datei.string() << ") -- die Messwerte selbst sind unberuehrt\n"
                              << std::flush;
            }
        }
    }

    // Welle 5 (E-W5-2): §38.b-Fertig-Signal -- done genau EINMAL am Fensterende (nach dem GANZEN Merge).
    fire_progress_done(builds.size());

    // TP1-N2 (B-1): der Mess-Pfad-flush -- hier sind die per-Binary-Pushes laengst geschehen (der
    // Mess-Loop pusht STRIKT SYNCHRON je Binary, W11-Doktrin), es gibt keinen Pump und nichts
    // Ausstehendes; registriert wird am Ende, wie in den beiden anderen Ausgaengen.
    bestandslog_flush();
    // G-E3: derselbe Zeitpunkt fuer das MESSWERT-Genus -- alle Zellen sind gemessen und ihre
    // result.csv-Rueckschriebe geschehen. Inert, wenn das Messwert-Lager nicht verdrahtet ist.
    messwert_flush();

    return result;
}

} // namespace comdare::cache_engine::builder::experiment
