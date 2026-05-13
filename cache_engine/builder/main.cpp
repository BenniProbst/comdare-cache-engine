// SPDX-License-Identifier: Apache-2.0
// CacheEngineBuilder Demo-Wrapper (REV 7.6, 2026-05-13)
//
// Vorher (REV 7.2): main.cpp orchestrierte alle Phase-1-bis-7 direkt.
// Jetzt:           kurzer Wrapper, der `comdare::builder::ExperimentDriver`
//                  (Library) aufruft. Echter Experiment-Orchestrator
//                  liegt in Diplomarbeit/Code/messung_driver/main.cpp.
//
// Aufruf:
//   comdare-cache-engine-builder <config_dir> <output_dir>
//       [--enumerate-only] [--comdare-root=DIR] [--skip-build]

#include "experiment_driver/experiment_driver.hpp"

#include <comdare/workload_generator/workload_generator.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void print_usage() {
    std::cerr
        << "Usage: comdare-cache-engine-builder <config_dir> <output_dir> [options]\n\n"
        << "  <config_dir>   Directory containing 4 XML configs:\n"
        << "                   cache_engine_permutations.xml\n"
        << "                   search_algorithm_permutations.xml\n"
        << "                   allocator_permutations.xml\n"
        << "                   test_data_sets.xml\n"
        << "  <output_dir>   Directory for generated modules + measurement output\n\n"
        << "Options:\n"
        << "  --enumerate-only     Skip codegen + execution; print descriptors only\n"
        << "  --comdare-root=DIR   Path to comdare-cache-engine repo root\n"
        << "  --skip-build         Generate sources + aggregator, do not invoke cmake\n\n"
        << "Note: This is a DEMO wrapper. The real Experiment-Orchestrator lives in\n"
        << "      Diplomarbeit/Code/messung_driver/main.cpp (REV 7.6).\n";
}

}  // anonymous

int main(int argc, char* argv[]) {
    if (argc < 3) { print_usage(); return 1; }

    comdare::builder::ExperimentDriverOptions opts;
    opts.config_dir = std::filesystem::path{argv[1]};
    opts.output_dir = std::filesystem::path{argv[2]};
    opts.comdare_root = std::filesystem::current_path();

    for (int i = 3; i < argc; ++i) {
        std::string a{argv[i]};
        if (a == "--enumerate-only") opts.enumerate_only = true;
        else if (a == "--skip-build") opts.skip_build = true;
        else if (a.rfind("--comdare-root=", 0) == 0)
            opts.comdare_root = std::filesystem::path{a.substr(15)};
        else { std::cerr << "Unknown option: " << a << "\n"; print_usage(); return 1; }
    }

    std::cout << "==== CacheEngineBuilder Demo Wrapper (REV 7.6) ====\n";
    std::cout << "Config       : " << opts.config_dir.string() << "\n";
    std::cout << "Output       : " << opts.output_dir.string() << "\n";
    std::cout << "Comdare-Root : " << opts.comdare_root.string() << "\n\n";

    // Default-Demo-Workload (YCSB-C, klein)
    comdare::builder::WorkloadOptions wopts;
    wopts.config.num_keys       = 1000;
    wopts.config.num_operations = 500;
    wopts.config.random_seed    = 42;
    wopts.workload              = comdare::workload_generator::YcsbWorkload::C;

    comdare::builder::ExperimentDriver driver{opts};
    int rc = driver.run_pipeline_full(wopts);
    if (rc != 0) {
        std::cerr << "Pipeline failed (status=" << rc << ")\n";
        return rc;
    }
    std::cout << "Demo pipeline OK.\n";
    return 0;
}
