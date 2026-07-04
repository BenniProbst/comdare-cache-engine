#pragma once
// ExperimentDriver — Library-Refactoring der Phase 1-7 Pipeline (REV 7.6, Q4)
//
// Vorher: main.cpp orchestrierte alle Phasen direkt.
// Jetzt: comdare::builder::ExperimentDriver kapselt Phase 1-7 als Klasse.
// Der Driver (von Diplomarbeit/Code/messung_driver/main.cpp) ruft die
// Methoden in der richtigen Reihenfolge auf.

#include "../codegen/codegen.hpp"
#include "../module_loader/module_loader.hpp"
#include "../permutation_loop/permutation_loop.hpp"
#include "xml_config_parser/xml_config_parser.hpp" // V23.C.2: libs/common/serialization/

#include <cache_engine/abi/module_abi_v1.hpp>
#include <comdare/experiment/result_aggregator.hpp>
#include <comdare/workload_generator/workload_generator.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace comdare::builder {

inline constexpr int status_ok                     = 0;
inline constexpr int status_xml_parse_failed       = 1;
inline constexpr int status_codegen_failed         = 2;
inline constexpr int status_cmake_configure_failed = 3;
inline constexpr int status_cmake_build_failed     = 4;
inline constexpr int status_no_modules_loaded      = 5;
inline constexpr int status_workload_failed        = 6;
inline constexpr int status_export_failed          = 7;

struct ExperimentDriverOptions {
    std::filesystem::path config_dir;   // XML configs (3+1 files)
    std::filesystem::path output_dir;   // Generated cpp + binaries + measurements
    std::filesystem::path comdare_root; // Repo-Root fuer Include-Paths
    bool                  enumerate_only = false;
    bool                  skip_build     = false;
    bool                  verbose        = true; // REV 7.6 F2: Diagnose-Output an

    // REV 7.6 V8.7 — Zwei-Stufen-CacheEngineBuilder:
    //   Stage 1 (vorbereitet ueber CMake-Stack der CacheEngine): Builder-Binary selbst.
    //   Stage 2 (zur Builder-Runtime): pro Permutation entweder vorkompiliertes
    //     Modul laden ODER per cmake/cl-Aufruf hot-kompilieren.
    // Defaults sind so gewaehlt, dass das Verhalten ohne Aenderung dem Stand
    // vor V8.7 entspricht — neue Faehigkeit ist opt-in via Flag.
    bool enable_runtime_codegen = false; // V8.7: hot-compile fehlende Permutationen
    bool enable_functional_tests =
        false; // V8.7: googletest-Funktionalitaetspruefung pro Modul (auch ohne EXPERIMENT_MODE)

    // REV 7.6 V10.6 — Messreihen-Mode (defined/full)
    //   Defined: nur Permutationen, die in sota_profile_filter referenziert sind
    //   Full:    alle Permutationen aus algorithm_profiles/sota/ (Auto-Pickup V10.5)
    enum class MessreihenMode { Defined, Full, FullSampled } messreihen_mode = MessreihenMode::Full;
    static constexpr std::uint32_t default_full_sampled_rate = 1000u;
    std::vector<std::string> sota_profile_filter; // optional: nur diese Profile (Defined-Mode)

    // REV 7.6 AP-5/#239 - FullSampled: deterministische 1:sample_rate-Teilmenge des Full-Raums,
    // seed- und toolchain-stabil (s. permutation_sampling.hpp). Nur wirksam bei messreihen_mode==FullSampled.
    std::uint32_t sample_rate = default_full_sampled_rate; // 1:1000 Default; <=1 deaktiviert Sampling
    std::uint64_t sample_seed = 0;                         // reproduzierbare Variation der Teilmenge

    // REV 7.6 V18.1 — Multi-Path-Lookup fuer Codegen-Templates
    //   Cache-engine SOTA-Templates haben Prioritaet, prt-art Pruefling-Templates Fallback.
    //   Leer = nur cache-engine SOTA wird gesucht.
    std::filesystem::path prt_art_root; // V18.1: optional Pruefling-Repo-Root
};

struct WorkloadOptions {
    workload_generator::WorkloadConfig config{};
    workload_generator::YcsbWorkload   workload = workload_generator::YcsbWorkload::C;
};

class ExperimentDriver {
public:
    explicit ExperimentDriver(ExperimentDriverOptions opts) noexcept : opts_{std::move(opts)} {}

    // Phase 1: ENUMERATION
    [[nodiscard]] int phase1_enumerate(std::vector<loop::PermutationDescriptor>& out_descriptors);

    // Phase 2: GENERATE (codegen pro Permutation + Aggregator-CMakeLists)
    [[nodiscard]] int phase2_generate(std::vector<loop::PermutationDescriptor> const& descriptors);

    // Phase 3: COMPILE (cmake configure + build aller Permutations-DLLs)
    [[nodiscard]] int phase3_compile();

    // REV 7.6 V13.2 — Hot-compile fuer fehlende Module (V8.7 enable_runtime_codegen)
    // Aufruf nach phase2_generate, vor phase4_load. Pro Fingerprint wird
    // gepruefet, ob die Modul-Binary existiert; falls nicht, wird per
    // cmake-Build-Aufruf nachkompiliert. No-op wenn enable_runtime_codegen=false.
    [[nodiscard]] int phase3_hot_compile_missing(std::vector<std::uint64_t> const& fingerprints);

    // REV 7.6 V13.3 — ABI-Vertrags-Pruefung (V8.7 enable_functional_tests)
    // Aufruf zwischen phase4_load und phase5_run_workload. Pro Modul: basic
    // create/destroy + run_workload mit empty WorkloadDescriptor + Pruefen
    // dass rec.version == COMDARE_ABI_VERSION. No-op wenn Flag false.
    [[nodiscard]] int phase4b_functional_tests(std::span<loader::ModuleHandle> handles);

    // Phase 4: LINK+LOAD (alle DLLs via ModuleLoader)
    [[nodiscard]] int phase4_load(std::vector<loader::ModuleHandle>& out_handles);

    // Phase 5+6: EXECUTE+MEASURE pro Modul
    [[nodiscard]] int phase5_run_workload(std::span<loader::ModuleHandle> handles, WorkloadOptions const& workload_opts,
                                          std::vector<loop::PermutationDescriptor> const& descriptors,
                                          experiment::ResultAggregator&                   out_aggregator);

    // Phase 7: PERSIST (CSV + JSON + optional binary)
    [[nodiscard]] int phase7_export(experiment::ResultAggregator const& aggregator);

    // Convenience: Phase 1-7 in einem Aufruf
    [[nodiscard]] int run_pipeline_full(WorkloadOptions const& workload_opts);

    [[nodiscard]] std::filesystem::path const& output_dir() const noexcept { return opts_.output_dir; }
    [[nodiscard]] std::filesystem::path const& build_dir() const noexcept;

private:
    ExperimentDriverOptions opts_;
};

} // namespace comdare::builder
