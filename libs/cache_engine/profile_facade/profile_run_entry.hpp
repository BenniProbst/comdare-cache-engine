#pragma once
// STRANG A KORRIGIERT — Increment 6 / S7c (2026-06-19, profil-getrieben). profile_run_entry: die EINE
// deklarative CEB-Eintritts-API run_profile(...) — der "duenne profile_runner-Einstieg, den der
// messung_driver (Diplomarbeit) triggert" (Doc 10 §2.2 "messung_driver ruft CacheEngineBuilder").
//
// Plan: docs/sessions/20260618-STRANG-A-KORRIGIERT-PROFIL-GETRIEBEN-PLAN.md (S7). Diese Funktion vereinigt
// die drei S7-Bausteine zu EINEM Aufruf:
//   • S7a SourceGen-Merge  : source_catalog (Basis-320, "search_algo=.../...") UNION sota_catalog
//                            ("sota_tier=sota::S::name") → make_union_source_gen (disjunkte Namensraeume).
//   • S7b Multi-Pass       : aus EINEM Profil fuehrt run_profile MEHRERE Selektions-Paesse:
//                            (1) BASIS-Pass  = permute_axes → build_profile_basis_levels → 320 (bzw. cap),
//                            (1b) je deklariertem <axis_sweep> EIN Achsen-Sweep-Pass (#26/GO-5 Multi-Sweep,
//                                profile_sweep_passes; explizites args.sweep_axis behaelt Einzel-Pass-Vorrang),
//                            (2) je <sota_series>-Eintrag = EIN SOTA-Lebewesen-Pass (einwertiger "sota_tier"-
//                                Baum), getaggt mit series A/B/C. Alle Zeilen → EINE CSV (Header genau EINMAL).
//   • S7c Eintritts-API    : run_profile(RunProfileArgs&) — run_lazy_150.main reduziert sich auf Pfad+Output;
//     (run_lazy_150 geloescht 2026-07-11; Host/Emitter heute Code/02_messung_driver, E4-XML)
//                            die WHAT-Konfiguration kommt komplett aus dem Profil.
//
// Resume/binary_id-Stabilitaet bleibt (jeder Pass ruft run_lazy_static_then_dynamic → .version-Sidecar +
// per-Binary-result.csv-Stamp). Lazy-Compile (1 DLL = 1 TU) bleibt: die Vereinigung waehlt nur die Quelle.
//
// ⚠️ Katalog-/Umbrella-schwer (source_catalog.hpp zieht den all_axes_umbrella) → gehoert in die HARNESS-/Test-
//    .cpp (run_lazy_150.cpp / test_*), NICHT in den engine-agnostischen Treiber-Header. C++23, header-only.
//    (run_lazy_150.cpp geloescht 2026-07-11; Host/Emitter heute Code/02_messung_driver, E4-XML)

#include "generated_source_catalog.hpp" // generated_make_catalog_source_gen (Basis-320-Quelle)
#include "h2_score_akte.hpp"            // GO-5 Fork 7: parse_h2_score_akte / h2_score_for (CSV-Endspalte)
#include "source_catalog.hpp"           // axis_sweep_source_map / axis_sweep_levels (Sweep-Quellen)
#include "sota_catalog.hpp"             // build_sota_passes / build_sota_view_source_map / kSotaTierAxis (S6/S7b)
#include "profile_runner.hpp" // load_thesis_profile / build_profile_basis_levels / profile_select / make_union_source_gen

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // run_lazy_static_then_dynamic / lazy_csv_header / LazyRunConfig
#include <builder/experiment_tree/coverage_selection.hpp> // select_explicit
#include <builder/build_orchestrator/system_ram.hpp>      // make_system_free_ram_fn

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace wd = ::comdare::cache_engine::builder::workload_driver;

// ── Eingabe der EINEN CEB-Eintritts-API. ALLES, was NICHT aus dem Profil kommt (Pfade/Toolchain/Output) ──
//    Die WHAT-Konfiguration (Lebewesen/Achsen/Sweeps/SOTA/Working-Set/run_options) liest run_profile selbst
//    aus dem Profil (tp). cap/resume/platform/build_version werden aus <run_options> vorbelegt; argv/env darf
//    weiterhin uebersteuern (der Treiber-Host setzt die Felder hier vor dem Aufruf).
struct RunProfileArgs {
    std::filesystem::path    profile_path;     // das comdare_thesis_profile (m3v2_study.profile.xml)
    std::filesystem::path    out_csv;          // Ziel-CSV (Header genau EINMAL, alle Paesse darunter)
    std::filesystem::path    src_dir;          // perm_<id>.cpp-Ausgabe (per-Binary-Subdir-Basis)
    std::filesystem::path    dll_dir;          // perm_<id>.dll-Ausgabe (per-Binary-Subdir-Basis)
    ex::CompileFn            compile;          // injizierter Compiler-Aufruf (cl @rsp) — wie BuildOrchestrator
    ex::AlgoSigFn            algo_sig;         // Bauplan §7: spec.axes → algo_sig (perm.algos); leer = Organ-Gate aus
    ex::CachePushFn          cache_push;       // Storage #51: perm.dll(+.version) -> Objekt-Store (B); leer = No-Op
    ex::MeasurementSinkFn    measurement_sink; // Storage #51: Mess-Datei -> NFS additiv (C); leer = No-Op
    std::vector<std::string> compile_includes; // ungenutzt hier (der Host backt die Includes in compile) — Doku
    std::uint64_t            n_ops               = 10000;  // Mess-Workload je dyn-Setting
    std::size_t              max_binaries        = 0;      // 0 ⇒ run_options.cap; beide 0 ⇒ KEIN Cap
    std::string              build_version       = "m3v2"; // Resume-Marke (.version-Sidecar)
    std::uint32_t            n_repeats           = 3;      // Wiederholungen je (Binary×Setting)
    std::size_t              cores_per_build     = 4;      // KF-16b Default
    double                   min_free_gb         = 0.0;    // RAM-Admission (0 = aus)
    bool                     resume_override_set = false;  // true ⇒ resume kommt aus `resume`, nicht aus <run_options>
    bool                     resume              = true;   // Mess-Resume (#139)
    std::string              sweep_axis;                 // leer = Basis-Selektion; sonst ein deklarierter <axis_sweep>
    std::string              platform_override;          // leer ⇒ <run_options>.platform; sonst Override (CSV-Tag)
    std::string              build_version_tag_override; // leer ⇒ <run_options>.build_version; sonst Override (CSV-Tag)
    bool                     run_sota_series = true;     // S7b: die <sota_series>-Paesse mitfahren (false = nur Basis)
    // Working-Set-Sweep: Default = der Profil-<working_set_sweep>. Ist `working_set_override` gesetzt (>0), ersetzt
    // er den Profil-Sweep durch EINEN einzigen N-Wert (rueckwaerts-kompatibel zur alten PS-foreach +
    // COMDARE_WORKLOAD_RECORDS, wo das Harness die aeussere N-Schleife selbst faehrt). 0 = Profil-Sweep nutzen.
    std::uint64_t working_set_override = 0;
    // Achse 2 (#135): XML-Lastprofil-Registry (id → WorkloadConfig). Vom Host via discover_load_profiles gesetzt.
    std::map<std::string, wd::WorkloadConfig> workload_registry;
    std::vector<std::string>                  workload_values; // nur fuers Log (Achse-2-Werte)
};

// ── Ergebnis (rein zaehlend; die CSV ist die maßgebliche Mess-Ausgabe). ──
struct RunProfileResult {
    int           exit_code        = 1;
    std::size_t   basis_rows       = 0; // CSV-Zeilen aus dem Basis-Pass (frisch+resumiert)
    std::size_t   sota_rows        = 0; // CSV-Zeilen aus den SOTA-Reihen-Paessen (frisch+resumiert)
    std::size_t   basis_binary_ids = 0; // distinkte Basis-binary_ids, die in DIESEM Lauf selektiert wurden
    std::size_t   sota_binary_ids  = 0; // distinkte SOTA-Reihen-binary_ids, die gebaut/gemessen wurden
    std::uint64_t any_measured     = 0;
    std::uint64_t any_resumed      = 0;
};

// Interne Helfer: zaehlt '\n' in einem CSV-Block (resumierte Zeilen liegen als String vor).
[[nodiscard]] inline std::size_t count_lines(std::string const& s) {
    std::size_t n = 0;
    for (char c : s)
        if (c == '\n') ++n;
    return n;
}

/// run_profile — DIE EINE deklarative CEB-Eintritts-API (S7c). Faehrt aus EINEM Profil BEIDE Subsets:
///   (1) den BASIS-Pass (permute_axes → source_catalog) und
///   (2) je <sota_series> einen SOTA-Reihen-Pass (sota_catalog), getaggt A/B/C,
/// ueber EINE vereinigte SourceGenFn (make_union_source_gen) in EINE CSV. Die per-Pass-Iteration ueber
/// <working_set_sweep> bleibt erhalten (aeussere N-Schleife). Resume/binary_id-Stabilitaet wie der heutige
/// Treiber (run_lazy_static_then_dynamic je Pass; #139-Stamp pro Pass).
[[nodiscard]] inline RunProfileResult run_profile(RunProfileArgs const& a) {
    RunProfileResult res;

    // ── (0) Profil EINMAL parsen (die EINZIGE WHAT-Quelle). ──
    std::optional<cx::ThesisProfile> const tp_opt = load_thesis_profile(a.profile_path);
    if (!tp_opt) {
        std::cerr << "run_profile: Profil '" << a.profile_path.string()
                  << "' nicht lesbar (parse_thesis_profile=nullopt) — Abbruch.\n";
        res.exit_code = 5;
        return res;
    }
    auto const&             tp        = *tp_opt;
    std::string const       mode_name = tp.modes.empty() ? std::string{"m3v2_base"} : tp.modes.front().name;
    ProfileRunOptions const ro        = profile_run_options(tp);

    // ── (1) Der BASIS-Baum (build_profile_basis_levels = build_axis_levels OHNE tier-Ebene). ──
    auto                       factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree         basis_tree{factory};
    std::vector<ex::AxisLevel> basis_levels = build_profile_basis_levels(tp, mode_name, /*with_dynamic=*/true);
    // ── (1b) ACHSE 2 (Lastprofil) als DYNAMISCHE Ebene injizieren (STRANG-A-Wiring-Luecke, 2026-06-19).
    //    build_axis_levels emittiert die runtime_dynamic-DynDims (concurrency.thread_count / prefetch.hw_prefetcher /
    //    repetition.repetition_index), aber NICHT die Lastprofil-Achse (die wird separat ueber discover_load_profiles
    //    entdeckt → a.workload_values). Ohne diese Ebene fehlt im setting_label das Segment "workload.workload_id=X";
    //    der Iterator faellt dann auf run_observable_perm (fixer Workload, KEIN Zwei-Phasen-Cache-Warmup) zurueck →
    //    JEDE Zeile two_phase_valid=0 (Mess-UNGUELTIG). Mit der Ebene laeuft run_workload_perm mit Pflicht-Rollback
    //    → two_phase_valid=1. Gleiche Konvention wie die uebrigen DynDims (block_id.variable=value): block_id
    //    "workload", variable "workload_id" (lazy_extract_workload_id sucht genau "workload_id="). is_static=false ⇒
    //    veraendert die binary_id NICHT (Round-Trip-Gate unberuehrt).
    if (!a.workload_values.empty())
        basis_levels.push_back(
            ex::AxisLevel{"workload", a.workload_values, /*is_static=*/false, "workload_id", "workload"});
    basis_tree.build(basis_levels);
    ex::StaticBinaryView const basis_view = basis_tree.static_binary_view();

    // ── (2) cap/resume/Tags (Profil-Defaults; Override aus den Args). cap-Aufloesung = profile_effective_cap
    //    (Single-Source, profile_runner.hpp): cap="0"/fehlendes cap = KEIN Cap → alle Basis-Zellen (GO-4/GO-5-
    //    Nebenbefund 2026-07-12; vorher ergab eff_cap==0 eine LEERE Basis-Selektion — m3_golden_coverage
    //    (cap="0" = dokumentiert "KEIN kuenstliches Cap") bekam damit faelschlich keine Basis-320). ──
    bool const        resume = a.resume_override_set ? a.resume : ro.resume;
    std::size_t const N      = profile_effective_cap(ro.cap, a.max_binaries, basis_tree.binary_count());

    std::string tag_platform      = a.platform_override.empty()
                                        ? (ro.platform.empty() ? std::string{"win-x86_64"} : ro.platform)
                                        : a.platform_override;
    std::string tag_build_version = a.build_version_tag_override.empty()
                                        ? (ro.build_version.empty() ? std::string{"m3v2"} : ro.build_version)
                                        : a.build_version_tag_override;

    // ── (3) S7a + FF(#168): die EINE vereinigte SourceGenFn (Basis-320 ∪ Achsen-Sweeps ∪ SOTA-Reihen).
    //    Reihenfolge unkritisch: Basis-320 ("search_algo=…/migration_policy=migration_none/…"-Pfade) und die
    //    Achsen-Sweep-Map (gleicher 19-Achsen-Pfad-Namensraum, ABER andere Auspraegungen wie
    //    …/migration_policy=migration_hot_cold/…, die im Basis-320 NICHT vorkommen) sind ueberlappungsfrei (bis auf
    //    die Baseline-DLL + die Basis-Achsen-Sweep-ids, die identisch sind → idempotent; union_gen fragt die
    //    Basis-320 zuerst). SOTA liegt im disjunkten "sota_tier=…"-Raum.
    std::map<std::string, std::string> fused =
        make_all_axis_sweeps_source_map(); // alle 19 Achsen-Sweeps (#26/GO-5; Eintragszahl USE-Enable-abhaengig)
    for (auto& [k, v] : build_sota_view_source_map(tp)) fused.emplace(k, std::move(v)); // + SOTA-Reihen (disjunkt)
    ex::SourceGenFn const union_gen = make_union_source_gen(generated_make_catalog_source_gen(), std::move(fused));
    ex::FreeRamFn         ram       = ex::make_system_free_ram_fn();

    // ── (4) Working-Set-Sweep = die aeussere N-Liste (gilt fuer BEIDE Subsets identisch). Das Profil-
    //    <working_set_sweep> ist AUTORITATIV (XML steuert ALLES, #229/G3); working_set_override (env
    //    COMDARE_WORKLOAD_RECORDS, ehem. PS-foreach-Behelf) greift NUR als Fallback, wenn das Profil keinen
    //    Sweep setzt — sonst kollabierte der Behelfs-Override den mehrwertigen XML-Sweep still auf EIN N. ──
    std::vector<std::uint64_t> n_sweep = profile_working_set_sweep(tp); // Profil-<working_set_sweep> autoritativ
    if (n_sweep.empty() && a.working_set_override > 0)
        n_sweep.push_back(a.working_set_override); // Fallback nur ohne Profil-Sweep
    if (n_sweep.empty()) n_sweep.push_back(0);     // 0 ⇒ Iterator setzt records = n_ops

    std::cout << "RUN_PROFILE (CEB-Eintritt): " << a.profile_path.string() << "  id=" << tp.id << " mode=" << mode_name
              << "  basis_count=" << basis_tree.binary_count() << " (N=" << N << ")"
              << "  sota_series=" << tp.sota_series.size() << "  working_set_n=" << n_sweep.size() << "\n";
    // GO-5 Fork 1 (2026-07-12): die deklarierten <datasets>-Akten-Referenzen als Lauf-Provenienz-Kopf
    // (Single-Source = die test_data-Akten, Fork 2/R2). EHRLICH: der Loader-MESS-Konsum (load_or_generate_ycsb
    // im Workload-Pfad) ist lauf-gated — heute reisen die Referenzen in den Resume-Stamp (make_cfg) + dieses Log.
    if (!tp.datasets.empty()) {
        std::cout << "  [DATASETS] deklariert=" << tp.datasets.size()
                  << " (Akten=Single-Source; Loader-Mess-Konsum lauf-gated, Signatur geht in den Resume-Stamp)\n";
        for (auto const& d : tp.datasets)
            std::cout << "      dataset id=" << d.id << " akte_ref=" << d.akte_ref << " loader=" << d.loader << "\n";
    }

    // ── EINE CSV; Header GENAU EINMAL; darunter Basis-Pass + SOTA-Paesse (alle N). ──
    // M11 (G5-Audit w289llo0o): Stream-Fehlerpruefung. Liess der open() scheitern (Pfad nicht
    // schreibbar / Platte voll), waere die CSV stillschweigend leer geblieben → exit_code haette
    // faelschlich Erfolg gemeldet. Open-Erfolg jetzt hart geprueft; Schreib-/Flush-Fehler fliessen
    // unten (csv.good() nach flush) in den exit_code ein.
    std::ofstream csv{a.out_csv.string(), std::ios::trunc};
    if (!csv) {
        std::cout << "RUN_PROFILE FEHLER: CSV nicht oeffenbar → " << a.out_csv.string() << "\n";
        res.exit_code = 1;
        return res;
    }
    csv << ex::lazy_csv_header();

    // Gemeinsame Lauf-Config-Vorlage (je Pass kopiert + getaggt). 1 DLL = 1 TU bleibt.
    // #171 (2026-06-20): make_cfg traegt zusaetzlich pruefling_type (full/abstract/-). Basis/Sweep uebergeben
    // leer ("-"); die SOTA-Paesse uebergeben den aus merge abgeleiteten Typ (sota_catalog::derive_pruefling_type).
    // GO-5 Fork 6 (2026-07-12): zusaetzlich fairness_mode (common_denominator/native/-) — Basis/Sweep "-",
    // SOTA-Paesse den deklarierten <sota_series fairness=..>-Modus (SotaPass.fairness_mode).
    // GO-5 Fork 1 (2026-07-12): die <datasets>-Deklarations-Signatur des Profils geht lauf-weit in JEDE Pass-
    // Config (Resume-Stamp-Konsum; Ankunfts-Nachweis in der Spec — der Loader-Mess-Konsum ist lauf-gated).
    // GO-5 Fork 7 (2026-07-12): die TOOL-BERECHNETE H2-Score-Akte (sota_h2_scores.xml, Schwester-Datei der
    // sota/*.profile.xml — gleiche Co-Lokalisierungs-Ableitung wie load_profiles/ in run_profile_facade).
    // Fehlt die Akte, sind ALLE SOTA-Reihen honest "n/a" (kein Abbruch, kein 0-Phantom); Basis/Sweep = "-".
    std::string const                datasets_signature = profile_datasets_signature(tp);
    std::optional<H2ScoreAkte> const h2_akte =
        a.profile_path.empty()
            ? std::nullopt
            : parse_h2_score_akte(a.profile_path.parent_path().parent_path() / "sota" / "sota_h2_scores.xml");
    if (h2_akte.has_value())
        std::cout << "  [H2-AKTE] sota_h2_scores.xml geladen: " << h2_akte->entries.size()
                  << " Eintraege (tool=" << h2_akte->tool << " " << h2_akte->tool_version << ")\n";
    else
        std::cout << "  [H2-AKTE] keine sota_h2_scores.xml — h2_code_quality_score der SOTA-Reihen = n/a (honest)\n";
    auto make_cfg = [&](std::uint64_t ws_n, std::size_t cap_for_pass, std::string const& series,
                        std::string const& sweep_axis, std::string const& pruefling_type,
                        std::string const& fairness_mode, std::string const& h2_score) {
        ex::LazyRunConfig cfg;
        cfg.max_binaries = cap_for_pass;
        // G5: <run_options n_ops> ist autoritativ (XML steuert ALLES, #229); der Fassaden-/argv-Wert
        // greift nur als Fallback (n_ops=0 im Profil = ungesetzt).
        cfg.n_ops                     = (tp.run_options.n_ops > 0) ? tp.run_options.n_ops : a.n_ops;
        cfg.workload_records          = ws_n;
        cfg.workload_configs          = a.workload_registry;
        cfg.build_version             = a.build_version;
        cfg.row_series                = series.empty() ? std::string{"-"} : series;
        cfg.row_pruefling_type        = pruefling_type.empty() ? std::string{"-"} : pruefling_type;
        cfg.row_sweep_axis            = sweep_axis.empty() ? std::string{"-"} : sweep_axis;
        cfg.row_fairness_mode         = fairness_mode.empty() ? std::string{"-"} : fairness_mode; // GO-5 Fork 6
        cfg.row_h2_score              = h2_score.empty() ? std::string{"-"} : h2_score;           // GO-5 Fork 7
        cfg.profile_datasets          = datasets_signature; // GO-5 Fork 1: lauf-weite <datasets>-Signatur (Stamp)
        cfg.row_platform              = tag_platform;
        cfg.row_build_version         = tag_build_version;
        cfg.source_dir                = a.src_dir;
        cfg.output_dir                = a.dll_dir;
        cfg.cores_per_build           = a.cores_per_build;
        cfg.per_binary_subdirs        = true;
        cfg.resume_completed_binaries = resume;
        cfg.cache_push                = a.cache_push;       // Storage #51: bis zur per-Binary-Naht (No-Op-Default)
        cfg.measurement_sink          = a.measurement_sink; // Storage #51: result.csv -> NFS (No-Op-Default)
        // G4: informatives Feld konsistent aus <repetitions count> speisen (die echten Wiederholungen
        // laufen ohnehin ueber die repetition-DynDim aus tp.repetitions; cfg.n_repeats wird nicht geloopt).
        cfg.n_repeats               = (tp.repetitions > 0) ? static_cast<std::uint32_t>(tp.repetitions) : a.n_repeats;
        cfg.env_limits.thread_count = 16;
        if (a.min_free_gb > 0.0) {
            cfg.ram_per_build_bytes     = static_cast<std::uint64_t>(a.min_free_gb * 1024.0 * 1024.0 * 1024.0);
            cfg.ram_safety_margin_bytes = cfg.ram_per_build_bytes;
        }
        return cfg;
    };

    auto emit = [&](ex::LazyRunResult const& r, std::size_t* row_sink) {
        csv << r.resumed_csv_rows;
        // #165-B (P-MD8, 2026-06-20): den statistischen Ausreißer-Flag VOR der Emission je Pass befüllen. Lokale,
        // mutierbare Kopie der frisch gemessenen Zeilen (LazyRunResult bleibt unberührt); annotate_quality_flags
        // setzt nur das additive quality_flag-Feld (0/1) je (binary_id, profile_name)-Gruppe — gate-frei, keine
        // bestehende Spalte/kein Messwert berührt. resumed_csv_rows (Alt-Zeilen) bleiben unangetastet (Datenerhaltung).
        std::vector<ex::LazyMeasuredRow> rows = r.csv_rows;
        ex::annotate_quality_flags(rows);
        for (auto const& row : rows) csv << ex::format_csv_row(row);
        *row_sink += count_lines(r.resumed_csv_rows) + rows.size();
        res.any_measured += r.measured;
        res.any_resumed += r.resumed_binaries;
    };

    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    // S7b — PASS 1..m: BASIS + ACHSEN-SWEEPS (permute_axes/axis_sweeps → source_catalog).
    //
    // FF(#168) — ACHSEN-SWEEP: ist die Pass-Achse sweep-katalogisiert (seit #26/GO-5 ALLE 19 Achsen,
    // is_deepened_axis), kann/soll der Basis-Baum sie nicht variieren (im Profil ggf. 1 Wert gepinnt →
    // level_size==1). Statt der Basis-View baut der Treiber einen EIGENEN Sweep-Baum aus axis_sweep_levels(axis)
    // — 18 Baseline-Ebenen + die gesweepte Achse VOLL — dessen 19-Achsen-binary_ids die axis_sweep_source_map-Keys
    // treffen (in der union_gen). So entsteht je Auspraegung eine REALE distinkte Lebewesen-DLL (z.B. migration_none
    // vs migration_hot_cold).
    //
    // #26/GO-5 (B.4.1-b, 2026-07-12) — MULTI-SWEEP-DURCHLAUF: vorher fuhr run_profile GENAU EINEN Selektions-Pass
    // (Basis ODER die eine args.sweep_axis) — und der E4-Treiber setzt sweep_axis nie, d.h. die im Profil
    // deklarierten <axis_sweeps> blieben im offiziellen XML-Weg UNGEFAHREN (Dossier-GO-5-Eigenbefund B.1; betraf
    // auch den #18-Coverage-Voll-Lauf). Jetzt liefert profile_sweep_passes die deterministische Pass-Liste:
    // explizites args.sweep_axis ⇒ genau 1 Pass (byte-identisch zum Alt-Verhalten); leer ⇒ Basis-Pass + je
    // deklariertem <axis_sweep> ein Pass in Dokument-Reihenfolge. pass_seen_ids dedupliziert ueber die Paesse
    // DIESES Laufs: die idempotente Baseline (und Basis-Achsen-Sweep-ids, die schon im Basis-Pass selektiert
    // wurden) wird nicht erneut selektiert — jede binary_id wird je Lauf GENAU EINMAL gemessen (keine
    // Doppel-Zeilen in der EINEN CSV); im Einzel-Pass-Fall ist das Set leer ⇒ Verhalten unveraendert.
    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    std::set<std::string> pass_seen_ids; // binary_ids bereits gefahrener Paesse DIESES Laufs (Dedupe)

    auto run_selection_pass = [&](std::string const& pass_axis) {
        if (is_deepened_axis(pass_axis)) {
            std::vector<ex::AxisLevel> sweep_levels = axis_sweep_levels(pass_axis);
            // Dieselben DynamicDims wie der Basis-Baum anhaengen (gleiche thread_count×prefetch×repetition-Variation).
            for (auto const& dd : basis_tree.dynamic_filter())
                sweep_levels.push_back(
                    ex::AxisLevel{dd.axis, dd.values, /*is_static=*/false, dd.variable, dd.block_id});
            auto               sweep_factory = std::make_shared<ex::ExperimentNodeFactory>();
            ex::ExperimentTree sweep_tree{sweep_factory};
            sweep_tree.build(sweep_levels);
            ex::StaticBinaryView const sweep_view = sweep_tree.static_binary_view();
            std::size_t const          sweep_n    = sweep_tree.binary_count(); // == |Achsen-Auspraegungen|
            std::vector<std::size_t>   ids;
            ids.reserve(sweep_n);
            for (std::size_t i = 0; i < sweep_n; ++i) // ALLE Auspraegungen (volle Achse), abzgl. bereits gefahrener
                if (pass_seen_ids.insert(sweep_view[i].binary_id).second) ids.push_back(i);
            std::size_t const        fresh_n = ids.size();
            ex::BuildSelection const sel     = ex::select_explicit(std::move(ids));
            std::cout << "  [BASIS/deep-sweep] axis=" << pass_axis << " auspraegungen=" << sweep_n
                      << " davon neu=" << fresh_n << " (eigener Sweep-Baum, NICHT Basis-View)\n";
            for (std::size_t const idx : sel.indices)
                std::cout << "      sweep binary_id[" << idx << "] = " << sweep_view[idx].binary_id << "\n";
            if (fresh_n == 0) {
                std::cout << "      (alle Auspraegungen bereits in frueherem Pass dieses Laufs selektiert — "
                             "Pass uebersprungen)\n";
                return;
            }
            for (std::uint64_t const ws_n : n_sweep) {
                ex::LazyRunConfig       cfg = make_cfg(ws_n, sweep_n, /*series=*/"-", pass_axis, /*pruefling_type=*/"-",
                                                       /*fairness_mode=*/"-", /*h2_score=*/"-");
                ex::LazyRunResult const r =
                    ex::run_lazy_static_then_dynamic(sweep_tree, sel, a.compile, union_gen, ram, cfg, a.algo_sig);
                emit(r, &res.basis_rows);
            }
        } else {
            ProfileTaggedSelection const pts = profile_select(tp, basis_levels, basis_view, pass_axis, N);
            ex::BuildSelection           sel = pts.selection;
            { // Multi-Sweep-Dedupe (im Einzel-Pass-Fall leer ⇒ no-op, Selektion byte-identisch).
                std::vector<std::size_t> fresh;
                fresh.reserve(sel.indices.size());
                for (std::size_t const idx : sel.indices)
                    if (pass_seen_ids.insert(basis_view[idx].binary_id).second) fresh.push_back(idx);
                if (fresh.size() != sel.indices.size()) {
                    std::string const prov = sel.provenance;
                    sel                    = ex::select_explicit(std::move(fresh));
                    sel.provenance         = prov;
                }
            }
            std::cout << "  [BASIS] label=" << pts.label << " provenance=" << sel.provenance
                      << " indices=" << sel.size() << " series=" << pts.series << " sweep=" << pts.sweep_axis << "\n";
            if (sel.indices.empty()) {
                std::cout << "      (keine neuen binary_ids in diesem Pass — Pass uebersprungen)\n";
                return;
            }
            for (std::uint64_t const ws_n : n_sweep) {
                ex::LazyRunConfig       cfg = make_cfg(ws_n, N, pts.series, pts.sweep_axis, /*pruefling_type=*/"-",
                                                       /*fairness_mode=*/"-", /*h2_score=*/"-");
                ex::LazyRunResult const r =
                    ex::run_lazy_static_then_dynamic(basis_tree, sel, a.compile, union_gen, ram, cfg, a.algo_sig);
                emit(r, &res.basis_rows);
            }
        }
    };

    std::vector<std::string> const selection_passes = profile_sweep_passes(tp, a.sweep_axis);
    if (selection_passes.size() > 1)
        std::cout << "  [MULTI-SWEEP] Basis-Pass + " << (selection_passes.size() - 1)
                  << " deklarierte <axis_sweep>-Paesse (Dokument-Reihenfolge, #26/GO-5)\n";
    for (auto const& pass_axis : selection_passes) run_selection_pass(pass_axis);
    // Distinkte Basis-/Sweep-binary_ids DIESES Laufs (Baseline zaehlt ueber alle Paesse genau 1x). Im
    // Einzel-Pass-Fall identisch zur frueheren sel.indices.size()-Zaehlung (Selektionen sind duplikatfrei).
    res.basis_binary_ids = pass_seen_ids.size();

    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    // S7b — PASS 2..k: je <sota_series> EIN SOTA-Reihen-Lebewesen (einwertiger "sota_tier"-Baum, Tag A/B/C).
    //   Disjunkter binary_id-Namensraum ("sota_tier=sota::S::name") → die union_gen liefert die SOTA-Quelle.
    // ════════════════════════════════════════════════════════════════════════════════════════════════════
    if (a.run_sota_series) {
        std::vector<SotaPass> const passes = build_sota_passes(tp);
        std::cout << "  [SOTA] real baubare <sota_series>-Paesse = " << passes.size() << " (von "
                  << tp.sota_series.size() << " deklariert)\n";
        // M-CE-10 (Voll-Review 2026-07-13, Fix (b)): res.sota_binary_ids ist per Doku "distinkte SOTA-Reihen-
        // binary_ids, die gebaut/gemessen wurden" — NICHT die Pass-Zahl. build_sota_passes dedupliziert bereits
        // identische Messungen (KORREKTUR F23 2026-07-16: seit der per-Host-Auffaecherung 2026-07-14 sind St2-
        // Paesse per-Host GENUINE distinkt — "St2 = 1 HOT-Pilot" war die VOR-M-CE-10-Semantik; dedupliziert
        // werden nur noch WIRKLICH identische Deklarationen, gleicher Host + gleicher fairness_mode); dieser
        // Set-Guard fängt zusätzlich die legitimen fairness-Varianten ab (gleiche binary_id, verschiedener
        // fairness_mode ⇒ EINE reale DLL). So bleibt der Zähler exakt == Zahl der real gebauten/gemessenen
        // distinkten DLLs (kein Über-Zählen wie vor dem Fix).
        std::set<std::string> sota_seen_bids;
        for (auto const& p : passes) {
            // Einwertiger Static-Baum: AxisLevel "sota_tier"=<sota_bid> + dieselben DynamicDims wie der Basis-Baum
            // (damit die SOTA-Zeilen die gleiche thread_count×prefetch×repetition-Variation tragen).
            std::vector<ex::AxisLevel> sota_levels;
            sota_levels.push_back(ex::AxisLevel{std::string{kSotaTierAxis}, {p.sota_bid}, /*is_static=*/true, "", ""});
            for (auto const& dd : basis_tree.dynamic_filter())
                sota_levels.push_back(ex::AxisLevel{dd.axis, dd.values, /*is_static=*/false, dd.variable, dd.block_id});

            auto               sota_factory = std::make_shared<ex::ExperimentNodeFactory>();
            ex::ExperimentTree sota_tree{sota_factory};
            sota_tree.build(sota_levels);
            ex::StaticBinaryView const sota_view = sota_tree.static_binary_view();
            // EIN Lebewesen je Reihe = view-Index 0. binary_id == p.view_binary_id ("sota_tier=…").
            ex::BuildSelection const sel = ex::select_explicit({0});
            if (sota_seen_bids.insert(p.view_binary_id).second) ++res.sota_binary_ids; // M-CE-10 (b): distinkt
            // GO-5 Fork 7 + M-CE-10 (c): der tool-berechnete H2-Score wird ueber das HOST-Lebewesen (p.h2_lebewesen)
            // aufgeloest — host-dominant (#171: "abstract" = Host fuellt 18/19 Achsen). KORREKTUR F23 (2026-07-16):
            // die fruehere Aussage 'fuer St2 FIX "hot", NIE das angefragte lebewesen' beschrieb die VOR-M-CE-10-
            // Semantik (nur der HOT-Host existierte als St2-Komposition). Seit der per-Host-Auffaecherung
            // (2026-07-14) gilt fuer ALLE Stufen St1/St2/St3: h2_lebewesen == lebewesen (der per-Host-Replace hat
            // DIESEN Host; Gate: test_sota_st2_dedup asserted EXPECT_EQ(p.h2_lebewesen, p.lebewesen), 19 Paesse).
            // prt_art/fehlende Akte ⇒ honest "n/a" (sota-Profil-Dateistamm == Lebewesen-Name der 6 SOTA).
            std::string const h2_score = h2_score_for(h2_akte, p.h2_lebewesen);
            std::cout << "    SOTA-Pass series=" << p.series << " pruefling_type=" << p.pruefling_type
                      << " fairness=" << p.fairness_mode << " h2_score=" << h2_score
                      << " h2_lebewesen=" << p.h2_lebewesen << " lebewesen=" << p.lebewesen
                      << " binary_id=" << (sota_view.empty() ? std::string{"<leer>"} : sota_view[0].binary_id) << "\n";
            for (std::uint64_t const ws_n : n_sweep) {
                ex::LazyRunConfig cfg =
                    make_cfg(ws_n, 1, p.series, /*sweep_axis=*/"", p.pruefling_type, p.fairness_mode,
                             h2_score); // #171 full/abstract + Fork 6 fairness + Fork 7 h2
                ex::LazyRunResult const r =
                    ex::run_lazy_static_then_dynamic(sota_tree, sel, a.compile, union_gen, ram, cfg, a.algo_sig);
                emit(r, &res.sota_rows);
            }
        }
    }

    csv.flush();
    // M11 (G5-Audit w289llo0o): nach dem Flush das Stream-Ergebnis pruefen. Ein waehrend des
    // Schreibens/Flushens aufgetretener Fehler (Platte voll, IO-Fehler) setzt failbit/badbit und
    // wuerde sonst eine still abgeschnittene CSV als Erfolg ausgeben.
    bool const csv_ok = csv.good();
    std::cout << "RUN_PROFILE fertig: basis_rows=" << res.basis_rows << " sota_rows=" << res.sota_rows
              << " (basis_ids=" << res.basis_binary_ids << " sota_ids=" << res.sota_binary_ids << ")"
              << " measured=" << res.any_measured << " resumed=" << res.any_resumed
              << " csv_ok=" << (csv_ok ? "1" : "0") << " → " << a.out_csv.string() << "\n";

    // Storage #51 (Ebene C, whole-run + datierter Baum): die EINE offizielle CSV NACH dem verifizierten Flush additiv
    // an die NFS-Senke. No-Op-Default (leere measurement_sink) => byte-neutral. Nur bei csv_ok (keine abgeschnittene
    // CSV spiegeln). SYNCHRON (alle Paesse fertig) — kein async/detached.
    if (csv_ok && a.measurement_sink) a.measurement_sink(a.out_csv, "measurements.csv");

    // Exit 0 = mind. 1 (Binary × Setting) real gemessen ODER resumiert (Voll-Resume = gueltiger Lauf)
    // UND die CSV fehlerfrei geschrieben+geflusht (M11). Ein Stream-Schreib-/Flush-Fehler erzwingt exit!=0.
    res.exit_code = ((res.any_measured > 0 || res.any_resumed > 0) && csv_ok) ? 0 : 1;
    return res;
}

} // namespace comdare::cache_engine::thesis_lazy
