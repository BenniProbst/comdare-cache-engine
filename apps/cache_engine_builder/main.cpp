// SPDX-License-Identifier: Apache-2.0
// CacheEngineBuilder — Library-Wrapper (REV 7.6, 2026-05-13)
//
// Diese main.cpp ist ein duenner Wrapper um die
// comdare::builder::ExperimentDriver-Library. Die volle Pipeline-Logik
// (Phase 1-7) und alle Diagnose-Outputs leben in der Library.
//
// Architektur (User-Direktive 2026-05-13): Die cache-engine bestimmt WIE
// gemessen wird (Builder, Codegen, ModuleLoader, Mikrobenchmark-Suite,
// Workload-Generator-Library) — die Diplomarbeit bestimmt WAS gemessen
// wird (Compile-time-XML-Configs + 3-Messreihen-Loop in
// Diplomarbeit/Code/messung_driver/).
//
// Aufruf:
//   comdare-cache-engine-builder <config_dir> <output_dir> [options]
//
//   <config_dir>   Directory containing 4 XML configs:
//                    cache_engine_permutations.xml
//                    search_algorithm_permutations.xml
//                    allocator_permutations.xml
//                    test_data_sets.xml   [DEPRECATED-Slot, GO-5 Fork 2/R2 2026-07-12: die Datei
//                                          existiert nirgends; Wahrheitsquelle der Datensaetze sind
//                                          die Akten Code/test_data_xml/*.test_data.xml — KEINE
//                                          Doppelquelle nachtraeglich erzeugen]
//   <output_dir>   Directory for generated modules + measurement output
//
// Optionen:
//   -h, --help           Show usage text and exit (status 0)
//   --enumerate-only     Skip codegen + execution; print descriptors only
//   --comdare-root=DIR   Path to comdare-cache-engine repo root
//                        (default: current working directory)
//   --skip-build         Generate sources + aggregator, do not invoke cmake
//   --quiet              Disable per-phase diagnostic output

#include "experiment_driver/experiment_driver.hpp"

#include <comdare/workload_generator/workload_generator.hpp>

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

namespace {

using MessreihenMode = comdare::builder::ExperimentDriverOptions::MessreihenMode;

inline constexpr std::string_view mode_prefix        = "--mode=";
inline constexpr std::string_view sample_rate_prefix = "--sample-rate=";
inline constexpr std::string_view sample_seed_prefix = "--sample-seed=";

[[nodiscard]] constexpr std::string_view messreihen_mode_name(MessreihenMode mode) noexcept {
    switch (mode) {
        case MessreihenMode::Defined: return "defined";
        case MessreihenMode::Full: return "full";
        case MessreihenMode::FullSampled: return "full-sampled";
    }
    return "unknown";
}

void print_usage(std::ostream& os) {
    os << "Usage: comdare-cache-engine-builder <config_dir> <output_dir> [options]\n\n"
       << "  <config_dir>   Directory containing 4 XML configs:\n"
       << "                   cache_engine_permutations.xml\n"
       << "                   search_algorithm_permutations.xml\n"
       << "                   allocator_permutations.xml\n"
       << "                   test_data_sets.xml  [DEPRECATED-Slot; Wahrheitsquelle sind die\n"
       << "                                        Code/test_data_xml/*.test_data.xml-Akten (GO-5 Fork 2)]\n"
       << "  <output_dir>   Directory for generated modules + measurement output\n\n"
       << "Options:\n"
       << "  -h, --help           Show this usage text and exit (status 0)\n"
       << "  --enumerate-only     Skip codegen + execution; print descriptors only\n"
       << "  --comdare-root=DIR   Path to comdare-cache-engine repo root\n"
       << "                       (default: current working directory)\n"
       << "  --mode=<defined|full|full-sampled>\n"
       << "                       Messreihen mode (default: full)\n"
       << "  --sample-rate=<N>    FullSampled sampling rate (default: 1000)\n"
       << "  --sample-seed=<N>    FullSampled sampling seed (default: 0)\n"
       << "  --skip-build         Generate sources + aggregator, do not invoke cmake\n"
       << "  --quiet              Disable per-phase diagnostic output\n\n"
       << "Note: This Builder is the cache-engine-Demo-Driver. The real\n"
       << "      Experiment-Orchestrator (3 Messreihen A/B/C) lives in\n"
       << "      Diplomarbeit/Code/messung_driver/ (REV 7.6).\n";
}

} // namespace

int main(int argc, char* argv[]) {
    // #193-D — -h/--help: Usage nach stdout + Status 0 (User-Self-Test: Programm-Einstieg auffindbar).
    for (int i = 1; i < argc; ++i) {
        std::string_view a{argv[i]};
        if (a == "--help" || a == "-h") {
            print_usage(std::cout);
            return 0;
        }
    }

    if (argc < 3) {
        print_usage(std::cerr);
        return 1;
    }

    comdare::builder::ExperimentDriverOptions opts;
    opts.config_dir   = std::filesystem::path{argv[1]};
    opts.output_dir   = std::filesystem::path{argv[2]};
    opts.comdare_root = std::filesystem::current_path();
    opts.verbose      = true;

    for (int i = 3; i < argc; ++i) {
        std::string_view a{argv[i]};
        if (a == "--enumerate-only")
            opts.enumerate_only = true;
        else if (a == "--skip-build")
            opts.skip_build = true;
        else if (a == "--quiet")
            opts.verbose = false;
        else if (a.rfind("--comdare-root=", 0) == 0) {
            opts.comdare_root = std::filesystem::path{std::string{a.substr(15)}};
        } else if (a.rfind(mode_prefix, 0) == 0) {
            std::string_view m = a.substr(mode_prefix.size());
            if (m == "defined")
                opts.messreihen_mode = MessreihenMode::Defined;
            else if (m == "full")
                opts.messreihen_mode = MessreihenMode::Full;
            else if (m == "full-sampled")
                opts.messreihen_mode = MessreihenMode::FullSampled;
            else {
                std::cerr << "Unknown --mode: " << m << " (defined|full|full-sampled)\n";
                return 1;
            }
        } else if (a.rfind(sample_rate_prefix, 0) == 0) {
            std::string_view v = a.substr(sample_rate_prefix.size());
            std::uint32_t    r{};
            auto const [p, ec] = std::from_chars(v.data(), v.data() + v.size(), r);
            if (ec != std::errc{} || p != v.data() + v.size()) {
                std::cerr << "Bad --sample-rate: " << v << "\n";
                return 1;
            }
            opts.sample_rate = r;
        } else if (a.rfind(sample_seed_prefix, 0) == 0) {
            std::string_view v = a.substr(sample_seed_prefix.size());
            std::uint64_t    s{};
            auto const [p, ec] = std::from_chars(v.data(), v.data() + v.size(), s);
            if (ec != std::errc{} || p != v.data() + v.size()) {
                std::cerr << "Bad --sample-seed: " << v << "\n";
                return 1;
            }
            opts.sample_seed = s;
        } else {
            std::cerr << "Unknown option: " << a << "\n";
            print_usage(std::cerr);
            return 1;
        }
    }

    std::cout << "==== CacheEngineBuilder (REV 7.6 Library-Wrapper) ====\n";
    std::cout << "Config       : " << opts.config_dir.string() << "\n";
    std::cout << "Output       : " << opts.output_dir.string() << "\n";
    std::cout << "Comdare-Root : " << opts.comdare_root.string() << "\n";
    std::cout << "Pipeline     : "
              << (opts.enumerate_only ? "enumerate-only" : (opts.skip_build ? "generate-only" : "full pipeline"))
              << "\n";
    std::cout << "Messreihe    : " << messreihen_mode_name(opts.messreihen_mode);
    if (opts.messreihen_mode == MessreihenMode::FullSampled) {
        std::cout << " (sample-rate=" << opts.sample_rate << ", sample-seed=" << opts.sample_seed << ")";
    }
    std::cout << "\n";
    std::cout << "Verbose      : " << (opts.verbose ? "ON" : "OFF") << "\n";

    // Demo-Workload (klein, YCSB-C). Echte Workloads pro Messreihe sind in
    // Diplomarbeit/Code/messung_driver/ (3 Messreihen A/B/C mit eigenen Configs).
    comdare::builder::WorkloadOptions wopts;
    wopts.config.num_keys       = 1000;
    wopts.config.num_operations = 500;
    wopts.config.random_seed    = 42;
    wopts.workload              = comdare::workload_generator::YcsbWorkload::C;

    comdare::builder::ExperimentDriver driver{opts};
    int const                          rc = driver.run_pipeline_full(wopts);
    if (rc != comdare::builder::status_ok) {
        std::cerr << "\n[CacheEngineBuilder] Pipeline failed (status=" << rc << ")\n";
        return rc;
    }

    std::cout << "\n==== CacheEngineBuilder OK ====\n";
    return 0;
}
