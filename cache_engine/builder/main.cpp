// CacheEngineBuilder - eigenstaendiges Programm (REV 7 §5)
//
// Master-Orchestrator fuer alle 7 Phasen:
//   1 ENUMERATION   - enumerate alle PermutationDescriptors aus XML
//   2 COMPILE-CONFIG - Template-Parameter setzen pro Permutation
//   3 COMPILE        - cmake build pro Permutation
//   4 LINK + LOAD    - dlopen Permutations-Modul
//   5 EXECUTE        - run_workload
//   6 MEASURE        - sammeln + persistieren
//   7 TEARDOWN       - destroy_instance + unload
//
// Aufruf:
//   cache_engine_builder <config_dir> <workload> <output_dir>

#include "xml_config_parser/xml_config_parser.hpp"
#include "codegen/codegen.hpp"
#include "permutation_loop/permutation_loop.hpp"
#include "experiment_runner/experiment_runner.hpp"
#include "module_loader/module_loader.hpp"

#include <cache_engine/abi/module_abi_v1.hpp>
#include <comdare/workload_generator/workload_generator.hpp>
#include <comdare/experiment/result_aggregator.hpp>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void print_usage() {
    std::cerr << "Usage: cache_engine_builder <config_dir> <output_dir> [options]\n"
              << "\n"
              << "  <config_dir>   Directory containing 4 XML configs:\n"
              << "                   cache_engine_permutations.xml\n"
              << "                   search_algorithm_permutations.xml\n"
              << "                   allocator_permutations.xml\n"
              << "                   test_data_sets.xml\n"
              << "  <output_dir>   Directory for generated modules + measurement output\n"
              << "\n"
              << "Options:\n"
              << "  --enumerate-only     Skip codegen + execution; print descriptors only\n"
              << "  --comdare-root=DIR   Path to comdare-cache-engine repo root\n"
              << "                       (default: current working directory)\n"
              << "  --skip-build         Generate sources + aggregator, but do not invoke cmake\n";
}

}  // anonymous namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }

    std::filesystem::path const config_dir{argv[1]};
    std::filesystem::path const output_dir{argv[2]};
    bool                  enumerate_only = false;
    bool                  skip_build     = false;
    std::filesystem::path comdare_root_override;
    for (int i = 3; i < argc; ++i) {
        std::string const a{argv[i]};
        if (a == "--enumerate-only") {
            enumerate_only = true;
        } else if (a == "--skip-build") {
            skip_build = true;
        } else if (a.rfind("--comdare-root=", 0) == 0) {
            comdare_root_override = std::filesystem::path(a.substr(15));
        } else {
            std::cerr << "Unknown option: " << a << "\n";
            print_usage();
            return 1;
        }
    }

    std::cout << "==== CacheEngineBuilder Phase 7.2 ====\n";
    std::cout << "Config:   " << config_dir.string() << "\n";
    std::cout << "Output:   " << output_dir.string() << "\n";
    std::cout << "Mode:     "
              << (enumerate_only ? "enumerate-only" : (skip_build ? "generate-only" : "full pipeline"))
              << "\n";

    // ====== Phase 1: ENUMERATION ======
    comdare::builder::xml::XmlConfigParser parser;
    auto cfg = parser.parse(config_dir);
    std::cout << "\n[Phase 1] Parsed XML configs:\n";
    std::cout << "  cache_engine_permutations:     " << cfg.cache_engine_permutations.size() << "\n";
    std::cout << "  search_algorithm_permutations: " << cfg.search_algorithm_permutations.size() << "\n";
    std::cout << "  allocator_permutations:         " << cfg.allocator_permutations.size() << "\n";
    std::cout << "  test_data_sets:                  " << cfg.test_data_sets.size() << "\n";

    comdare::builder::loop::PermutationLoop loop;
    auto descriptors = loop.enumerate(cfg);
    std::cout << "  Enumerated " << descriptors.size() << " permutations.\n";

    if (enumerate_only) {
        for (auto const& d : descriptors) {
            std::cout << "    " << d.id << " (fp=0x" << std::hex << d.fingerprint << std::dec << ")\n";
        }
        return 0;
    }

    // ====== Phase 2+3: COMPILE-CONFIG + COMPILE ======
    std::filesystem::create_directories(output_dir);
    std::filesystem::path const comdare_root =
        comdare_root_override.empty() ? std::filesystem::current_path() : comdare_root_override;
    std::cout << "Comdare-Root: " << comdare_root.string() << "\n";

    comdare::builder::codegen::CodegenOptions opts;
    opts.output_root  = output_dir / "generated";
    opts.comdare_root = comdare_root;
    comdare::builder::codegen::CodegenEngine codegen{opts};

    std::cout << "\n[Phase 2] Generating " << descriptors.size() << " modules ...\n";
    std::vector<std::uint64_t> fingerprints;
    fingerprints.reserve(descriptors.size());
    for (auto const& d : descriptors) {
        codegen.generate_module(d.cache_engine_perm,
                                 d.search_algorithm_perm,
                                 d.allocator_perm,
                                 d.fingerprint);
        fingerprints.push_back(d.fingerprint);
        std::cout << "  Generated: " << d.id << "\n";
    }

    // Phase 7.2.A: Aggregator-CMakeLists.txt schreiben (kombiniert alle Permutationen)
    codegen.generate_aggregate_cmake(fingerprints);
    std::cout << "  Aggregator: " << codegen.aggregate_cmake_path().string() << "\n";

    if (skip_build) {
        std::cout << "[--skip-build] Generation done, cmake not invoked.\n";
        return 0;
    }

    // ====== Phase 3: COMPILE (cmake subbuild) ======
    auto const build_dir = output_dir / "build-perms";
    std::filesystem::create_directories(build_dir);

    std::cout << "\n[Phase 3] Configuring cmake subbuild under " << build_dir.string() << " ...\n";

    auto run_cmd = [](std::string const& cmd) -> int {
        std::cout << "  $ " << cmd << "\n";
        int const rc = std::system(cmd.c_str());
        return rc;
    };

    std::string const configure_cmd =
        "cmake -S \"" + opts.output_root.string() + "\" -B \"" + build_dir.string() + "\"";
    int rc = run_cmd(configure_cmd);
    if (rc != 0) {
        std::cerr << "[Phase 3] cmake configure failed with exit " << rc << "\n";
        return rc;
    }

    std::string const build_cmd =
        "cmake --build \"" + build_dir.string() + "\" --target comdare_all_permutations";
    rc = run_cmd(build_cmd);
    if (rc != 0) {
        std::cerr << "[Phase 3] cmake --build failed with exit " << rc << "\n";
        return rc;
    }
    std::cout << "[Phase 3] OK — all " << descriptors.size() << " permutation modules built.\n";

    // ====== Phase 4: LINK + LOAD ======
    // Module sind nach cmake-Build unter build-perms/Debug/comdare_perm_<fp>.<suffix>
    // bzw. unter build-perms/comdare_perm_<fp>.<suffix> (Single-Config-Generator).
    std::filesystem::path const dll_dir_a = build_dir / "Debug";
    std::filesystem::path const dll_dir_b = build_dir;
    std::filesystem::path const dll_dir =
        std::filesystem::is_directory(dll_dir_a) ? dll_dir_a : dll_dir_b;

    std::cout << "\n[Phase 4] Loading permutation modules from " << dll_dir.string() << " ...\n";
    std::vector<comdare::builder::loader::ModuleHandle> handles;
    int const load_status = comdare::builder::loader::ModuleLoader::load_all(dll_dir, handles);
    if (load_status != comdare::builder::loader::status_ok) {
        std::cerr << "[Phase 4] Some modules failed to load (last status="
                  << load_status << ")\n";
    }
    std::cout << "[Phase 4] Loaded " << handles.size() << " of " << descriptors.size()
              << " modules.\n";

    // ====== Phase 5: EXECUTE (Demo-Workload) ======
    comdare::workload_generator::WorkloadConfig wc;
    wc.num_keys       = 1000;
    wc.num_operations = 500;
    wc.random_seed    = 42;
    comdare::workload_generator::WorkloadGenerator gen{wc};
    auto const ops          = gen.generate_ycsb(comdare::workload_generator::YcsbWorkload::C);
    auto const workload_abi = gen.to_abi_descriptor(ops);

    std::cout << "\n[Phase 5+6] Running workload (" << ops.size() << " ops) against "
              << handles.size() << " modules ...\n";

    // Map fingerprint -> descriptor-id fuer Aggregator-Labels
    std::unordered_map<std::uint64_t, std::string> fp_to_id;
    for (auto const& d : descriptors) {
        fp_to_id[d.fingerprint] = d.id;
    }

    comdare::experiment::ResultAggregator aggregator;
    std::uint64_t total_records = 0;
    for (auto& h : handles) {
        auto const* m = h.get();
        if (!m) continue;
        void* inst = m->create_instance(nullptr);
        if (!inst) continue;

        comdare_measurement_record_v1 rec{};
        m->run_workload(inst, &workload_abi, &rec);

        // Phase 7: TEARDOWN
        m->destroy_instance(inst);

        comdare::experiment::PermutationResult pr;
        auto const it = fp_to_id.find(h.fingerprint());
        pr.permutation_id = (it != fp_to_id.end()) ? it->second
                                                    : ("fp_" + std::to_string(h.fingerprint()));
        pr.fingerprint    = h.fingerprint();
        pr.record         = rec;
        pr.succeeded      = true;
        aggregator.add(std::move(pr));
        ++total_records;
    }
    std::cout << "[Phase 5+6] Collected " << total_records << " measurement records.\n";

    // ====== Phase 7: PERSIST + UNLOAD ======
    auto const csv_path  = output_dir / "measurements.csv";
    auto const json_path = output_dir / "measurements.json";
    aggregator.export_csv(csv_path);
    aggregator.export_json(json_path);
    std::cout << "[Phase 7] Wrote " << csv_path.string() << "\n";
    std::cout << "[Phase 7] Wrote " << json_path.string() << "\n";

    // Module-Handles werden durch RAII (Vector-Destructor) entladen.
    handles.clear();
    std::cout << "[Phase 7] Unloaded all modules.\n";

    return 0;
}
