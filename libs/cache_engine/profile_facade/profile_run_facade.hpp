#pragma once
// -----------------------------------------------------------------------------
// profile_run_facade -- schlanke produktive Fassade um die profile_run_entry-API.
//
// GoF Facade: main.cpp und andere produktive Konsumenten sehen nur diese POD-
// Signatur. Die umbrella-schwere run_profile-Welt bleibt in genau einer .cpp
// gekapselt.
// -----------------------------------------------------------------------------

#include <builder/artifact_transport/artifact_cache.hpp> // Storage #51: CachePushFn / MeasurementSinkFn (No-Op-Naht)
#include <builder/experiment_tree/progress_delta.hpp> // Welle 5 (E-W5-2): ProgressSinkFn / ProgressDelta (§38-Naht, No-Op)

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <ostream>
#include <string>

namespace comdare::cache_engine::builder::profile_facade {

// S2-NACHT-3 (2026-07-23): einen COMDARE_PLAN_METHODIK_PROFILE-Wert zum ladbaren Pfad aufloesen. Ein BARE-BASENAME
// (kein Verzeichnisanteil, z.B. "m3_smoke_coverage.profile.xml") wird gegen DASSELBE Verzeichnis wie das Haupt-Profil
// aufgeloest (thesis_profiles/ = main_profile_path.parent_path()) -- dieselbe Wurzel, aus der die anderen Profile
// geladen werden. So kann die super-YAML den KLASSEN-konformen Basename setzen (der zugleich am ceb:trigger
// weitergereicht wird), waehrend die Facade ihn zur Emissionszeit laedt. Ein bereits Pfad-behafteter Wert (absolut
// ODER mit Verzeichnisanteil) bleibt UNVERAENDERT gueltig; leeres main_profile_path (degenerate) => Basename unveraendert.
// Reine Pfad-Arithmetik (kein Datei-I/O) => direkt testbar.
[[nodiscard]] inline std::filesystem::path
resolve_methodik_profile_path(std::filesystem::path const& methodik_value,
                              std::filesystem::path const& main_profile_path) {
    if (methodik_value.has_parent_path() || main_profile_path.empty()) return methodik_value;
    return main_profile_path.parent_path() / methodik_value;
}

struct ProfileRunArgs {
    std::filesystem::path profile_path;
    std::filesystem::path out_csv;
    std::filesystem::path src_dir;
    std::filesystem::path dll_dir;
    std::filesystem::path load_profile_dir;

    std::uint64_t n_ops                = 10000;
    std::size_t   max_binaries         = 0;
    std::string   build_version        = "m3v2";
    std::uint32_t n_repeats            = 3;
    std::size_t   cores_per_build      = 4;
    double        min_free_gb          = 0.0;
    bool          resume_override_set  = false;
    bool          resume               = true;
    bool          run_sota_series      = true;
    std::uint64_t working_set_override = 0;
    std::string   sweep_axis;
    std::string   platform_override;
    std::string   build_version_tag_override;
    // INC-G6 (Ledger 33/34/35, 2026-07-19): der golden-N-Materialisierungs-Kanal (Chunk-Fenster + provision-only).
    // ADDITIV/INERT -- golden_range_count==0 UND provision_only==false => byte-identisch zum Ist-Lauf. Der Host
    // (messung_driver) belegt sie aus COMDARE_GOLDEN_N_RANGE="start:count" bzw. COMDARE_GOLDEN_N_PROVISION_ONLY.
    std::size_t golden_range_start = 0;
    std::size_t golden_range_count = 0;     // 0 = kein Fenster (Ist-Verhalten)
    bool        provision_only     = false; // true = nur bauen, nicht messen
    // S3 (§62-B COMDARE_PRUEF_ONLY): true = NUR das Konformitaets-Gate je gebauter .so (kein Bau, keine Messung). Der
    // Host (messung_driver) belegt es aus COMDARE_PRUEF_ONLY. Gegenseitig ausschliessend mit provision_only.
    bool pruef_only = false;
    // W6 (Ledger §32-F7, 2026-07-19): expliziter Bau-Pool-Worker-Override. Der Host (messung_driver) belegt ihn aus
    // COMDARE_BUILD_PARALLEL (env_parallelism_value). 0 = ungesetzt => parallel_jobs()-Heuristik = byte-neutrales Ist;
    // >0 = harte parallele Compile-Zahl (KOMPILATION parallel, MESSEN bleibt 1-Thread).
    std::size_t build_parallelism = 0;
    // W5-C+ (§36.1 Zellen-Locking, 2026-07-19): der GN-Zellen-Filter der opt×simd-Delegations-Naht. Der Host
    // (messung_driver) belegt sie aus COMDARE_GN_OPT/COMDARE_GN_SIMD. Gesetzt => run_profile baut in dieser
    // Cluster-Zelle NUR die matchende (opt,simd)-Perm; leer (Default) = kein Filter = alle Perms => byte-neutral.
    std::string gn_cell_opt;  // leer = kein opt-Zellen-Filter
    std::string gn_cell_simd; // leer = kein simd-Zellen-Filter
    // Storage #51 (No-Op-Default => byte-neutral): der Host (messung_driver) konstruiert sie via
    // artifact_transport::ArtifactCache::from_env und reicht sie zur per-Binary-/whole-run-Naht durch. Leer = No-Op.
    artifact_transport::CachePushFn cache_push;
    artifact_transport::CachePullFn cache_pull; // S2 (#46a): BATCH-Warm-Cache-Hydrierung VOR dem Bau (No-Op-Default)
    artifact_transport::MeasurementSinkFn measurement_sink;
    // W11 (Ledger §43.c): BAU-Modus async Push -- der Teil-Marker-Sink (nach je chunk_part_size gepushten DLLs) + N.
    // Der Host konstruiert den Sink via ArtifactCache::push_chunk_partial_marker (range gekapselt) + liest N aus
    // COMDARE_GN_PART_SIZE (Default 1024). Leer/0 = keine Teil-Marker (byte-neutral). Der Pump selbst (Ueberlappen)
    // haengt nur an cache_push + provision_only -- die Push-MENGE bleibt unveraendert.
    artifact_transport::PartialMarkerFn partial_marker_sink;
    std::size_t                         chunk_part_size = 0;
    // Welle 5 (E-W5-2, §38-Fortschritts-Rueck-Kanal): No-Op-Default => byte-neutral; Muster EXAKT wie cache_push/
    // measurement_sink. Der Host (messung_driver) konstruiert den Sink und reicht ihn zur per-Binary-/Fenster-Naht
    // durch; run_profile feuert je Binary EIN Delta + am Fensterende EIN done. Leer (Default) => No-Op => golden/CI
    // byte-identisch (Anti-Phantom). Nur der Treiber-Host setzt ihn -- alle anderen Fassaden-Konsumenten bleiben inert.
    experiment::ProgressSinkFn progress_sink;
};

struct ProfileRunResult {
    int           exit_code        = 1;
    std::size_t   basis_rows       = 0;
    std::size_t   sota_rows        = 0;
    std::size_t   basis_binary_ids = 0;
    std::size_t   sota_binary_ids  = 0;
    std::uint64_t measured         = 0;
    std::uint64_t resumed          = 0;
};

[[nodiscard]] ProfileRunResult run_profile_facade(ProfileRunArgs const& args);

// GoF Facade (Brücke-I4, 2026-07-16): die DÜNNE Lauf-Fassade der 3-Phasen-comdare_experiment-XML — das
// Schwester-POD zu run_profile_facade für den zweiten offiziellen Profil-Typ. GLEICHE Bauart (POD-Args rein,
// POD-Result raus, umbrella-frei): parst comdare_experiment → validiert MIT registry_dir + known_workload_ids
// (I1/I2-Gate) → projiziert je Phase auf SOTA-Reihen-Pässe (I3) → speist sie in den BESTEHENDEN run_profile-
// Unterbau (echte DLL via BuildOrchestrator → AnatomyModuleLoader → Zwei-Phasen-Messung) → ALLE Zeilen in DIE
// EINE offizielle CSV (lazy_csv_header() genau EINMAL, E8). KEINE Parallelstrecke, KEIN eigener Bau-/Mess-/CSV-
// Emitter-Code — reine Projektion. Der Host reicht BEIDE Registry-Pfade herein (die ce-Fassade hält KEINEN
// prt-art-Pfad hart vor — Baseline-Layering), identisch zu validate_experiment_profile_facade.
struct ExperimentRunArgs {
    std::filesystem::path profile_path;
    std::filesystem::path out_csv;
    std::filesystem::path src_dir;
    std::filesystem::path dll_dir;
    std::filesystem::path load_profile_dir;  // leer ⇒ co-lokalisierter Default (profile/../load_profiles)
    std::filesystem::path ce_registry_path;  // cache_engine_axis_registry.xml (Validat, 2-Registry-Kanon)
    std::filesystem::path prt_registry_path; // prt_art_axis_registry.xml (Validat, 2-Registry-Kanon)

    std::uint64_t n_ops                = 10000;
    std::size_t   max_binaries         = 0; // 0 ⇒ alle Pässe; sonst Cap auf die Zahl der SOTA-Pässe (Smoke)
    std::string   build_version        = "m3v2";
    std::uint32_t n_repeats            = 3;
    std::size_t   cores_per_build      = 4;
    double        min_free_gb          = 0.0;
    bool          resume_override_set  = false;
    bool          resume               = true;
    std::uint64_t working_set_override = 0;
    std::string   platform_override;
    std::string   build_version_tag_override;
    // W5-C+ (§36.1 Zellen-Locking): der GN-Zellen-Filter — SPIEGEL zu ProfileRunArgs::gn_cell_* (der Host belegt
    // sie aus COMDARE_GN_OPT/COMDARE_GN_SIMD). Leer (Default) = kein Filter = alle Perms => byte-neutral.
    std::string gn_cell_opt;  // leer = kein opt-Zellen-Filter
    std::string gn_cell_simd; // leer = kein simd-Zellen-Filter
    // W6 (Ledger §32-F7): expliziter Bau-Pool-Worker-Override -- SPIEGEL zu ProfileRunArgs::build_parallelism
    // (der Host belegt ihn aus COMDARE_BUILD_PARALLEL). 0 = ungesetzt => byte-neutrales Ist.
    std::size_t build_parallelism = 0;
    // Storage #51 (No-Op-Default => byte-neutral): wie ProfileRunArgs — vom Host via from_env konstruiert,
    // zur per-Binary-/whole-run-Naht durchgereicht. Leer = No-Op.
    artifact_transport::CachePushFn cache_push;
    artifact_transport::CachePullFn cache_pull; // S2 (#46a): BATCH-Warm-Cache-Hydrierung VOR dem Bau (No-Op-Default)
    artifact_transport::MeasurementSinkFn measurement_sink;
    // W11 (Ledger §43.c): BAU-Modus async Push -- der Teil-Marker-Sink (nach je chunk_part_size gepushten DLLs) + N.
    // Der Host konstruiert den Sink via ArtifactCache::push_chunk_partial_marker (range gekapselt) + liest N aus
    // COMDARE_GN_PART_SIZE (Default 1024). Leer/0 = keine Teil-Marker (byte-neutral). Der Pump selbst (Ueberlappen)
    // haengt nur an cache_push + provision_only -- die Push-MENGE bleibt unveraendert.
    artifact_transport::PartialMarkerFn partial_marker_sink;
    std::size_t                         chunk_part_size = 0;
    // Welle 5 (E-W5-2, §38-Fortschritts-Rueck-Kanal): SPIEGEL zu ProfileRunArgs::progress_sink. No-Op-Default =>
    // byte-neutral (Muster wie cache_push/measurement_sink); vom Host durchgereicht bis in den run_experiment-cfg.
    experiment::ProgressSinkFn progress_sink;
};

struct ExperimentRunResult {
    int           exit_code       = 1;
    std::size_t   phases          = 0;
    std::size_t   sota_rows       = 0;
    std::size_t   sota_binary_ids = 0;
    std::uint64_t measured        = 0;
    std::uint64_t resumed         = 0;
};

[[nodiscard]] ExperimentRunResult run_experiment_profile_facade(ExperimentRunArgs const& args);

// GoF Facade: rein-lesende Profil-Vorabpruefung (Pre-Flight, #169(A)/P5, migriert von run_lazy_150
// (run_lazy_150 geloescht 2026-07-11; Host/Emitter heute Code/02_messung_driver, E4-XML)
// --validate). Parst das Thesis-Profil + prueft ALLE Achsen-Werte gegen die realen EnabledStrategies;
// baut KEINE DLL und misst NICHT. Rueckgabe: 0 = gueltig, 1 = Verstoss (Report auf os), 5 = Profil
// nicht lesbar. Erlaubt einen mehrtaegigen Voll-Lauf vor dem teuren Bau abzusichern.
[[nodiscard]] int validate_profile_facade(std::filesystem::path const& profile_path, std::ostream& os);

// GoF Facade (Bruecke-I2, 2026-07-16): rein-lesende Vorabpruefung der 3-Phasen-comdare_experiment-XML
// (ExperimentProfile) — das Schwestergate zu validate_profile_facade fuer den zweiten offiziellen Profil-
// Typ. Parst das Experiment-Profil + loest die je-Engine-Registries am STATISCHEN Pfad auf (2-Registry-
// Kanon): der Host reicht beide Pfade als PARAMETER herein (die ce-Fassade haelt KEINEN prt-art-Pfad hart
// vor — Baseline-Layering). `ce_registry_path` = cache_engine_axis_registry.xml, `prt_registry_path` =
// prt_art_axis_registry.xml. Baut KEINE DLL und misst NICHT. Rueckgabe: 0 = gueltig, 1 = Verstoss (Report
// auf os), 5 = Profil nicht als comdare_experiment lesbar.
[[nodiscard]] int validate_experiment_profile_facade(std::filesystem::path const& profile_path,
                                                     std::filesystem::path const& ce_registry_path,
                                                     std::filesystem::path const& prt_registry_path, std::ostream& os);

// GoF Facade (PAKET W5-B, 2026-07-19): --dump-plan -- rein-lesende Emission des deterministischen
// ExperimentPlanDirector-Walks (GoF Director + PlanTextBuilder) fuer BEIDE offiziellen Profil-Wurzeln. Ein
// Root-Tag-Sniff (common-DOM, wie main.cpp:675-680) waehlt: <comdare_thesis_profile> -> Thesis-Kanal
// (opt x simd x Sweep-Passes), <comdare_experiment> -> Experiment-Kanal (opt x simd x Phasen). Baut KEINE
// DLL und misst NICHT (Anti-Phantom, golden-neutral). Rueckgabe: 0 = Plan-Text nach os emittiert, 5 = Profil
// nicht als bekannte Wurzel lesbar. Der katalog-schwere Planer-Header (experiment_plan_director.hpp:42-43)
// wird NUR in der Fassaden-.cpp inkludiert, NIE in diesem Header (umbrella-frei fuer den Treiber).
[[nodiscard]] int dump_experiment_plan_facade(std::filesystem::path const& profile_path, std::ostream& os);

// GoF Facade (PAKET W7-A, §40.b): --dump-ci -- rein-lesende Emission der deterministischen GitLab-Child-
// Pipeline-YAML (CiYamlBuilder am SELBEN Director-Walk). Die dynamische, Planer-gesteuerte Folge-CI (§40.b:
// Pilot->Serie): zweistufig (STUFE 1 = CEB-Bau-Jobs je System-Perm; STUFE 2 = Tier-Job-Emitter + Grandchild-
// Trigger). Byte-deterministisch/host-unabhaengig (nur CI-Variablen + opt/simd-Plan-Konstanten). Baut KEINE
// DLL und misst NICHT. Rueckgabe: 0 = YAML nach os emittiert, 5 = Profil nicht als bekannte Wurzel lesbar.
[[nodiscard]] int dump_experiment_ci_facade(std::filesystem::path const& profile_path, std::ostream& os);

// GoF Facade (PAKET W7-B, §40.c): --dump-cmake -- rein-lesende Emission des STUFE-1-experiment_plan.cmake
// (CMakeGraphBuilder am SELBEN Director-Walk). W10-A/§42: die MESS-ACHSEN-Stufe (Planer-Rolle) -- je
// Mess-Kombination [a,b,c] ein CEB-Bau- + CEB-Emit-Target (--emit-tier-cmake => Stufe-2). Der Bare-Metal-Bau
// ist damit dreistufig. Byte-deterministisch/host-unabhaengig (Treiber/Profil = CMake-Variablen). Baut KEINE
// DLL und misst NICHT. Rueckgabe: 0 = .cmake nach os emittiert, 5 = Profil nicht als bekannte Wurzel lesbar.
[[nodiscard]] int dump_experiment_cmake_facade(std::filesystem::path const& profile_path, std::ostream& os);

// GoF Facade (PAKET W10-A, §42/§42.b): --emit-tier-ci -- die CEB-ROLLEN-Emission (Stufe 2). Emittiert NUR die
// STUFE-2-Sicht des FREIGEGEBENEN CEB-Raums (System-Perms [d,e,f] + Tier-Chunk-Bau-Jobs "tier:build:[d,e,f]
// [g,h,i]:chunk<k>" + GN-11/320er-gegatete Mess-Jobs "measure:[a,b,c][d,e,f][g,h,i]") als GitLab-Child-2-YAML
// (TierCiYamlBuilder am SELBEN Director-Walk). CEB-Hoheit (§40.b-Praezisierung: der Planer steuert die CEB-Jobs
// via --dump-ci, die CEB steuert die Tier-Jobs via --emit-tier-ci; heute EINE Binary in zwei Rollen). Baut
// KEINE DLL, misst NICHT. Rueckgabe: 0 = YAML nach os emittiert, 5 = Profil nicht als bekannte Wurzel lesbar.
// A5 (§56-T2-FANOUT D4): `combo_selector` (leer = Identitaet, heutige Live-Strecke byte-stabil) waehlt bei N>1
// CEB-Konfigs die EINE repraesentierte Mess-Kombination (cmake_slug der [a,b,c]-Legende, --measurement-combo-Wert).
[[nodiscard]] int emit_tier_ci_facade(std::filesystem::path const& profile_path, std::ostream& os,
                                      std::string const& combo_selector = {});

// GoF Facade (PAKET W10-A, §42/§42.b): --emit-tier-cmake -- der Bare-Metal-Gegenpart zu --emit-tier-ci. Emittiert
// das STUFE-2-tier_plan.cmake (TierCmakeGraphBuilder): je System-Perm die REALEN provision-only-Tier-Chunk-Bau-
// Targets + je Perm ein GN-11/320er-gegatetes measure:-Skelett. Der Ort des Tier-Baus in der dreistufigen
// Bare-Metal-Kette. Byte-deterministisch/host-unabhaengig. Baut KEINE DLL, misst NICHT. Rueckgabe: 0 = .cmake
// nach os emittiert, 5 = Profil nicht als bekannte Wurzel lesbar.
// A8(a)-Symmetrie (§56-T2-FANOUT D4): `combo_selector` SPIEGELT emit_tier_ci_facade (leer = Identitaet, heutige
// Live-Strecke byte-stabil) und waehlt bei N>1 CEB-Konfigs die EINE repraesentierte Mess-Kombination (cmake_slug
// der [a,b,c]-Legende, --measurement-combo-Wert) -- damit die Bare-Metal- und CI-Naht dieselbe Selektor-Semantik tragen.
[[nodiscard]] int emit_tier_cmake_facade(std::filesystem::path const& profile_path, std::ostream& os,
                                         std::string const& combo_selector = {});

// Cache-Resthygiene-2 (2026-07-21): --chunk-organ-fingerprint -- druckt das Chunk-Organ-Fingerprint-PRE-IMAGE nach os
// (die stem-sortiert konkatenierten perm.dll.algos-Inhalte der Range-Binaries). Die CI pipet os durch `sha256sum` ->
// COMDARE_GN_ALGO_SIG == der S1-F1-Whole-Chunk-Marker-algo_sig (macht die Marker-Wache scharf). Range = (start,count);
// count==0 => ganze Basis-View. Baut KEINE DLL (rein aus dem Katalog). Rueckgabe 0 (immer, auch bei leerem Praefix).
[[nodiscard]] int chunk_organ_fingerprint_facade(std::filesystem::path const& profile_path, std::size_t range_start,
                                                 std::size_t range_count, std::ostream& os);

// R8 (Nacht-Audit 2026-07-22): --print-cache-key -- druckt cache_key_prefix(base_build_version + Perm-Suffix aus der
// Env) nach os (EINE Zeile), damit die CI (.golden_n_build) den VOLLEN ce-Objekt-Key LITERAL konsumiert (kein
// bash-Key-Drift der +bt/+ceb/+mtool/+mrg-Segmente). base_build_version = die Mess-Lauf-build_version (main: "m3v2");
// Perm-Suffix aus COMDARE_GN_OPT/COMDARE_GN_SIMD/COMDARE_CXX/COMDARE_BUILD_TYPE (perm-loop-Reihenfolge); +ceb/+mtool/
// +mrg aus ArtifactCache::from_env(). Baut KEINE DLL. Rueckgabe 0.
[[nodiscard]] int print_cache_key_facade(std::string const& base_build_version, std::ostream& os);

// G1 (K7b-4, Section 62-B, B6-Auflage): --version -- druckt den Je-Binary-Selbst-Stempel des Treiber-Binary (Planer- +
// CEB-Rolle) nach os: vier gelabelte non-empty Zeilen (planner-Selbst-Stempel / ceb-contract / build-type /
// build-version = system_axes_version_suffix). Rein-lesend, baut KEINE DLL. Rueckgabe 0.
[[nodiscard]] int print_version_facade(std::ostream& os);

} // namespace comdare::cache_engine::builder::profile_facade
