#pragma once
// L-LAZY-E2E (gate-frei, 2026-06-03) — cache_engine_builder_iterator: DIE EINE Host-Treiber-Funktion, die den
// Experiment-B+-Baum END-TO-END LAZY durchläuft: erst statische Kompilierung (Tier-Binary-DLLs), dann dynamische
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

#include "experiment_tree.hpp"            // ExperimentTree / StaticBinaryView / NodeObserverSnapshot
#include "axis_path_serialization.hpp"    // (X) kCompositionAxisNames[19] — Single-Source der 19 seg_*-Spaltennamen
#include "coverage_selection.hpp"         // BuildSelection
#include "runtime_variable_loop.hpp"      // RuntimeVariableLoop / RuntimeSetting (gefiltert-dynamisch)
#include "perm_runner.hpp"                // run_observable_perm / format_perm_result
#include "result_ingest.hpp"             // ingest_result_line
#include "../build_orchestrator/build_orchestrator.hpp"             // BuildOrchestrator / BuildConfig / *Fn
#include "../anatomy_module_loader/anatomy_module_loader.hpp"       // AnatomyModuleLoader / AnatomyModuleHandle
#include "../../anatomy/observable_tier.hpp"                         // IObservableTier
#include "../../anatomy/measurable_workload.hpp"                     // (X): IMeasurableWorkloadV3 + ComdareSegmentLatencyV2 (19 Segmente)
#include "../../anatomy/resource_controllable_tier.hpp"             // IResourceControllableTier

#include <algorithm>   // #165-B: std::nth_element (Gruppen-Median im quality_flag)
#include <array>       // GOAL-L1: LazyMeasuredRow::op_lat (per-Interface-Funktions-Latenzen)
#include <cstddef>
#include <map>         // #165-B: Gruppen-Buckets (binary_id|profile_name) im annotate_quality_flags
#include <cstdint>
#include <cstdio>      // (C-1) std::snprintf für ns_per_op-Formatierung
#include <filesystem>
#include <fstream>     // (E) per-Binary-Ergebnis-CSV schreiben
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// ── Konfiguration des Lazy-E2E-Laufs ──────────────────────────────────────────
struct LazyRunConfig {
    std::size_t           max_binaries  = 150;     // wie viele statische Blätter (erste N) gebaut+gemessen werden
    std::uint64_t         n_ops         = 1000;    // Mess-Workload je dyn-Setting (insert+lookup)
    std::string           build_version = "v1";    // Resume-Marke (BuildOrchestrator .version-Sidecar)
    std::filesystem::path source_dir;              // perm_<id>.cpp (Source-Ausgabe)
    std::filesystem::path output_dir;              // perm_<id>.dll (Build-Ausgabe)
    std::size_t           cores_per_build = 4;     // KF-16b Default (keine Oversubscription)
    std::uint64_t         ram_per_build_bytes    = 0;  // 0 = RAM-Gate aus (nur CPU-Cap)
    std::uint64_t         ram_safety_margin_bytes = 0;
    // (E): je Tier-Binary ein eigener Unterordner output_dir/<stem>/ (DLL + Source + .obj + .cl.log + .version
    // + per-Binary-Ergebnis-CSV). Default false = altes flaches Verhalten (rückwärtskompatibel, opt-in).
    bool                  per_binary_subdirs = false;
    // (D, KF-10): Anzahl der Wiederholungen je (Binary×Setting). Wirkt über die repetition-DynamicDim im Baum
    // (repetition-DynamicDim aus dem Profil-<runtime_dynamic>; hier durchgereicht). Default 3 (0 → 1 normalisiert).
    std::uint32_t         n_repeats = 3;
    // (C-2): per-Segment-Workload-Parameter (run_workload_segmented). seg_batches=0 → kein Segment-Timing (n/a).
    std::uint64_t         seg_ops_per_batch = 4000;  // Operationen je Batch im 19-Segment-Workload (X)
    std::uint64_t         seg_batches       = 32;    // gemessene Batches (Warmup verworfen); 0 = Segment-Timing aus
    std::uint64_t         seg_seed          = 0xB15u; // deterministischer Seed
    // Achse 2 (INC-3): fester Seed für die Workload-Op-Sequenz-Materialisierung. Hängt NUR vom Profil ab (via
    // profile_by_name), NICHT von Binary/Setting/Rep → dieselbe Workload erzeugt bit-identische Sequenzen über
    // ALLE Binaries (Cross-Binary-Vergleichbarkeit = Spalte des kartesischen Kreuzes). [[feedback_two_phase_warmup_mandatory_validity]]
    std::uint64_t         workload_seed     = 42u;
    // Achse 2 (INC-3c): YCSB-Load-Phase. Anzahl der VOR der gemessenen Run-Phase befüllten Sätze (records). 0 →
    // records = n_ops. Key-Verteilung des Profils wird auf [1, records] ausgerichtet (read-heavy/scan treffen Keys).
    std::uint64_t         workload_records  = 0u;
    // Achse 2 (#135): XML-Lastprofil-Registry (id → WorkloadConfig: op-mix/dist/negative_query_pct aus dem XML).
    // Leer → run_workload_perm fällt auf hartcodiertes profile_by_name (env-String) zurück. Befüllt von run_lazy_150
    // via discover_load_profiles(load_profiles/). Die ids sind die Werte der dynamischen Workload-Achse.
    std::map<std::string, wd::WorkloadConfig> workload_configs{};
    // Laufzeit-Obergrenze (System-Limits) für die dyn-Variation (RuntimeVariableLoop clamp gegen caps∩env).
    anatomy::ComdareResourceControlV1 env_limits{};
    // M3v2-SELEKTION (2026-06-18, Task #156): Lauf-weite Tags je Mess-Zeile, damit die Auswertung die drei
    // Mess-Klassen (Basis-320 / Per-Achsen-Sweep / SOTA-Reihen A/B/C) UND die Working-Set-N-Dimension UND die
    // Plattform/Build-Version trennen kann. NUR Metadaten (kein Mess-Einfluss) — sie reisen rein über die
    // LazyMeasuredRow/CSV-Tag-Spalten (series/sweep_axis/working_set_n/platform/build_version), NICHT in die binary_id
    // (die binary_id bleibt die reine Achsen-Rekombination — keine Tag-Verschmutzung der Round-Trip-Identität).
    // Quelle: das Diplomarbeit-Mess-Profil (profile_run_entry/run_profile setzt diese 5 Felder je Basis-/Sweep-/SOTA-Pass).
    std::string row_series       = "-";     // SOTA-Reihe ∈ {A,B,C} bzw. "-" (Basis/Sweep, keine Reihe)
    // #171 (2026-06-20): die Pruefling-Auspraegung "full" (Reihe A self-contained / Originalkonfiguration) vs
    // "abstract" (Reihe B/C Teilmenge + Host-Fallback) — abgeleitet aus merge (sota_catalog::derive_pruefling_type),
    // getaggt je SOTA-Pass; "-" fuer Basis/Sweep/cowfix-v1 (kein Pruefling). REINE Metadaten (kein Mess-Einfluss),
    // reist wie series rein in der CSV-Tag-Spalte — NICHT in die binary_id (binary_id-Drift vermieden).
    std::string row_pruefling_type = "-";   // "full" / "abstract" / "-" (Basis/Sweep)
    std::string row_sweep_axis   = "-";     // gesweepte Achse (z.B. "migration_policy") bzw. "-" (Basis/SOTA)
    std::string row_platform     = "win-x86_64";  // Plattform-Tag (Infra-Agent überschreibt für ZIH-Reihen)
    std::string row_build_version = "m3v2"; // Build-Version-Tag (= BuildVersion-Marke; Default m3v2)
    // working_set_n je Zeile = cfg.workload_records (der N-Sweep ruft den Treiber je N-Wert mit gesetztem records).
    // Mess-RESUME (#139, User 2026-06-10 „Wiedereinstieg bei einem bestimmten nicht fertigen Tier"): Binaries,
    // deren per-Binary result.csv VOLLSTÄNDIG + KONFIGURATIONS-AKTUELL ist (result.csv.stamp == aktueller
    // Config-Stempel: build_version/n_ops/seed/records/ALLE dyn-Dimensionen inkl. Workload-Set + Zeilenzahl;
    // Schema via Header-Identität), werden ÜBERSPRUNGEN — ihre Zeilen fließen unverändert in die globale CSV
    // (LazyRunResult::resumed_csv_rows). Unfertige/stale (z.B. anderer n_ops-Testlauf, andere BuildVersion)
    // werden NEU gemessen — der Zwei-Phasen-Cache-Warmup gilt auf Re-Entry intrinsisch je Op (Mess-Gültigkeit,
    // [[feedback_two_phase_warmup_mandatory_validity]]). Nur wirksam mit per_binary_subdirs.
    bool resume_completed_binaries = true;
};

// ── Eine gemessene CSV-Zeile (Binary × dyn-Setting) ───────────────────────────
struct LazyMeasuredRow {
    std::string          binary_id;        // statische Rekombination (= die Tier-Binary)
    std::string          setting_label;    // dyn. Belegung "axis.var=value/…" (leer = keine dyn-Dimensionen)
    std::string          setting_id;       // binary_id (+ "#" + setting_label) = eindeutiger Baum-Key
    NodeObserverSnapshot  observer{};       // die real gezogenen Observer-Werte (>0 bei echter Messung)
    std::uint64_t        applied_axis_count = 0;  // wie viele Achsen die Steuerung real annahmen
    // (B): Host-Gesamt-Wall-Clock DIESER Messung + Eingabe-n_ops; GOAL-M1.1 (Audit K2): ns_per_op =
    // total_ns/timed_ops (Workload-Pfad: Σ getimte Samples; Legacy: 2*n_ops). timed_ops==0 → ns_per_op=0.
    std::int64_t         total_ns  = 0;
    std::uint64_t        n_ops     = 0;
    std::uint64_t        timed_ops = 0;
    // GOAL-L1: per-Interface-Funktions-Latenzen (Reihenfolge kOpKindNames) — z-Achsen-Quelle der 3D-Auswertung.
    std::array<OpKindLatency, 6> op_lat{};
    // KONSOLIDIERUNG (I1): der EINE konsolidierte Observer-POD (axis_stats[19][8] + seg_ns[19]/Pfad B + Meta).
    // Maßgebliche CSV-Quelle: stat_*-Spalten aus unified.axis_stats, seg_*-Spalten aus unified.seg_ns (Pfad B, reale
    // Komposition). Ersetzt den früheren V3-Snapshot + den Pfad-A-Segment-Timer.
    anatomy::ComdareTierObserverSnapshot unified{};
    bool                 unified_real = false;
    // Achse 2 (INC-3): Lastprofil + Mess-GÜLTIGKEIT (Zwei-Phasen-Cache-Warmup exakt). profile_name leer/"-" =
    // alter fixer Workload (kein Achse-2-Profil). two_phase_valid=false ⇒ Messung UNGÜLTIG (nicht als valide werten).
    std::string          profile_name;
    bool                 two_phase_valid = false;
    // M3v2-SELEKTION (Task #156): die 5 Lauf-/Selektions-Tags je Zeile (aus LazyRunConfig durchgereicht). Reine
    // Metadaten (kein Mess-Einfluss) → ermöglichen die Trennung Basis vs Per-Achsen-Sweep vs SOTA-Reihe A/B/C
    // sowie die Working-Set-N- und Plattform/Build-Version-Achsen in der Auswertung.
    std::string          series        = "-";            // SOTA-Reihe {A,B,C} oder "-"
    std::string          pruefling_type = "-";           // #171: "full" (Reihe A self-contained) / "abstract" (B/C) / "-"
    std::string          sweep_axis    = "-";            // gesweepte Achse oder "-"
    std::uint64_t        working_set_n = 0;              // N (Record-Zahl) dieser Mess-Zeile (= workload_records)
    std::string          platform      = "win-x86_64";   // Plattform-Tag
    std::string          build_version = "m3v2";         // Build-Version-Tag
    // #165-B (P-MD8, 2026-06-20): STATISTISCHER Ausreißer-Flag je Zeile (gate-frei). 1 = ns_per_op dieser Zeile
    // ist ein Ausreißer relativ zum Median der (binary_id, workload/profile_name)-Gruppe (Heuristik s.u.); 0 = nicht.
    // Default 0 (kein Flag) → bestehende Aufrufer unverändert; befüllt OPT-IN durch annotate_quality_flags(rows) VOR
    // der CSV-Emission. WICHTIG (Klarstellung): dies ist NUR der statistische Ausreißer-Flag (rein aus den Mess-Werten
    // ableitbar, ohne Infra/Gate). Die OS-quiesced `system_disturbed`-Provenienz (AP-M1/P-MD2) ist eine GETRENNTE,
    // HELD/Infra-gebundene Sache und NICHT Teil dieser Spalte.
    std::uint32_t        quality_flag  = 0;              // statistischer Ausreißer-Flag (1) / kein Ausreißer (0)
    // #156-De-Risk (2026-06-20): die HW-Performance-Counter (PMC) DIESER Mess-Zeile (aus PermResult::pmc). Default
    // PmcCounters{} = alle 0, available=false → lokal mit NullPmcSource (COMDARE_ENABLE_PMC=OFF) ehrlich 0/available=0;
    // mit Intel-PCM=ON real (Montag Linux+PMC). Die 7 PMC-Felder werden ADDITIV als LETZTE CSV-Spalten emittiert
    // (lazy_csv_header single-source) — bestehende Spalten unberührt → cowfix-v1/tier150-Leser bleiben kompatibel.
    builder::PmcCounters pmc{};
};

// ── (B/C/D/X) EINHEITLICHES CSV-Schema (global + per-Binary identisch) ──────────────────────────────────
//   binary_id;setting;repetition;n_ops;total_ns;ns_per_op;
//   seg_<T0>_ns;…;seg_<T18>_ns;   (19 per-Achsen-Timer-Spalten, kCompositionAxisNames-Reihenfolge)
//   <die 13 differenzierten Observer-Counter>;applied_axes
// (X) Die frühere `na_axes`-Notiz-Spalte ist ENTFALLEN: ALLE 19 Achsen tragen jetzt einen echten per-Achsen-
// Timer (kein „15 Deskriptor-Achsen ohne Timer" mehr). Die 19 seg_*-Spalten = "n/a", wenn das Modul kein
// IMeasurableWorkloadV3 exponiert (seg_real=false) — ehrlich n/a, NICHT 0. Die Spaltennamen werden aus der
// EINEN Single-Source kCompositionAxisNames (axis_path_serialization.hpp) generiert → keine Namens-Drift.
//
// Phase A (2026-06-04) PER-ACHSEN-OBSERVER-SPALTEN: zusätzlich `stat_<achse>_<feld>` je befülltem statistics()-Feld
// (generisch aus der EINEN Single-Source kV3AxisSchema, observable_tier.hpp). CSV-SCHEMA-WAHL = WIDE NAMED COLUMNS
// (nicht long-format), BEGRÜNDUNG: (1) die bestehende CSV ist bereits wide (13 Observer-Counter + 19 seg-Spalten),
// 1 Zeile je Messung — wide bleibt konsistent + direkt vom Thesis-PDF-Pipeline konsumierbar; (2) der V3-POD ist
// generisch [19][8], ABER nur die im Schema BENANNTEN (non-null) Felder werden zu Spalten → die Breite ist gegen
// weitere Felder gedeckelt (Phase B füllt nur leere Schema-Slots, KEINE neue Spalte ausser tatsächlich benannt);
// (3) Schema-Stabilität: kV3AxisSchema IST der Vertrag Schreiber(DLL)↔Spaltenname(Host) → keine Drift. Die
// stat_*-Spalten sind „n/a", wenn die DLL kein Mess-Interface trägt (unified_real=false) — ehrlich n/a, NICHT 0.
//
// SEMANTIK der stat_*-Spalten (Konsistenz-Stand nach dem tier_clear-Reset-Fix 2026-06-04): PER-MESSUNG, WARMUP-FREI
// — KEINE Cumulative-Absolut-Werte. perm_runner::run_observable_perm ruft tier_clear() VOR der Mess-Last; die auto-
// gekoppelten Instanz-Achsen (T1/T2/T3/T7/T8/T10/T17/T18) werden dabei STATISTIK-zurückgesetzt (reset(), nicht nur
// daten-clear()) → ihr V3-Wert = NUR die Op-Zähler DIESER Messung. WICHTIG (Defekt-Fix): vor dem Fix riefen T1/T2/T17
// nur clear() (= nur Daten, stats_ blieb stehen) und T18/T10 gar nichts → ihre Zähler akkumulierten über die 3
// Wiederholungen je (Binary×Setting); der Fix ergänzt reset() für diese 4+1 Organe. Die Scan-Achsen (T4/T5/T9/T11..T16)
// sind in fill_observer_v3 idempotent (reset()+scan je Observe) → Zustand zum Observe-Zeitpunkt; T0/T6 tragen die V1-
// Delta-Counter. Damit ist der stat_*-Block pro Zeile konsistent mit dem V1-Delta-Block (search_*/alloc_*): kein
// doppeltes Zählen über Wiederholungen hinweg.
[[nodiscard]] inline std::string lazy_csv_header() {
    std::string h = "binary_id;setting;repetition;n_ops;total_ns;ns_per_op;";
    // GOAL-L1 (2026-06-12): per-Interface-Funktions-Spalten op_<art>_{n,p50_ns,p99_ns} (Reihenfolge
    // kOpKindNames, single-source perm_runner) — Verarbeitungsdauer je Testdatensatz-Operation GETRENNT
    // je Interface-Funktion (z-Achse der 3D-Diagramme; Ausgabe = Testdaten-Konfig × Tier).
    for (char const* k : kOpKindNames) {
        h += "op_"; h += k; h += "_n;";
        h += "op_"; h += k; h += "_p50_ns;";
        h += "op_"; h += k; h += "_p99_ns;";
    }
    for (std::size_t i = 0; i < kCompositionAxisNames.size(); ++i) {  // 19 seg_<axis>_ns-Spalten, single-source
        h += "seg_"; h += kCompositionAxisNames[i]; h += "_ns;";
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
            if (fld == nullptr) continue;   // ungenutztes / Phase-B-Feld → keine Spalte
            h += "stat_"; h += kCompositionAxisNames[t]; h += '_'; h += fld; h += ';';
        }
    }
    h += "v3_filled_axes;workload;two_phase_valid;";   // Diagnose 19 Achsen befüllt + Achse 2 (Lastprofil + Mess-Gültigkeit)
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
         "pmc_coherence_invalidations;pmc_energy_micro_joules;pmc_available\n";
    return h;
}

/// Extrahiert den repetition_index aus dem setting_label (Segment "repetition.repetition_index=N");
/// "-" wenn die Rep-Dim nicht aktiv ist (kein Segment gefunden).
[[nodiscard]] inline std::string lazy_extract_repetition(std::string const& setting_label) {
    static constexpr char key[] = "repetition_index=";
    std::size_t const p = setting_label.find(key);
    if (p == std::string::npos) return "-";
    std::size_t const b = p + (sizeof(key) - 1);
    std::size_t e = b;
    while (e < setting_label.size() && setting_label[e] != '/' ) ++e;
    return setting_label.substr(b, e - b);
}

/// Achse 2 (INC-3): extrahiert die workload_id aus dem setting_label (Segment "workload.workload_id=X"); leer
/// wenn die Workload-Dim nicht aktiv ist (kein Segment) → Aufrufer fällt auf den alten fixen Workload zurück.
[[nodiscard]] inline std::string lazy_extract_workload_id(std::string const& setting_label) {
    static constexpr char key[] = "workload_id=";
    std::size_t const p = setting_label.find(key);
    if (p == std::string::npos) return {};
    std::size_t const b = p + (sizeof(key) - 1);
    std::size_t e = b;
    while (e < setting_label.size() && setting_label[e] != '/' ) ++e;
    return setting_label.substr(b, e - b);
}

/// Formatiert EINE LazyMeasuredRow als CSV-Zeile (Schema lazy_csv_header). ns_per_op = total_ns/(2*n_ops)
/// (insert+lookup). seg_*-Felder: echte ns wenn seg_real, sonst "n/a" (ehrlich, NICHT 0).
[[nodiscard]] inline std::string format_csv_row(LazyMeasuredRow const& row) {
    auto const& o = row.observer;
    std::string out;
    out.reserve(256);
    out += row.binary_id; out += ';';
    out += (row.setting_label.empty() ? std::string{"-"} : row.setting_label); out += ';';
    out += lazy_extract_repetition(row.setting_label); out += ';';
    out += std::to_string(row.n_ops); out += ';';
    out += std::to_string(row.total_ns); out += ';';
    // GOAL-M1.1 (Audit K2): ns_per_op = total_ns / timed_ops (tatsächlich getimte Einzel-Ops; Workload-Pfad
    // = Σ Samples inkl. Scan-Skips, Legacy = 2*n_ops). Der frühere fixe 2*n_ops-Divisor halbierte alle
    // Lastprofil-Zeilen. timed_ops==0 → 0.
    double const ns_per_op = (row.timed_ops != 0)
        ? (static_cast<double>(row.total_ns) / static_cast<double>(row.timed_ops)) : 0.0;
    { char buf[48]; int const n = std::snprintf(buf, sizeof(buf), "%.3f", ns_per_op);
      out.append(buf, (n > 0) ? static_cast<std::size_t>(n) : 0); }
    out += ';';
    // GOAL-L1: per-Interface-Funktions-Latenzen (Reihenfolge identisch zum Header / kOpKindNames).
    for (auto const& ol : row.op_lat) {
        out += std::to_string(ol.n);      out += ';';
        out += std::to_string(ol.p50_ns); out += ';';
        out += std::to_string(ol.p99_ns); out += ';';
    }
    // (X) die 19 per-Segment-ns (T0..T18) — echt wenn seg_real, sonst ehrlich n/a (NICHT 0). Geschleift über
    // seg_ns[19] in derselben Reihenfolge wie die Header-Spalten (kCompositionAxisNames) — single-source, keine Drift.
    // KONSOLIDIERUNG (I-B): seg_<achse>_ns bevorzugt aus dem EINEN konsolidierten POD = Pfad-B-Timing (reale
    // Komposition, User-Entscheid 2026-06-04). Fallback (additive Übergangsphase, entfällt in I-C): row.seg
    // (Pfad A) für noch nicht migrierte Aufrufer; sonst ehrlich n/a (alte DLL ohne konsolidierte tier_observe).
    auto seg_field = [&](int i) {
        if (row.unified_real) out += std::to_string(row.unified.seg_ns[i]);   // Pfad-B-Timing aus dem EINEN POD
        else                  out += "n/a";                                    // alte/Nicht-Mess-DLL → ehrlich n/a
        out += ';';
    };
    for (int i = 0; i < 19; ++i) seg_field(i);
    // P-MD3 (2026-06-18): die Coverage-Versöhnung. seg_framework_ns/seg_run_total_ns ehrlich n/a, wenn keine Mess-DLL;
    // seg_coverage = Σseg_ns / seg_run_total_ns (gegen die KOMMENSURABLE eigene Wall-Clock des Segment-Laufs → ~1.0,
    // NICHT gegen die unkommensurable Real-Workload-total_ns). seg_run_total_ns==0 → coverage n/a (kein Div-by-0).
    if (row.unified_real) {
        std::int64_t seg_sum = 0; for (int i = 0; i < 19; ++i) seg_sum += row.unified.seg_ns[i];
        out += std::to_string(row.unified.seg_framework_ns); out += ';';
        out += std::to_string(row.unified.seg_run_total_ns); out += ';';
        if (row.unified.seg_run_total_ns > 0) {
            double const cov = static_cast<double>(seg_sum) / static_cast<double>(row.unified.seg_run_total_ns);
            char buf[48]; int const nb = std::snprintf(buf, sizeof(buf), "%.6f", cov);
            out.append(buf, (nb > 0) ? static_cast<std::size_t>(nb) : 0);
        } else { out += "n/a"; }
        out += ';';
    } else { out += "n/a;n/a;n/a;"; }
    // die 4 differenzierten Observer-Counter (search_algo + allocator) — DELTA je Messung (A).
    out += std::to_string(o.search_lookup_count);      out += ';';
    out += std::to_string(o.search_hit_count);         out += ';';
    out += std::to_string(o.search_miss_count);        out += ';';
    out += std::to_string(o.search_insert_count);      out += ';';
    out += std::to_string(o.search_erase_count);       out += ';';
    out += std::to_string(o.search_peak_occupancy);    out += ';';
    out += std::to_string(o.alloc_bytes_allocated);    out += ';';
    out += std::to_string(o.alloc_bytes_in_use);       out += ';';
    out += std::to_string(o.alloc_allocation_count);   out += ';';
    out += std::to_string(o.alloc_deallocation_count); out += ';';
    out += std::to_string(o.alloc_failure_count);      out += ';';
    out += std::to_string(o.observable_axis_count);    out += ';';
    out += std::to_string(o.tier_fill_level);          out += ';';
    out += std::to_string(row.applied_axis_count);     out += ';';   // applied_axes
    // Phase A: die per-Achsen-Observer-Werte stat_<achse>_<feld> (WIDE, generisch aus kV3AxisSchema). Echt wenn
    // unified_real (Modul trägt das Mess-Interface), sonst ehrlich „n/a" (NICHT 0). Reihenfolge IDENTISCH zum Header.
    for (std::size_t t = 0; t < anatomy::kV3AxisCount; ++t) {
        for (std::size_t f = 0; f < anatomy::kV3FieldCount; ++f) {
            if (anatomy::kV3AxisSchema[t].names[f] == nullptr) continue;   // ungenutzt / Phase B → keine Spalte
            if (row.unified_real) out += std::to_string(row.unified.axis_stats[t][f]);   // konsolidierter POD
            else                  out += "n/a";                                           // alte/Nicht-Mess-DLL → ehrlich n/a
            out += ';';
        }
    }
    out += (row.unified_real ? std::to_string(row.unified.filled_axis_count) : std::string{"n/a"});   // filled_axes
    out += ';'; out += (row.profile_name.empty() ? std::string{"-"} : row.profile_name);              // workload (Achse 2)
    out += ';'; out += (row.two_phase_valid ? "1" : "0");                                             // Mess-Gültigkeit (Cache-Warmup)
    // M3v2-SELEKTION (Task #156): die 5 Selektions-/Lauf-Tags (Reihenfolge IDENTISCH zum Header).
    out += ';'; out += (row.series.empty()     ? std::string{"-"} : row.series);
    out += ';'; out += (row.sweep_axis.empty() ? std::string{"-"} : row.sweep_axis);
    out += ';'; out += std::to_string(row.working_set_n);
    out += ';'; out += (row.platform.empty()      ? std::string{"-"} : row.platform);
    out += ';'; out += (row.build_version.empty() ? std::string{"-"} : row.build_version);
    // #171 (2026-06-20): pruefling_type GANZ ANS ENDE (Reihenfolge IDENTISCH zum Header). "-" für Basis/Sweep.
    out += ';'; out += (row.pruefling_type.empty() ? std::string{"-"} : row.pruefling_type);
    // #165-B (P-MD8, 2026-06-20): quality_flag ALS LETZTE Spalte (Reihenfolge IDENTISCH zum Header). 0 = kein
    // Ausreißer / nicht annotiert (Default); 1 = statistischer Ausreißer (s. annotate_quality_flags). Gate-frei.
    out += ';'; out += std::to_string(row.quality_flag);
    // #156-De-Risk (2026-06-20): die 7 PMC/HW-Counter ALS LETZTE Spalten (Reihenfolge IDENTISCH zum Header). Mit
    // NullPmcSource (COMDARE_ENABLE_PMC=OFF) sind alle Werte 0 und pmc_available=0 (ehrlich „nicht real gemessen");
    // mit Intel-PCM=ON real. Additiv → cowfix-v1/tier150-Leser unberührt (leere PMC-Spalten dort = n-a).
    out += ';'; out += std::to_string(row.pmc.cache_misses_l1);
    out += ';'; out += std::to_string(row.pmc.cache_misses_l2);
    out += ';'; out += std::to_string(row.pmc.cache_misses_l3);
    out += ';'; out += std::to_string(row.pmc.dtlb_misses);
    out += ';'; out += std::to_string(row.pmc.coherence_invalidations);
    out += ';'; out += std::to_string(row.pmc.energy_micro_joules);
    out += ';'; out += (row.pmc.available ? "1" : "0");
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
inline constexpr double      kQualityOutlierK = 3.0;   // Median-Multiplikator-Schwelle (benannt/dokumentiert)
inline constexpr std::size_t kQualityMinGroup = 3;     // min. Messpunkte je Gruppe für eine Ausreißer-Aussage

inline void annotate_quality_flags(std::vector<LazyMeasuredRow>& rows) {
    // ns_per_op je Zeile (konsistent mit format_csv_row); -1.0 = nicht messbar (timed_ops==0) → nie Flag/Median.
    auto ns_per_op_of = [](LazyMeasuredRow const& r) -> double {
        return (r.timed_ops != 0) ? (static_cast<double>(r.total_ns) / static_cast<double>(r.timed_ops)) : -1.0;
    };
    // (1) Gruppen-Buckets (binary_id|profile_name) → Indizes der zugehörigen Zeilen sammeln.
    std::map<std::string, std::vector<std::size_t>> groups;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        rows[i].quality_flag = 0;   // Reset (idempotent — Mehrfachaufruf sicher)
        groups[rows[i].binary_id + "|" + rows[i].profile_name].push_back(i);
    }
    // (2) je Gruppe: Median der gültigen ns_per_op (Nearest-Rank, nth_element) → Zeilen > k×Median flaggen.
    for (auto const& [key, idxs] : groups) {
        std::vector<double> vals;
        vals.reserve(idxs.size());
        for (std::size_t i : idxs) { double const v = ns_per_op_of(rows[i]); if (v >= 0.0) vals.push_back(v); }
        if (vals.size() < kQualityMinGroup) continue;   // zu wenig Evidenz → konservativ kein Flag
        std::size_t const mid = vals.size() / 2;
        std::nth_element(vals.begin(), vals.begin() + static_cast<std::ptrdiff_t>(mid), vals.end());
        double const median = vals[mid];
        if (median <= 0.0) continue;                    // degenerierter Median → kein sinnvoller Multiplikator
        double const thr = kQualityOutlierK * median;
        for (std::size_t i : idxs) {
            double const v = ns_per_op_of(rows[i]);
            if (v >= 0.0 && v > thr) rows[i].quality_flag = 1;   // statistischer Ausreißer
        }
    }
}

// ── Ergebnis des Lazy-E2E-Laufs (rein zählend + die Mess-Zeilen; kein ∏-Vektor) ──
struct LazyRunResult {
    std::size_t                  selected   = 0;   // selektierte statische Blätter (== min(max_binaries, view))
    std::size_t                  built      = 0;   // erfolgreich bereitgestellte DLLs (gebaut ODER resumiert)
    std::size_t                  built_new  = 0;   // davon tatsächlich (neu) kompiliert
    std::size_t                  built_skip = 0;   // davon resumiert (versions-aktuell, .version-Sidecar)
    std::size_t                  loaded     = 0;   // DLLs, die geladen + als IObservableTier nutzbar waren
    std::size_t                  load_failed = 0;  // gebaut, aber nicht ladbar / kein Mess-Interface
    std::size_t                  measured   = 0;   // gemessene (Binary × dyn-Setting)-Zeilen, in den Baum ge-ingestet
    std::size_t                  dynamic_settings_total = 0;  // Σ dyn-Settings über alle geladenen Binaries
    std::uint64_t                min_free_ram_bytes = 0;      // RAM-Low-Water-Mark des Build-Schritts
    std::vector<LazyMeasuredRow> csv_rows;          // je gemessene (Binary × dyn-Setting)-Zeile (für CSV)
    BuildStats                   build_stats{};      // Roh-Statistik des Build-Schritts (peak_concurrency, …)
    // Mess-RESUME (#139): Binaries, die per vollständiger+aktueller result.csv übersprungen wurden. Ihre
    // Daten-Zeilen (ohne Header, Schema header-identisch verifiziert) stehen UNVERÄNDERT in resumed_csv_rows —
    // der globale CSV-Schreiber hängt sie VOR den frisch formatierten csv_rows an (Spalten identisch).
    // HINWEIS: resumierte Zeilen werden NICHT erneut in den Experiment-Baum ge-ingestet (die Auswertung läuft
    // über die globale CSV; der Baum trägt nur die in DIESEM Lauf frisch gemessenen Knoten).
    std::size_t                  resumed_binaries = 0;
    std::string                  resumed_csv_rows;
};

// ── Mess-RESUME (#139): Config-Stempel + Vollständigkeits-Prüfung der per-Binary result.csv ───────────────
// Der Stempel kodiert ALLES, was die Mess-Matrix einer Binary bestimmt: BuildVersion (Memento-/Code-Stand der
// DLL — copymem-v1-Ergebnisse sind mit undolog-v1 NICHT mischbar), n_ops/seed/records (Workload-Skala) und
// JEDE dynamische Dimension mit ihrer vollen Werte-Liste (deckt repetition/n_repeats, das Workload-Set aus den
// XML-Lastprofilen und alle Resource-Control-Dims ab). Schema-Drift fängt der Header-Vergleich (lazy_csv_header).
[[nodiscard]] inline std::string lazy_resume_stamp_prefix(LazyRunConfig const& cfg,
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
    std::string s = "resume-v4|build=" + cfg.build_version
                  + "|series=" + cfg.row_series + "|ptype=" + cfg.row_pruefling_type
                  + "|sweep=" + cfg.row_sweep_axis
                  + "|plat=" + cfg.row_platform + "|bv=" + cfg.row_build_version
                  + "|n_ops=" + std::to_string(cfg.n_ops)
                  + "|seed=" + std::to_string(cfg.workload_seed)
                  + "|records=" + std::to_string(cfg.workload_records)
                  + "|env=" + std::to_string(cfg.env_limits.thread_count)
                  + ',' + std::to_string(cfg.env_limits.prefetch_distance)
                  + ',' + std::to_string(cfg.env_limits.pool_budget_bytes)
                  + ',' + std::to_string(cfg.env_limits.batch_size)
                  + ',' + std::to_string(cfg.env_limits.inline_threshold_bytes)
                  + "|wlcfg=";
    // std::map → deterministische id-Sortierung. Je Profil die mess-bestimmenden XML-INHALTSfelder (NICHT seed/n_ops/
    // key_range — die sind Harness-Skala, schon oben). std::to_string(double) ist deterministisch (6 Nachkommastellen).
    for (auto const& [id, c] : cfg.workload_configs) {
        s += id; s += ':';
        s += std::to_string(c.pct_insert) + '/' + std::to_string(c.pct_lookup) + '/' + std::to_string(c.pct_erase)
           + '/' + std::to_string(c.pct_clear) + '/' + std::to_string(c.pct_scan) + '/' + std::to_string(c.pct_rmw)
           + '/' + std::to_string(static_cast<int>(c.key_distribution))
           + '/' + std::to_string(c.zipfian_theta) + '/' + std::to_string(c.negative_query_pct)
           + '/' + std::to_string(c.scan_length_max);
        s += ';';
    }
    s += "|dims=";
    for (DynamicDim const& d : dims) {
        s += d.axis; s += '.'; s += d.variable; s += ':';
        for (std::size_t i = 0; i < d.values.size(); ++i) { if (i) s += ','; s += d.values[i]; }
        s += ';';
    }
    return s;
}

/// Prüft, ob `dir/result.csv` für den aktuellen Lauf VOLLSTÄNDIG + AKTUELL ist (Stamp-Match + Header-Identität
/// + Zeilenzahl), und liefert bei Erfolg die Daten-Zeilen (ohne Header) in *out_rows. Jede Abweichung → false
/// (Binary wird normal gemessen — keine stillen Teil-Übernahmen).
[[nodiscard]] inline bool lazy_try_resume_binary(std::filesystem::path const& dir,
                                                 std::string const& stamp_prefix,
                                                 std::string* out_rows) {
    std::error_code ec;
    std::filesystem::path const csv_p   = dir / "result.csv";
    std::filesystem::path const stamp_p = dir / "result.csv.stamp";
    if (!std::filesystem::exists(csv_p, ec) || !std::filesystem::exists(stamp_p, ec)) return false;

    std::ifstream sf{stamp_p};
    std::string stamp;
    if (!sf || !std::getline(sf, stamp)) return false;
    std::string const rows_key = "|rows=";
    if (stamp.size() <= stamp_prefix.size() + rows_key.size()) return false;
    if (stamp.compare(0, stamp_prefix.size(), stamp_prefix) != 0) return false;            // Config weicht ab
    if (stamp.compare(stamp_prefix.size(), rows_key.size(), rows_key) != 0) return false;  // Format weicht ab
    std::uint64_t expected_rows = 0;
    try { expected_rows = std::stoull(stamp.substr(stamp_prefix.size() + rows_key.size())); }
    catch (...) { return false; }
    if (expected_rows == 0) return false;

    std::ifstream cf{csv_p};
    if (!cf) return false;
    std::string header_line;
    if (!std::getline(cf, header_line)) return false;
    std::string expected_header = lazy_csv_header();
    while (!expected_header.empty() && (expected_header.back() == '\n' || expected_header.back() == '\r'))
        expected_header.pop_back();
    while (!header_line.empty() && header_line.back() == '\r') header_line.pop_back();
    if (header_line != expected_header) return false;                                      // Schema-Drift → neu messen

    std::string rows, line;
    std::uint64_t n = 0;
    while (std::getline(cf, line)) {
        while (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        rows += line; rows += '\n'; ++n;
    }
    if (n != expected_rows) return false;                                                  // unvollständig/abgeschnitten
    if (out_rows != nullptr) *out_rows = std::move(rows);
    return true;
}

/// run_lazy_static_then_dynamic — DIE EINE fehlende Host-Treiber-Funktion. Verdrahtet die volle Lazy-Kette:
/// (1) Haupt/statisch-Iterator über view+selection → BuildOrchestrator (STATISCHE Kompilierung, resumierbar/RAM-gated),
/// (2) je DLL load → IObservableTier + IResourceControllableTier,
/// (3) gefiltert-dynamisch-Iterator (RuntimeVariableLoop über tree.dynamic_filter()) → messen → ingest.
/// `sel` liefert die endlichen View-Indizes (z.B. select_explicit(first N) / select_one_wise(view)); es wird auf
/// die ersten cfg.max_binaries gekappt. compile/gen/ram werden injiziert (Engine-agnostisch wie BuildOrchestrator).
[[nodiscard]] inline LazyRunResult run_lazy_static_then_dynamic(
        ExperimentTree& tree, BuildSelection const& sel,
        CompileFn compile, SourceGenFn gen, FreeRamFn ram, LazyRunConfig const& cfg) {

    LazyRunResult result;
    StaticBinaryView const view = tree.static_binary_view();
    std::vector<DynamicDim> const dyn_dims = tree.dynamic_filter();  // der dynamische Sub-Filterbaum (LAZY-Quelle)

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
    bcfg.per_binary_subdirs      = cfg.per_binary_subdirs;   // (E): je Tier-Binary ein eigener Unterordner

    BuildOrchestrator orch{bcfg, std::move(compile), std::move(gen), std::move(ram)};
    std::vector<BuildResult> const builds =
        orch.provision_all(view, std::span<const std::size_t>{indices}, &result.build_stats);

    result.built              = result.build_stats.succeeded;
    result.built_new          = result.build_stats.built;
    result.built_skip         = result.build_stats.skipped;
    result.min_free_ram_bytes = result.build_stats.min_free_ram_bytes;

    RuntimeVariableLoop const loop{cfg.env_limits};

    // #156-De-Risk (2026-06-20): die EINE PMC-Quelle für den GANZEN Treiber-Lauf (Strategy+Factory; build-/OS-abhängig:
    // Windows-Intel-PCM unter COMDARE_ENABLE_PMC, sonst NullPmcSource → available=false). EINMAL erzeugt — NICHT je Op,
    // NICHT je Binary — und per Referenz in den WIDE-Mess-Pfad (run_observable_perm/run_workload_perm) gereicht; dort
    // klammert begin()/end() nur den getimten Batch. Schließt die #156-Naht: die WIDE-CSV trägt jetzt reale PMC-Counter
    // (lokal 0/available=0 mit NullPmcSource; Montag Linux+PMC=ON real). Identisches Muster wie f15_compare/main.cpp:252.
    std::unique_ptr<IPmcSource> pmc = make_pmc_source();

    // Mess-RESUME (#139): EIN Config-Stempel je Lauf (BuildVersion + Skala + volle dyn-Dimensions-Signatur).
    std::string const resume_stamp_prefix = lazy_resume_stamp_prefix(cfg, dyn_dims);

    // ── Je erfolgreich bereitgestellte DLL: laden + die zwei Lazy-Sub-Iteratoren fahren ──
    for (BuildResult const& b : builds) {
        // ════════════════════════════════════════════════════════════════════════════════════════════════
        // Mess-RESUME (#139 + Audit K8): Resume-Check VOR dem b.ok()-Gate. Ist die per-Binary result.csv für DIESE
        // Konfiguration vollständig+aktuell (Stamp+Header+Zeilenzahl), wird die Binary übersprungen und ihre Zeilen
        // unverändert in die globale CSV übernommen — AUCH wenn der aktuelle Build fehlschlägt (sonst stille CSV-Lücke
        // bei Exit 0, obwohl ein gültiges Ergebnis bereits vorliegt). b.output ist die INTENDIERTE Pfad-Angabe
        // (build_orchestrator:257, vor dem Build gesetzt) → der per-Binary-Unterordner steht auch bei Build-Fehler fest;
        // lazy_try_resume_binary prüft exists() → fehlender Pfad scheitert sauber. Stale (Stamp-Mismatch, z.B. andere
        // BuildVersion/n_ops/Workload/env/XML-Inhalt) → Neu-Messung mit Zwei-Phasen-Cache-Warmup (intrinsisch je Op).
        // ════════════════════════════════════════════════════════════════════════════════════════════════
        if (cfg.resume_completed_binaries && cfg.per_binary_subdirs) {
            std::string resumed_rows;
            if (lazy_try_resume_binary(b.output.parent_path(), resume_stamp_prefix, &resumed_rows)) {
                result.resumed_csv_rows += resumed_rows;
                ++result.resumed_binaries;
                continue;
            }
        }
        if (!b.ok()) continue;  // Build-Fehler UND nicht resumebar → kein Mess-Eintrag (ehrlicher Sparse-Kontrast)

        // ════════════════════════════════════════════════════════════════════════════════════════════════
        // (2) LADEN: DLL → IAnatomyBase* → die zwei ABI-Sub-Interfaces via dynamic_cast.
        //     IObservableTier  = Mess-Antrieb (nur bei COMDARE_MEASUREMENT_ON in der DLL vorhanden).
        //     IResourceControllableTier = Laufzeit-Steuerung (IMMER vorhanden, auch Messung-aus).
        // ════════════════════════════════════════════════════════════════════════════════════════════════
        anatomy_loader::AnatomyModuleHandle handle;
        int const st = anatomy_loader::AnatomyModuleLoader::load(b.output, handle);
        if (st != anatomy_loader::status_ok) { ++result.load_failed; continue; }

        anatomy::IAnatomyBase* base = handle.anatomy();
        auto* obs  = (base != nullptr) ? dynamic_cast<anatomy::IObservableTier*>(base) : nullptr;
        auto* ctrl = (base != nullptr) ? dynamic_cast<anatomy::IResourceControllableTier*>(base) : nullptr;
        // Achse 2 (INC-3): Sub-Interfaces für den Interpreter — IRollbackableTier (Zwei-Phasen-Cache-Warmup,
        // PFLICHT für Mess-Gültigkeit) + IScannableTier (YCSB-E Range-Scan). Alte DLLs → nullptr (Skip/Fallback).
        auto* rbk  = (base != nullptr) ? dynamic_cast<anatomy::IRollbackableTier*>(base) : nullptr;
        auto* scn  = (base != nullptr) ? dynamic_cast<anatomy::IScannableTier*>(base) : nullptr;
        if (obs == nullptr) { ++result.load_failed; continue; }  // keine Mess-Ebene (kein COMDARE_MEASUREMENT_ON-Build)
        ++result.loaded;

        std::string const binary_id = b.binary_id;
        // (E): der per-Binary-Ordner (= Parent der DLL bei per_binary_subdirs). Für die per-Binary-Ergebnis-CSV.
        std::filesystem::path const bin_dir = b.output.parent_path();
        std::string per_binary_csv;   // (E): akkumulierte CSV-Zeilen DIESER Binary (geschrieben am Binary-Ende)
        std::size_t per_binary_rows     = 0;   // #139: geschriebene Zeilen DIESER Binary (für den Stamp)
        std::size_t per_binary_settings = 0;   // #139: besuchte dyn-Settings DIESER Binary (Vollständigkeits-Gate)
        bool per_binary_all_valid       = true; // GOAL-M1.4: Stamp nur wenn JEDE Zeile two_phase_valid (Audit)

        // ════════════════════════════════════════════════════════════════════════════════════════════════
        // (3) GEFILTERT-DYNAMISCH-ITERATOR: LAZY über die virtuelle Kartesik des dynamischen Sub-Filterbaums
        //     (tree.dynamic_filter()) auf der GELADENEN Binary (RuntimeVariableLoop — KEIN Neu-Bauen/Neu-Laden).
        //     Je Setting wendet der Loop die Resource-Control an (tier_apply_resource_control, clamp gegen
        //     caps∩env); im Visitor messen wir UNTER dem Setting + ingesten die Zeile.
        // ════════════════════════════════════════════════════════════════════════════════════════════════
        auto measure_under_setting = [&](RuntimeSetting const& s) {
            ++result.dynamic_settings_total;
            ++per_binary_settings;   // #139: jede besuchte Einstellung zählt (auch wenn der Ingest scheitert)

            // Messen UNTER dem angewandten Setting (Antrieb + Observer ziehen). run_observable_perm RESETET den
            // Tier (A), misst die Gesamt-Wall-Clock (B) und liefert die result_ingest-Zeile als Observer-DELTA.
            // Die ID prefixen wir mit dem Setting → eindeutig je (Binary × Setting × Rep, da rep eine dyn-Dim ist).
            std::string const setting_id =
                s.setting_label.empty() ? binary_id : (binary_id + "#" + s.setting_label);
            // KONSOLIDIERUNG (I1): EINE tier_observe liefert Observer-Stats + Pfad-B-seg_ns in EINEM POD.
            // Achse 2 (INC-3): ist die Workload-Dim aktiv (workload_id im Label), treibt der bereits implementierte
            // Interpreter (run_workload_perm) EIN Lastprofil mit Pflicht-Zwei-Phasen-Cache-Warmup über die
            // map-Interfaces; sonst der alte fixe Workload (run_observable_perm, rückwärtskompatibel).
            std::string const workload_id = lazy_extract_workload_id(s.setting_label);
            PermResult const pr = workload_id.empty()
                ? run_observable_perm(*obs, setting_id, cfg.n_ops, pmc.get())
                : run_workload_perm(*obs, rbk, scn, setting_id, workload_id, cfg.n_ops,
                                    cfg.workload_seed, cfg.workload_records, &cfg.workload_configs, pmc.get());

            if (ingest_result_line(tree, pr.line)) {
                ++result.measured;
                // CSV-Zeile aus dem ge-ingesteten Baum-Knoten ziehen (Round-Trip-konsistent: identischer POD).
                NodeValue const nv = tree.node_value(setting_id);
                LazyMeasuredRow row;
                row.binary_id          = binary_id;
                row.setting_label      = s.setting_label;
                row.setting_id         = setting_id;
                row.observer           = nv.observer;
                row.applied_axis_count = s.applied_axis_count;
                row.total_ns           = pr.total_ns;       // (B)
                row.n_ops              = pr.n_ops;
                row.timed_ops          = pr.timed_ops;      // GOAL-M1.1: korrekte ns_per_op-Basis
                row.op_lat             = pr.op_lat;         // GOAL-L1: per-Interface-Funktions-Latenzen
                row.unified            = pr.unified;         // KONSOLIDIERUNG (I1): maßgebliche CSV-Quelle (Stats + Pfad-B-seg_ns)
                row.unified_real       = pr.unified_real;
                row.profile_name       = pr.profile_name;     // Achse 2: Lastprofil-Name (leer = alter fixer Workload)
                row.two_phase_valid    = pr.two_phase_valid;  // Achse 2: Mess-Gültigkeit (Zwei-Phasen-Cache-Warmup exakt)
                row.pmc                = pr.pmc;               // #156-De-Risk: die HW-PMC-Counter DIESER Messung → CSV-Endspalten
                // M3v2-SELEKTION (Task #156): die 5 Lauf-/Selektions-Tags je Zeile (aus der Config durchgereicht).
                row.series             = cfg.row_series;
                row.pruefling_type     = cfg.row_pruefling_type;  // #171: full/abstract/- (aus dem SOTA-/Pruefling-Pass)
                row.sweep_axis         = cfg.row_sweep_axis;
                row.working_set_n      = cfg.workload_records;   // N = befüllte Sätze (= Working-Set-Achse, P-MD7)
                row.platform           = cfg.row_platform;
                row.build_version      = cfg.row_build_version;
                // (E): die per-Binary-Ergebnis-CSV mit-akkumulieren (gleiches Schema wie die globale CSV).
                per_binary_all_valid = per_binary_all_valid && row.two_phase_valid;   // GOAL-M1.4 Gültigkeits-Gate
                if (cfg.per_binary_subdirs) { per_binary_csv += format_csv_row(row); ++per_binary_rows; }
                result.csv_rows.push_back(std::move(row));
            }
        };

        if (ctrl != nullptr && !dyn_dims.empty()) {
            // Echte dynamische Variation: kartesisch über die dyn. Dimensionen auf der geladenen Binary.
            loop.run(*ctrl, dyn_dims, measure_under_setting);
        } else {
            // Keine dyn. Dimensionen (oder keine Steuer-Ebene): EIN Mess-Punkt = die Binary „as built".
            // (RuntimeVariableLoop mit leeren dims liefert ebenfalls genau 1 Setting; dieser Zweig deckt den
            //  Fall ohne IResourceControllableTier ab — z.B. eine Nicht-Mess-/Alt-DLL, hier aber obs!=null.)
            RuntimeSetting s{};  // leeres Label → setting_id == binary_id
            measure_under_setting(s);
        }

        // (E): per-Binary-Ergebnis-CSV in den per-Binary-Ordner schreiben (Header + die Zeilen DIESER Binary).
        if (cfg.per_binary_subdirs && !per_binary_csv.empty() && !bin_dir.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(bin_dir, ec);   // existiert i.d.R. schon (Build legte ihn an)
            bool csv_write_ok = false;   // GOAL-M1.4 (Audit M7): Stamp nur nach VERIFIZIERTEM Write
            {
                std::ofstream pf{bin_dir / "result.csv", std::ios::trunc};
                if (pf) { pf << lazy_csv_header() << per_binary_csv; pf.flush(); csv_write_ok = pf.good(); }
            }
            // Mess-RESUME (#139 + GOAL-M1.4): den Config-Stempel NUR schreiben, wenn (a) der CSV-Write
            // stream-verifiziert gelang, (b) JEDE besuchte Einstellung eine Zeile lieferte (Vollständigkeit)
            // und (c) JEDE Zeile two_phase_valid ist (Gültigkeit — ungültige Messungen nie als „fertig"
            // einfrieren, Audit). Sonst Stempel entfernen → der nächste Lauf misst die Binary komplett neu.
            if (csv_write_ok && per_binary_rows == per_binary_settings && per_binary_rows > 0
                && per_binary_all_valid) {
                std::ofstream sf{bin_dir / "result.csv.stamp", std::ios::trunc};
                if (sf) { sf << resume_stamp_prefix << "|rows=" << per_binary_rows << "\n"; }
            } else {
                std::filesystem::remove(bin_dir / "result.csv.stamp", ec);   // stale Stempel nie stehen lassen
            }
        }
        // handle: RAII entlädt die DLL am Schleifenende (Pointer zuerst, dann FreeLibrary).
    }

    return result;
}

}  // namespace comdare::cache_engine::builder::experiment
