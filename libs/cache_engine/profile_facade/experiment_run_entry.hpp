#pragma once
// -----------------------------------------------------------------------------
// BRÜCKE-I4 (2026-07-16) — experiment_run_entry: die umbrella-schwere Lauf-Eintritts-API der
// comdare_experiment-Brücke. GENAU wie run_profile (profile_run_entry.hpp) der EINE Lauf-Einstieg des
// comdare_thesis_profile ist, ist run_experiment_profile der EINE Lauf-Einstieg der 3-Phasen-Experiment-XML.
//
// Leitsatz (Dossier §2, Fork A): die Brücke ist eine PROJEKTION `ExperimentProfile → Pässe des BESTEHENDEN
// run_profile-Unterbaus`. KEIN neuer Bau-, Lade-, Mess- oder CSV-Emitter-Code — run_experiment_profile fährt
// EXAKT dieselben Primitive wie die SOTA-Reihen-Schleife in run_profile (profile_run_entry.hpp:374-423):
//   • project_experiment_to_sota_passes(ep)  (I3, sota_catalog.hpp) → je <phase> die (merge×lebewesen)-Pässe
//     + die view_binary_id→Quelltext-Map (render_sota_module_source).
//   • make_union_source_gen(generated_make_catalog_source_gen(), fused_sota_map) → EINE SourceGenFn.
//   • je Pass EIN einwertiger "sota_tier"-Baum (+ dieselben DynDims workload/repetition wie der Basis-Baum)
//     → select_explicit({0}) → run_lazy_static_then_dynamic (echte DLL via BuildOrchestrator::provision_all →
//     AnatomyModuleLoader::load → dynamic_cast-Kette → Zwei-Phasen-Messung).
//   • ALLE Zeilen in DIE EINE offizielle CSV (lazy_csv_header() genau EINMAL), format_csv_row/annotate_quality_flags
//     byte-gleich zum run_profile-Emitter.
//
// Die binary_id-/Resume-Stabilität bleibt (jeder Pass ruft run_lazy_static_then_dynamic → .version-Sidecar +
// per-Binary-result.csv-Stamp), die CSV-Schema-Identität bleibt (KEINE neue Spalte — I4 nutzt lazy_csv_header()
// unverändert; die Phasen-Provenienz ist aus (series × pruefling_type × binary_id) ableitbar, Dossier §2 S5(b)).
//
// ⚠️ Katalog-/Umbrella-schwer (zieht über profile_run_entry.hpp den generierten Basis-320-Katalog) → gehört in
//    die HARNESS-/Fassaden-.cpp (profile_run_facade.cpp), NICHT in einen engine-agnostischen Treiber-Header.
//    C++23, header-only.
// -----------------------------------------------------------------------------

#include <cache_engine/measurement/load_framework_system_axis.hpp> // INC-1f: Single-Source des "workload"-Unter-Achsen-Labels

#include "profile_run_entry.hpp" // run_profile-Unterbau: count_lines / ex-/wd-Aliase / generated_make_catalog_source_gen /
                                 //   make_union_source_gen / make_system_free_ram_fn / sota_catalog (Projektion I3)

#include <ctime>

namespace comdare::cache_engine::thesis_lazy {

// ── Eingabe der EINEN comdare_experiment-Lauf-API. Analog RunProfileArgs: ALLES, was NICHT aus der Experiment-
//    XML kommt (Pfade/Toolchain/Output/Achse-2-Lastprofile), reicht der Host (die Fassade) hier herein. Die
//    WHAT-Konfiguration (Phasen/Lebewesen/merge) liest run_experiment_profile selbst aus der XML (parse_experiment_profile).
struct RunExperimentArgs {
    std::filesystem::path profile_path; // die comdare_experiment-XML (experiment_golden.xml)
    std::filesystem::path out_csv;      // DIE EINE offizielle CSV (Header genau EINMAL, alle Phasen darunter)
    std::filesystem::path src_dir;      // perm_<id>.cpp-Ausgabe (per-Binary-Subdir-Basis)
    std::filesystem::path dll_dir;      // perm_<id>.dll-Ausgabe (per-Binary-Subdir-Basis)
    ex::CompileFn         compile;      // injizierter Compiler-Aufruf (make_gpp_compile_fn) — wie run_profile
    std::uint64_t         n_ops                = 10000; // Mess-Workload je dyn-Setting
    std::size_t           max_binaries         = 0;     // 0 ⇒ ALLE Pässe; sonst Cap auf die Zahl der SOTA-Pässe (Smoke)
    std::string           build_version        = "m3v2"; // Resume-Marke (.version-Sidecar)
    std::uint32_t         n_repeats            = 3;      // Wiederholungen je (Binary×Setting) — repetition-DynDim
    std::size_t           cores_per_build      = 4;
    double                min_free_gb          = 0.0;
    bool                  resume_override_set  = false;
    bool                  resume               = true;
    std::uint64_t         working_set_override = 0;   // >0 ⇒ ein N-Wert (records); 0 ⇒ records=n_ops
    std::string           platform_override;          // leer ⇒ Default-Tag; sonst Override (CSV-Tag)
    std::string           build_version_tag_override; // leer ⇒ build_version; sonst Override (CSV-Tag)
    // Achse 2 (#135): XML-Lastprofil-Registry (id → WorkloadConfig). Vom Host via discover_load_profiles gesetzt.
    std::map<std::string, wd::WorkloadConfig> workload_registry;
    std::vector<std::string>                  workload_values; // die id-Werte der dynamischen Workload-Achse
};

// ── Ergebnis (rein zählend; die CSV ist die maßgebliche Mess-Ausgabe). ──
struct RunExperimentResult {
    int           exit_code       = 1;
    std::size_t   phases          = 0; // Zahl der projizierten Phasen (ExperimentPhaseProjection)
    std::size_t   sota_rows       = 0; // CSV-Zeilen aller Phasen-Pässe (frisch + resumiert)
    std::size_t   sota_binary_ids = 0; // distinkte gebaute/gemessene view_binary_ids (über alle Phasen)
    std::uint64_t any_measured    = 0;
    std::uint64_t any_resumed     = 0;
};

// ── substitute_date — ersetzt jedes ${date}-Token in einem <output>-Pfad durch das aktuelle Datum (YYYYMMDD).
//    Übernommen aus dem (ersetzten) Parallelstrang v32_messreihe_antrieb.hpp:236 — die Experiment-Golden nutzt
//    `_runs/${date}/…`-Pfade ohne externe Shell-Expansion. Reine Provenienz-Auflösung (E8: die AUTORITATIVE CSV
//    bleibt a.out_csv = e4_xml/measurements.csv; die aufgelösten <output>-Pfade werden nur geloggt).
[[nodiscard]] inline std::string experiment_substitute_date(std::string path) {
    constexpr std::string_view token = "${date}";
    if (path.find(token) == std::string::npos) return path;
    std::time_t const now = std::time(nullptr);
    std::tm           tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char buf[16] = {};
    std::strftime(buf, sizeof(buf), "%Y%m%d", &tm);
    for (auto p = path.find(token); p != std::string::npos; p = path.find(token, p)) path.replace(p, token.size(), buf);
    return path;
}

/// run_experiment_profile — DIE EINE deklarative comdare_experiment-Lauf-API (Brücke-I4). Projiziert die
/// 3-Phasen-XML auf SOTA-Reihen-Pässe (I3) und fährt sie über den BESTEHENDEN run_profile-Unterbau
/// (run_lazy_static_then_dynamic) in DIE EINE offizielle CSV. KEINE Parallelstrecke, KEIN eigener Bau-/Mess-Code.
[[nodiscard]] inline RunExperimentResult run_experiment_profile(RunExperimentArgs const& a) {
    RunExperimentResult res;

    // ── (0) Experiment-XML EINMAL parsen (die EINZIGE WHAT-Quelle). Fremd-Wurzel/unlesbar ⇒ exit 5 (wie run_profile). ──
    cx::XmlConfigParser const parser;
    auto const                ep_opt = parser.parse_experiment_profile(a.profile_path);
    if (!ep_opt) {
        std::cerr << "run_experiment_profile: '" << a.profile_path.string()
                  << "' nicht als comdare_experiment lesbar (parse_experiment_profile=nullopt) — Abbruch.\n";
        res.exit_code = 5;
        return res;
    }
    auto const& ep = *ep_opt;

    // ── (1) I3-Projektion: je <phase> das Kreuzprodukt phase.merge × profile.lebewesen → Pässe + Quellen-Map.
    //    REINER Enumerations-/Render-Schritt (KEIN Bau) — sota_catalog.hpp::project_experiment_to_sota_passes. ──
    std::vector<ExperimentPhaseProjection> const projections = project_experiment_to_sota_passes(ep);
    res.phases                                               = projections.size();

    // ── (2) ${date}-Auflösung der <output>-Pfade als LAUF-PROVENIENZ (E8: die AUTORITATIVE CSV ist a.out_csv =
    //    e4_xml/measurements.csv — EINE gemeinsame offizielle CSV wie der Thesis-Weg; die XML-<output>-Pfade werden
    //    nur aufgelöst geloggt, KEINE Parallel-CSV). ──
    std::cout << "RUN_EXPERIMENT (comdare_experiment-Brücke): " << a.profile_path.string() << "  id=" << ep.id
              << " version=" << ep.version << "  phasen=" << projections.size() << " lebewesen=" << ep.lebewesen.size()
              << " workloads=" << a.workload_values.size() << "\n";
    std::cout << "  [OUTPUT] autoritativ (E8): " << a.out_csv.string() << "\n";
    std::cout << "  [OUTPUT] XML-<output> (aufgelöst, nur Provenienz): csv="
              << experiment_substitute_date(ep.output.csv_path)
              << " bin=" << experiment_substitute_date(ep.output.binary_path)
              << " tex=" << experiment_substitute_date(ep.output.latex_path) << "\n";
    if (!ep.datasets.empty()) {
        std::cout << "  [DATASETS] deklariert=" << ep.datasets.size()
                  << " (Signatur geht in den Resume-Stamp; Loader-Mess-Konsum lauf-gated wie der Thesis-Weg)\n";
        for (auto const& d : ep.datasets)
            std::cout << "      dataset id=" << d.id << " akte_ref=" << d.akte_ref << " loader=" << d.loader << "\n";
    }

    // ── (3) Die EINE vereinigte SourceGenFn: Basis-320-Katalog (für die union-Semantik unkritisch — die
    //    Experiment-Pässe leben im disjunkten "sota_tier=…"-Namensraum) ∪ die Quellen-Map ALLER Phasen. ──
    std::map<std::string, std::string> fused;
    for (auto const& proj : projections)
        for (auto const& [view_id, src] : proj.source_by_view_id) fused.emplace(view_id, src);
    ex::SourceGenFn const union_gen = make_union_source_gen(generated_make_catalog_source_gen(), std::move(fused));
    ex::FreeRamFn         ram       = ex::make_system_free_ram_fn();

    // ── (4) Die DynDims der Experiment-Pässe: Achse 2 (workload) + Wiederholungs-Achse (repetition). Beide
    //    is_static=false ⇒ verändern die binary_id NICHT (Round-Trip-Gate unberührt). Die workload-Ebene ist
    //    ZWINGEND für die Zwei-Phasen-Gültigkeit: run_lazy_static_then_dynamic wählt run_workload_perm (statt
    //    run_observable_perm) NUR, wenn das setting_label das Segment "workload.workload_id=X" trägt
    //    (cache_engine_builder_iterator.hpp:789-793) → two_phase_valid=1. Gleiche Konvention wie der Basis-Baum
    //    (profile_run_entry.hpp:139-141 + profile_to_tree.hpp:105-111). ──
    std::vector<ex::AxisLevel> dyn_levels;
    if (!a.workload_values.empty()) {
        // INC-1f: das Unter-Achsen-Label kommt Single-Source aus der Last-Framework-System-Achse (H-9).
        std::string const workload_label{::comdare::cache_engine::measurement::YcsbLoadFrameworkAxis::sub_axis_label()};
        dyn_levels.push_back(
            ex::AxisLevel{workload_label, a.workload_values, /*is_static=*/false, "workload_id", workload_label});
    }
    {
        std::uint32_t const      reps = (a.n_repeats == 0) ? 1u : a.n_repeats;
        std::vector<std::string> rep_vals;
        rep_vals.reserve(reps);
        for (std::uint32_t r = 0; r < reps; ++r) rep_vals.push_back(std::to_string(r));
        dyn_levels.push_back(
            ex::AxisLevel{"repetition", std::move(rep_vals), /*is_static=*/false, "repetition_index", "repetition"});
    }

    // ── (5) Working-Set-N-Liste (die äußere N-Schleife). Die Experiment-XML deklariert keinen <working_set_sweep>;
    //    working_set_override (env COMDARE_WORKLOAD_RECORDS) greift als einziger N-Wert, sonst 0 (records=n_ops). ──
    std::vector<std::uint64_t> n_sweep;
    if (a.working_set_override > 0) n_sweep.push_back(a.working_set_override);
    if (n_sweep.empty()) n_sweep.push_back(0);

    std::string const tag_platform      = a.platform_override.empty() ? std::string{"win-x86_64"} : a.platform_override;
    std::string const tag_build_version = a.build_version_tag_override.empty()
                                              ? (a.build_version.empty() ? std::string{"m3v2"} : a.build_version)
                                              : a.build_version_tag_override;

    // Deterministische <datasets>-Signatur für den Resume-Stamp (geänderte Deklaration ⇒ konservativ Neu-Messung).
    std::string datasets_signature;
    for (auto const& d : ep.datasets) {
        if (!datasets_signature.empty()) datasets_signature += '|';
        datasets_signature += d.id + ':' + d.akte_ref + ':' + d.loader;
    }

    // ── (6) EINE CSV; Header GENAU EINMAL; darunter alle Phasen-Pässe. Open-Erfolg hart geprüft (M11-Muster). ──
    std::ofstream csv{a.out_csv.string(), std::ios::trunc};
    if (!csv) {
        std::cout << "RUN_EXPERIMENT FEHLER: CSV nicht oeffenbar → " << a.out_csv.string() << "\n";
        res.exit_code = 1;
        return res;
    }
    csv << ex::lazy_csv_header();

    auto emit = [&](ex::LazyRunResult const& r) {
        csv << r.resumed_csv_rows;
        std::vector<ex::LazyMeasuredRow> rows = r.csv_rows; // lokale, mutierbare Kopie (LazyRunResult unberührt)
        ex::annotate_quality_flags(rows); // gate-freier additiver quality_flag (#165-B), wie run_profile
        for (auto const& row : rows) csv << ex::format_csv_row(row);
        res.sota_rows += count_lines(r.resumed_csv_rows) + rows.size();
        res.any_measured += r.measured;
        res.any_resumed += r.resumed_binaries;
    };

    // ── (7) Je Phase, je real baubarem (merge×lebewesen)-Pass: EIN einwertiger "sota_tier"-Baum + DynDims →
    //    select_explicit({0}) → run_lazy_static_then_dynamic. BYTE-gleich zum SOTA-Pass-Muster in run_profile
    //    (profile_run_entry.hpp:387-421). Cap (a.max_binaries>0) begrenzt die GESAMTZAHL der Pässe (Smoke). Der
    //    globale seen-Set dedupliziert view_binary_ids über die Phasen (jede reale DLL genau EINMAL). ──
    std::set<std::string> sota_seen_bids;
    bool const            capped = a.max_binaries > 0;
    for (auto const& proj : projections) {
        std::cout << "  [PHASE] name=" << proj.phase_name << " merge=" << proj.merge
                  << " baubare-paare=" << proj.passes.size() << "\n";
        for (auto const& p : proj.passes) {
            if (capped && sota_seen_bids.size() >= a.max_binaries) {
                std::cout << "    [CAP] max_binaries=" << a.max_binaries << " erreicht — weitere Pässe übersprungen\n";
                break;
            }
            if (!sota_seen_bids.insert(p.view_binary_id).second) continue; // schon in früherer Phase gebaut/gemessen

            std::vector<ex::AxisLevel> sota_levels;
            sota_levels.push_back(ex::AxisLevel{std::string{kSotaTierAxis}, {p.sota_bid}, /*is_static=*/true, "", ""});
            for (auto const& dd : dyn_levels) sota_levels.push_back(dd);

            auto               sota_factory = std::make_shared<ex::ExperimentNodeFactory>();
            ex::ExperimentTree sota_tree{sota_factory};
            sota_tree.build(sota_levels);
            ex::StaticBinaryView const sota_view = sota_tree.static_binary_view();
            ex::BuildSelection const   sel       = ex::select_explicit({0}); // EIN Lebewesen je Reihe = view-Index 0

            std::cout << "    SOTA-Pass phase=" << proj.phase_name << " series=" << p.series
                      << " pruefling_type=" << p.pruefling_type << " lebewesen=" << p.lebewesen
                      << " binary_id=" << (sota_view.empty() ? std::string{"<leer>"} : sota_view[0].binary_id) << "\n";

            for (std::uint64_t const ws_n : n_sweep) {
                ex::LazyRunConfig cfg;
                cfg.max_binaries       = 1; // einwertiger sota_tier-Baum (1 statisches Blatt), wie run_profile
                cfg.n_ops              = a.n_ops;
                cfg.workload_records   = ws_n;
                cfg.workload_configs   = a.workload_registry;
                cfg.build_version      = a.build_version;
                cfg.row_series         = p.series.empty() ? std::string{"-"} : p.series;
                cfg.row_pruefling_type = p.pruefling_type.empty() ? std::string{"-"} : p.pruefling_type;
                cfg.row_sweep_axis     = "-"; // Experiment-Pässe sind keine Achsen-Sweeps
                cfg.row_fairness_mode  = p.fairness_mode.empty() ? std::string{"-"} : p.fairness_mode;
                cfg.row_h2_score       = "-"; // H2-Akte ist Thesis-Profil-Provenienz — von der Brücke nicht getragen
                cfg.profile_datasets   = datasets_signature;
                cfg.row_platform       = tag_platform;
                cfg.row_build_version  = tag_build_version;
                cfg.source_dir         = a.src_dir;
                cfg.output_dir         = a.dll_dir;
                cfg.cores_per_build    = a.cores_per_build;
                cfg.per_binary_subdirs = true;
                cfg.resume_completed_binaries = a.resume_override_set ? a.resume : true;
                cfg.n_repeats                 = (a.n_repeats == 0) ? 1u : a.n_repeats;
                cfg.env_limits.thread_count   = 16;
                if (a.min_free_gb > 0.0) {
                    cfg.ram_per_build_bytes     = static_cast<std::uint64_t>(a.min_free_gb * 1024.0 * 1024.0 * 1024.0);
                    cfg.ram_safety_margin_bytes = cfg.ram_per_build_bytes;
                }
                ex::LazyRunResult const r =
                    ex::run_lazy_static_then_dynamic(sota_tree, sel, a.compile, union_gen, ram, cfg);
                std::cout << "      [pass] selected=" << r.selected << " built=" << r.built
                          << " built_new=" << r.built_new << " loaded=" << r.loaded << " load_failed=" << r.load_failed
                          << " dyn_settings=" << r.dynamic_settings_total << " measured=" << r.measured
                          << " resumed=" << r.resumed_binaries << "\n";
                emit(r);
            }
        }
        if (capped && sota_seen_bids.size() >= a.max_binaries) break;
    }
    res.sota_binary_ids = sota_seen_bids.size();

    csv.flush();
    bool const csv_ok = csv.good();
    std::cout << "RUN_EXPERIMENT fertig: phasen=" << res.phases << " sota_rows=" << res.sota_rows
              << " sota_ids=" << res.sota_binary_ids << " measured=" << res.any_measured
              << " resumed=" << res.any_resumed << " csv_ok=" << (csv_ok ? "1" : "0") << " → " << a.out_csv.string()
              << "\n";

    // Exit 0 = mind. 1 (Binary × Setting) real gemessen ODER resumiert UND die CSV fehlerfrei geschrieben (M11).
    res.exit_code = ((res.any_measured > 0 || res.any_resumed > 0) && csv_ok) ? 0 : 1;
    return res;
}

} // namespace comdare::cache_engine::thesis_lazy
