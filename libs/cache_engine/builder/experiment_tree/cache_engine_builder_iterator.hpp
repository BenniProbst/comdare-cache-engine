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
#include "coverage_selection.hpp"         // BuildSelection
#include "runtime_variable_loop.hpp"      // RuntimeVariableLoop / RuntimeSetting (gefiltert-dynamisch)
#include "perm_runner.hpp"                // run_observable_perm / format_perm_result
#include "result_ingest.hpp"             // ingest_result_line
#include "../build_orchestrator/build_orchestrator.hpp"             // BuildOrchestrator / BuildConfig / *Fn
#include "../anatomy_module_loader/anatomy_module_loader.hpp"       // AnatomyModuleLoader / AnatomyModuleHandle
#include "../../anatomy/observable_tier.hpp"                         // IObservableTier
#include "../../anatomy/resource_controllable_tier.hpp"             // IResourceControllableTier

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
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
};

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
        if (obs == nullptr) { ++result.load_failed; continue; }  // keine Mess-Ebene (kein COMDARE_MEASUREMENT_ON-Build)
        ++result.loaded;

        std::string const binary_id = b.binary_id;

        // ════════════════════════════════════════════════════════════════════════════════════════════════
        // (3) GEFILTERT-DYNAMISCH-ITERATOR: LAZY über die virtuelle Kartesik des dynamischen Sub-Filterbaums
        //     (tree.dynamic_filter()) auf der GELADENEN Binary (RuntimeVariableLoop — KEIN Neu-Bauen/Neu-Laden).
        //     Je Setting wendet der Loop die Resource-Control an (tier_apply_resource_control, clamp gegen
        //     caps∩env); im Visitor messen wir UNTER dem Setting + ingesten die Zeile.
        // ════════════════════════════════════════════════════════════════════════════════════════════════
        auto measure_under_setting = [&](RuntimeSetting const& s) {
            ++result.dynamic_settings_total;

            // Messen UNTER dem angewandten Setting (Antrieb + Observer ziehen). run_observable_perm formatiert
            // eine result_ingest-Zeile; die ID prefixen wir mit dem Setting → eindeutig je (Binary × Setting).
            std::string const setting_id =
                s.setting_label.empty() ? binary_id : (binary_id + "#" + s.setting_label);
            std::string const line = run_observable_perm(*obs, setting_id, cfg.n_ops);

            if (ingest_result_line(tree, line)) {
                ++result.measured;
                // CSV-Zeile aus dem ge-ingesteten Baum-Knoten ziehen (Round-Trip-konsistent: identischer POD).
                NodeValue const nv = tree.node_value(setting_id);
                LazyMeasuredRow row;
                row.binary_id          = binary_id;
                row.setting_label      = s.setting_label;
                row.setting_id         = setting_id;
                row.observer           = nv.observer;
                row.applied_axis_count = s.applied_axis_count;
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
        // handle: RAII entlädt die DLL am Schleifenende (Pointer zuerst, dann FreeLibrary).
    }

    return result;
}

}  // namespace comdare::cache_engine::builder::experiment
