// SPDX-License-Identifier: Apache-2.0
// V41.F.6.1.R5.F — comdare-anatomy-codegen-tool CLI Wrapper
//
// Generiert ein CMake-Composition-Snippet aus einer Selektion der 11 known
// Compositions. Output wird via include() von einem nachfolgenden CMake-
// Configure-Pass eingelesen und ueber comdare_codegen_anatomy_module_list()
// in N SHARED/STATIC-Permutations-DLLs ueberfuehrt.
//
// @task #710 V41.F.6.1.R5.F

#include <builder/anatomy_codegen_tool/anatomy_codegen_tool.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace tool = ::comdare::cache_engine::builder::codegen_tool;

namespace {

void print_usage() {
    std::cerr <<
        "Usage: comdare-anatomy-codegen-tool [options]\n"
        "\n"
        "Options:\n"
        "  --output FILE        Output CMake-snippet file (required)\n"
        "  --names LIST         Comma-separated short-names (default: alle)\n"
        "  --library-type T     SHARED | STATIC (default: SHARED)\n"
        "  --list               Print known compositions and exit\n"
        "  --help               Show this help and exit\n"
        "\n"
        "Example:\n"
        "  comdare-anatomy-codegen-tool \\\n"
        "    --output  /tmp/perm_list.cmake \\\n"
        "    --names   art,hot,wormhole \\\n"
        "    --library-type SHARED\n"
        "\n"
        "Output (CMake-Snippet) wird via include(<output>) in einem CMake-Pass\n"
        "verwendet, der dann comdare_codegen_anatomy_module_list(\n"
        "    LIBRARY_TYPE  ${COMDARE_PERMUTATION_LIBRARY_TYPE}\n"
        "    COMPOSITIONS  ${COMDARE_PERMUTATION_COMPOSITIONS}) aufruft.\n";
}

void print_list() {
    auto const all = tool::known_compositions();
    std::cout << "Available compositions (" << all.size() << "):\n";
    for (auto const& c : all) {
        std::cout << "  " << c.short_name
                  << "\n    type:    " << c.cpp_type_name
                  << "\n    header:  " << c.header_include
                  << '\n';
    }
}

}  // anonymous

int main(int argc, char** argv) {
    std::string output_path;
    std::string names_csv;
    std::string library_type = "SHARED";

    for (int i = 1; i < argc; ++i) {
        std::string_view const a{argv[i]};
        if (a == "--help") { print_usage(); return 0; }
        if (a == "--list") { print_list();  return 0; }
        if (a == "--output" && i + 1 < argc) { output_path = argv[++i]; continue; }
        if (a == "--names"  && i + 1 < argc) { names_csv   = argv[++i]; continue; }
        if (a == "--library-type" && i + 1 < argc) { library_type = argv[++i]; continue; }
        std::cerr << "Unknown option: " << a << "\n\n";
        print_usage();
        return 1;
    }

    if (output_path.empty()) {
        std::cerr << "ERROR: --output FILE is required.\n\n";
        print_usage();
        return 1;
    }

    auto const* lib_type_ptr = tool::parse_library_type(library_type);
    if (!lib_type_ptr) {
        std::cerr << "ERROR: --library-type must be SHARED or STATIC (got: "
                  << library_type << ").\n";
        return 1;
    }

    std::vector<std::string> unknown;
    auto const selected = tool::select_compositions(names_csv, &unknown);

    if (!unknown.empty()) {
        std::cerr << "ERROR: unknown composition short-name(s):\n";
        for (auto const& u : unknown) {
            std::cerr << "  - " << u << '\n';
        }
        std::cerr << "Use --list to see available compositions.\n";
        return 1;
    }

    if (selected.empty()) {
        std::cerr << "ERROR: no compositions selected (empty result).\n";
        return 1;
    }

    if (!tool::write_cmake_snippet(output_path, selected, *lib_type_ptr)) {
        std::cerr << "ERROR: failed to write output file: " << output_path << "\n";
        return 1;
    }

    std::cout << "comdare-anatomy-codegen-tool: wrote " << selected.size()
              << " composition entries to " << output_path
              << " (library_type=" << library_type << ")\n";
    return 0;
}
