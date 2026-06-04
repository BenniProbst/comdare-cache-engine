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

#include <cstddef>
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
    // (build_pilot_levels(..., n_repeats)); hier dokumentiert/durchgereicht. Default 3 (0 → 1 normalisiert).
    std::uint32_t         n_repeats = 3;
    // (C-2): per-Segment-Workload-Parameter (run_workload_segmented). seg_batches=0 → kein Segment-Timing (n/a).
    std::uint64_t         seg_ops_per_batch = 4000;  // Operationen je Batch im 19-Segment-Workload (X)
    std::uint64_t         seg_batches       = 32;    // gemessene Batches (Warmup verworfen); 0 = Segment-Timing aus
    std::uint64_t         seg_seed          = 0xB15u; // deterministischer Seed
    // Laufzeit-Obergrenze (System-Limits) für die dyn-Variation (RuntimeVariableLoop clamp gegen caps∩env).
    anatomy::ComdareResourceControlV1 env_limits{};
};

// ── Eine gemessene CSV-Zeile (Binary × dyn-Setting) ───────────────────────────
struct LazyMeasuredRow {
    std::string          binary_id;        // statische Rekombination (= die Tier-Binary)
    std::string          setting_label;    // dyn. Belegung "axis.var=value/…" (leer = keine dyn-Dimensionen)
    std::string          setting_id;       // binary_id (+ "#" + setting_label) = eindeutiger Baum-Key
    NodeObserverSnapshot  observer{};       // die real gezogenen Observer-Werte (>0 bei echter Messung)
    std::uint64_t        applied_axis_count = 0;  // wie viele Achsen die Steuerung real annahmen
    // (B): Host-Gesamt-Wall-Clock DIESER Messung (insert+lookup) + Eingabe-n_ops → ns_per_op = total_ns/(2*n_ops).
    std::int64_t         total_ns = 0;
    std::uint64_t        n_ops    = 0;
    // (X): die 19 ECHT gemessenen per-Segment-ns ALLER SearchAlgorithm-Achsen (T0..T18, seg_ns[19]).
    // seg_real=false → das Modul exponiert IMeasurableWorkloadV3 nicht → ehrlich n/a in der CSV (NICHT 0).
    anatomy::ComdareSegmentLatencyV2 seg{};
    bool                 seg_real = false;
    // Phase A (2026-06-04): der generische Per-Achsen-V3-Observer-Snapshot (axis_stats[19][8]) — die 10 Phase-A-
    // Achsen tragen echte statistics()-Werte (>0), die 9 Phase-B-Achsen 0. v3_real=false → das Modul exponiert
    // IObservableTierV3 nicht (alte DLL) → die stat_*-Spalten = ehrlich „n/a" (NICHT 0).
    anatomy::ComdareTierObserverSnapshotV3 v3{};
    bool                 v3_real = false;
    // KONSOLIDIERUNG (I-B, 2026-06-04): der EINE konsolidierte Observer-POD (axis_stats[19][8] + seg_ns[19] + Meta).
    // Maßgebliche CSV-Quelle: stat_*-Spalten aus unified.axis_stats, seg_*-Spalten aus unified.seg_ns (Pfad B, reale
    // Komposition — User-Entscheid 2026-06-04, ersetzt den Pfad-A-Segment-Timer). V3/seg entfallen in I-C.
    anatomy::ComdareTierObserverSnapshot unified{};
    bool                 unified_real = false;
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
// stat_*-Spalten sind „n/a", wenn die DLL kein IObservableTierV3 trägt (v3_real=false) — ehrlich n/a, NICHT 0.
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
    for (std::size_t i = 0; i < kCompositionAxisNames.size(); ++i) {  // 19 seg_<axis>_ns-Spalten, single-source
        h += "seg_"; h += kCompositionAxisNames[i]; h += "_ns;";
    }
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
    h += "v3_filled_axes\n";   // Diagnose: wie viele der 19 Achsen jetzt befüllt sind (Phase B Abschluss: alle 19)
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
    // (C-1) ns_per_op abgeleitet: total_ns / (2*n_ops) — insert+lookup; n_ops==0 → 0.
    double const ns_per_op = (row.n_ops != 0)
        ? (static_cast<double>(row.total_ns) / (2.0 * static_cast<double>(row.n_ops))) : 0.0;
    { char buf[48]; int const n = std::snprintf(buf, sizeof(buf), "%.3f", ns_per_op);
      out.append(buf, (n > 0) ? static_cast<std::size_t>(n) : 0); }
    out += ';';
    // (X) die 19 per-Segment-ns (T0..T18) — echt wenn seg_real, sonst ehrlich n/a (NICHT 0). Geschleift über
    // seg_ns[19] in derselben Reihenfolge wie die Header-Spalten (kCompositionAxisNames) — single-source, keine Drift.
    // KONSOLIDIERUNG (I-B): seg_<achse>_ns aus dem EINEN konsolidierten POD = Pfad-B-Timing (reale Komposition,
    // User-Entscheid 2026-06-04). unified_real=false (alte DLL ohne die konsolidierte tier_observe) → ehrlich n/a.
    auto seg_field = [&](std::int64_t v) { out += (row.unified_real ? std::to_string(v) : std::string{"n/a"}); out += ';'; };
    for (int i = 0; i < 19; ++i) seg_field(row.unified.seg_ns[i]);
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
    // v3_real (Modul trägt IObservableTierV3), sonst ehrlich „n/a" (NICHT 0). Reihenfolge IDENTISCH zum Header.
    for (std::size_t t = 0; t < anatomy::kV3AxisCount; ++t) {
        for (std::size_t f = 0; f < anatomy::kV3FieldCount; ++f) {
            if (anatomy::kV3AxisSchema[t].names[f] == nullptr) continue;   // ungenutzt / Phase B → keine Spalte
            out += (row.unified_real ? std::to_string(row.unified.axis_stats[t][f]) : std::string{"n/a"});
            out += ';';
        }
    }
    out += (row.unified_real ? std::to_string(row.unified.filled_axis_count) : std::string{"n/a"});   // v3_filled_axes (aus konsolidiertem POD)
    out += '\n';
    return out;
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
};

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

    // ── Je erfolgreich bereitgestellte DLL: laden + die zwei Lazy-Sub-Iteratoren fahren ──
    for (BuildResult const& b : builds) {
        if (!b.ok()) continue;  // Build-Fehler → kein Mess-Eintrag (ehrlicher Sparse-Kontrast)

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
        // (X): das 19-Segment-Timer-Sub-Interface (alte/ohne-V3 DLL → nullptr → ehrlich n/a, kein Crash).
        auto* segw = (base != nullptr) ? dynamic_cast<anatomy::IMeasurableWorkloadV3*>(base) : nullptr;
        // Phase A: das generische Per-Achsen-V3-Observer-Sub-Interface (alte DLL ohne V3 → nullptr → stat_*=n/a).
        auto* obs3 = (base != nullptr) ? dynamic_cast<anatomy::IObservableTierV3*>(base) : nullptr;
        if (obs == nullptr) { ++result.load_failed; continue; }  // keine Mess-Ebene (kein COMDARE_MEASUREMENT_ON-Build)
        ++result.loaded;

        std::string const binary_id = b.binary_id;
        // (E): der per-Binary-Ordner (= Parent der DLL bei per_binary_subdirs). Für die per-Binary-Ergebnis-CSV.
        std::filesystem::path const bin_dir = b.output.parent_path();
        std::string per_binary_csv;   // (E): akkumulierte CSV-Zeilen DIESER Binary (geschrieben am Binary-Ende)

        // ════════════════════════════════════════════════════════════════════════════════════════════════
        // (3) GEFILTERT-DYNAMISCH-ITERATOR: LAZY über die virtuelle Kartesik des dynamischen Sub-Filterbaums
        //     (tree.dynamic_filter()) auf der GELADENEN Binary (RuntimeVariableLoop — KEIN Neu-Bauen/Neu-Laden).
        //     Je Setting wendet der Loop die Resource-Control an (tier_apply_resource_control, clamp gegen
        //     caps∩env); im Visitor messen wir UNTER dem Setting + ingesten die Zeile.
        // ════════════════════════════════════════════════════════════════════════════════════════════════
        auto measure_under_setting = [&](RuntimeSetting const& s) {
            ++result.dynamic_settings_total;

            // Messen UNTER dem angewandten Setting (Antrieb + Observer ziehen). run_observable_perm RESETET den
            // Tier (A), misst die Gesamt-Wall-Clock (B) und liefert die result_ingest-Zeile als Observer-DELTA.
            // Die ID prefixen wir mit dem Setting → eindeutig je (Binary × Setting × Rep, da rep eine dyn-Dim ist).
            std::string const setting_id =
                s.setting_label.empty() ? binary_id : (binary_id + "#" + s.setting_label);
            // Phase A: obs3 (IObservableTierV3*, ggf. nullptr) wird durchgereicht → run_observable_perm zieht
            // nach der Mess-Last den generischen Per-Achsen-V3-Snapshot (oder lässt v3_real=false → CSV n/a).
            PermResult const pr = run_observable_perm(*obs, setting_id, cfg.n_ops, obs3);

            // (X): echter per-Segment-Timer (ALLE 19 instrumentierten Achsen) — n/a, wenn das Modul kein
            // IMeasurableWorkloadV3 exponiert (segw==nullptr) ODER seg_batches==0 (Segment-Timing aus).
            anatomy::ComdareSegmentLatencyV2 seg{};
            std::uint64_t seg_batches_done = 0;
            if (segw != nullptr && cfg.seg_batches != 0) {
                seg_batches_done = drive_segment_latencies(segw, cfg.seg_ops_per_batch, cfg.seg_batches,
                                                           cfg.seg_seed, seg);
            }

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
                row.seg                = seg;                // (C-2)
                row.seg_real           = (seg_batches_done > 0);
                row.unified            = pr.unified;         // KONSOLIDIERUNG (I-B): maßgebliche CSV-Quelle (Stats + Pfad-B-seg_ns)
                row.unified_real       = pr.unified_real;
                row.v3                 = pr.v3;              // Phase A: generischer Per-Achsen-V3-Snapshot
                row.v3_real            = pr.v3_real;
                // (E): die per-Binary-Ergebnis-CSV mit-akkumulieren (gleiches Schema wie die globale CSV).
                if (cfg.per_binary_subdirs) per_binary_csv += format_csv_row(row);
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
            std::ofstream pf{bin_dir / "result.csv", std::ios::trunc};
            if (pf) { pf << lazy_csv_header() << per_binary_csv; }
        }
        // handle: RAII entlädt die DLL am Schleifenende (Pointer zuerst, dann FreeLibrary).
    }

    return result;
}

}  // namespace comdare::cache_engine::builder::experiment
