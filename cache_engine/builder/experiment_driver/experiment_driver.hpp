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
#include "../xml_config_parser/xml_config_parser.hpp"

#include <cache_engine/abi/module_abi_v1.hpp>
#include <comdare/experiment/result_aggregator.hpp>
#include <comdare/workload_generator/workload_generator.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace comdare::builder {

inline constexpr int status_ok                    = 0;
inline constexpr int status_xml_parse_failed      = 1;
inline constexpr int status_codegen_failed        = 2;
inline constexpr int status_cmake_configure_failed = 3;
inline constexpr int status_cmake_build_failed   = 4;
inline constexpr int status_no_modules_loaded     = 5;
inline constexpr int status_workload_failed       = 6;
inline constexpr int status_export_failed         = 7;

struct ExperimentDriverOptions {
    std::filesystem::path config_dir;       // XML configs (3+1 files)
    std::filesystem::path output_dir;       // Generated cpp + binaries + measurements
    std::filesystem::path comdare_root;     // Repo-Root fuer Include-Paths
    bool                  enumerate_only = false;
    bool                  skip_build     = false;
};

struct WorkloadOptions {
    workload_generator::WorkloadConfig config{};
    workload_generator::YcsbWorkload   workload = workload_generator::YcsbWorkload::C;
};

class ExperimentDriver {
public:
    explicit ExperimentDriver(ExperimentDriverOptions opts) noexcept
        : opts_{std::move(opts)} {}

    // Phase 1: ENUMERATION
    [[nodiscard]] int phase1_enumerate(
        std::vector<loop::PermutationDescriptor>& out_descriptors);

    // Phase 2: GENERATE (codegen pro Permutation + Aggregator-CMakeLists)
    [[nodiscard]] int phase2_generate(
        std::vector<loop::PermutationDescriptor> const& descriptors);

    // Phase 3: COMPILE (cmake configure + build aller Permutations-DLLs)
    [[nodiscard]] int phase3_compile();

    // Phase 4: LINK+LOAD (alle DLLs via ModuleLoader)
    [[nodiscard]] int phase4_load(
        std::vector<loader::ModuleHandle>& out_handles);

    // Phase 5+6: EXECUTE+MEASURE pro Modul
    [[nodiscard]] int phase5_run_workload(
        std::span<loader::ModuleHandle> handles,
        WorkloadOptions const& workload_opts,
        std::vector<loop::PermutationDescriptor> const& descriptors,
        experiment::ResultAggregator& out_aggregator);

    // Phase 7: PERSIST (CSV + JSON + optional binary)
    [[nodiscard]] int phase7_export(experiment::ResultAggregator const& aggregator);

    // Convenience: Phase 1-7 in einem Aufruf
    [[nodiscard]] int run_pipeline_full(WorkloadOptions const& workload_opts);

    [[nodiscard]] std::filesystem::path const& output_dir()  const noexcept { return opts_.output_dir; }
    [[nodiscard]] std::filesystem::path const& build_dir()   const noexcept;

private:
    ExperimentDriverOptions opts_;
};

}  // namespace comdare::builder
