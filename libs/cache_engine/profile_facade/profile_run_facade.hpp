#pragma once
// -----------------------------------------------------------------------------
// profile_run_facade -- schlanke produktive Fassade um die profile_run_entry-API.
//
// GoF Facade: main.cpp und andere produktive Konsumenten sehen nur diese POD-
// Signatur. Die umbrella-schwere run_profile-Welt bleibt in genau einer .cpp
// gekapselt.
// -----------------------------------------------------------------------------

#include <builder/artifact_transport/artifact_cache.hpp> // Storage #51: CachePushFn / MeasurementSinkFn (No-Op-Naht)

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <ostream>
#include <string>

namespace comdare::cache_engine::builder::profile_facade {

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
    // Storage #51 (No-Op-Default => byte-neutral): der Host (messung_driver) konstruiert sie via
    // artifact_transport::ArtifactCache::from_env und reicht sie zur per-Binary-/whole-run-Naht durch. Leer = No-Op.
    artifact_transport::CachePushFn       cache_push;
    artifact_transport::MeasurementSinkFn measurement_sink;
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
    // Storage #51 (No-Op-Default => byte-neutral): wie ProfileRunArgs — vom Host via from_env konstruiert,
    // zur per-Binary-/whole-run-Naht durchgereicht. Leer = No-Op.
    artifact_transport::CachePushFn       cache_push;
    artifact_transport::MeasurementSinkFn measurement_sink;
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

} // namespace comdare::cache_engine::builder::profile_facade
