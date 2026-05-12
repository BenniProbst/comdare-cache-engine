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

#include <cache_engine/abi/module_abi_v1.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void print_usage() {
    std::cerr << "Usage: cache_engine_builder <config_dir> <output_dir> [--enumerate-only]\n"
              << "\n"
              << "  <config_dir>   Directory containing 4 XML configs:\n"
              << "                   cache_engine_permutations.xml\n"
              << "                   search_algorithm_permutations.xml\n"
              << "                   allocator_permutations.xml\n"
              << "                   test_data_sets.xml\n"
              << "  <output_dir>   Directory for generated modules + measurement output\n"
              << "  --enumerate-only  Skip codegen + execution; print descriptors only\n";
}

}  // anonymous namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }

    std::filesystem::path const config_dir{argv[1]};
    std::filesystem::path const output_dir{argv[2]};
    bool const enumerate_only = (argc >= 4 && std::string{argv[3]} == "--enumerate-only");

    std::cout << "==== CacheEngineBuilder Phase 6.4 ====\n";
    std::cout << "Config:   " << config_dir.string() << "\n";
    std::cout << "Output:   " << output_dir.string() << "\n";
    std::cout << "Mode:     " << (enumerate_only ? "enumerate-only" : "full pipeline") << "\n";

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
    std::filesystem::path const comdare_root = std::filesystem::current_path();

    comdare::builder::codegen::CodegenOptions opts;
    opts.output_root  = output_dir / "generated";
    opts.comdare_root = comdare_root;
    comdare::builder::codegen::CodegenEngine codegen{opts};

    std::cout << "\n[Phase 2+3] Generating + Compiling " << descriptors.size() << " modules ...\n";
    for (auto const& d : descriptors) {
        codegen.generate_module(d.cache_engine_perm,
                                 d.search_algorithm_perm,
                                 d.allocator_perm,
                                 d.fingerprint);
        std::cout << "  Generated: " << d.id << "\n";
    }
    std::cout << "  [TODO] Invoke cmake -B build && cmake --build build for each module.\n";

    // ====== Phase 4-7: LINK+LOAD + EXECUTE + MEASURE + TEARDOWN ======
    std::cout << "\n[Phase 4-7] Experiment-Runner waitnig for compiled modules.\n";
    std::cout << "  Module-Binaries expected under " << (output_dir / "binaries").string() << "\n";
    std::cout << "  Workload-Descriptors will be derived from test_data_sets.xml.\n";
    std::cout << "  [TODO] Wire up workload + cache_engine_ref + run_all() once cmake builds finish.\n";

    return 0;
}
