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
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tool = ::comdare::cache_engine::builder::codegen_tool;

namespace {

/// Parst 'short|cpp_type|header' in einen CompositionDescriptor. Die string_views verweisen in
/// `spec` (= ein argv-Eintrag, stabil fuer die gesamte main-Laufzeit) — zero-copy, kein Lifetime-Issue.
/// Ermoeglicht externe (Pruefling-)Compositions via Plugin-Controller (F.5).
[[nodiscard]] std::optional<tool::CompositionDescriptor>
parse_external_composition(std::string_view spec) {
    auto const p1 = spec.find('|');
    if (p1 == std::string_view::npos) return std::nullopt;
    auto const p2 = spec.find('|', p1 + 1);
    if (p2 == std::string_view::npos) return std::nullopt;
    auto const sname = spec.substr(0, p1);
    auto const ctype = spec.substr(p1 + 1, p2 - p1 - 1);
    auto const hinc  = spec.substr(p2 + 1);
    if (sname.empty() || ctype.empty() || hinc.empty()) return std::nullopt;
    return tool::CompositionDescriptor{sname, ctype, hinc};
}

void print_usage() {
    std::cerr <<
        "Usage: comdare-anatomy-codegen-tool [options]\n"
        "\n"
        "Options:\n"
        "  --output FILE        Output CMake-snippet file (required)\n"
        "  --names LIST         Comma-separated short-names (default: alle)\n"
        "  --library-type T     SHARED | STATIC (default: SHARED)\n"
        "  --external-composition SPEC\n"
        "                       Externe (Pruefling-)Composition als 'short|cpp_type|header'\n"
        "                       (wiederholbar). Ermoeglicht Codegen von prt-art-Kompositionen\n"
        "                       ueber den Plugin-Controller (F.5 comdare_perms_pa/full_join).\n"
        "  --no-known           Bekannte CE-Compositions NICHT einschliessen (nur --external-*)\n"
        "  --list               Print known compositions and exit\n"
        "  --help               Show this help and exit\n"
        "\n"
        "Example (CE-only):\n"
        "  comdare-anatomy-codegen-tool \\\n"
        "    --output  /tmp/perm_list.cmake \\\n"
        "    --names   art,hot,wormhole \\\n"
        "    --library-type SHARED\n"
        "\n"
        "Example (Pruefling-Stufe-2, nur prt-art-Composition):\n"
        "  comdare-anatomy-codegen-tool --output /tmp/perms_pa.cmake --no-known \\\n"
        "    --external-composition 'prtart-demo|::comdare::prt_art::slots::PrtArtCompositionDemo|prt_art/slots/prt_art_composition_demo.hpp'\n"
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
    bool no_known = false;
    // Reserviert, damit spaeter genommene Adressen (&ext_descs[i]) stabil bleiben.
    std::vector<tool::CompositionDescriptor> ext_descs;
    ext_descs.reserve(static_cast<std::size_t>(argc));

    for (int i = 1; i < argc; ++i) {
        std::string_view const a{argv[i]};
        if (a == "--help") { print_usage(); return 0; }
        if (a == "--list") { print_list();  return 0; }
        if (a == "--output" && i + 1 < argc) { output_path = argv[++i]; continue; }
        if (a == "--names"  && i + 1 < argc) { names_csv   = argv[++i]; continue; }
        if (a == "--library-type" && i + 1 < argc) { library_type = argv[++i]; continue; }
        if (a == "--no-known") { no_known = true; continue; }
        if (a == "--external-composition" && i + 1 < argc) {
            auto const d = parse_external_composition(argv[++i]);
            if (!d) {
                std::cerr << "ERROR: --external-composition erwartet 'short|cpp_type|header' "
                             "(3 nicht-leere |-getrennte Felder).\n";
                return 1;
            }
            ext_descs.push_back(*d);
            continue;
        }
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

    std::vector<tool::CompositionDescriptor const*> selected;
    if (!no_known) {
        std::vector<std::string> unknown;
        selected = tool::select_compositions(names_csv, &unknown);
        if (!unknown.empty()) {
            std::cerr << "ERROR: unknown composition short-name(s):\n";
            for (auto const& u : unknown) {
                std::cerr << "  - " << u << '\n';
            }
            std::cerr << "Use --list to see available compositions.\n";
            return 1;
        }
    }
    // Externe (Pruefling-)Compositions anhaengen (F.5: Plugin-Controller-Codegen).
    for (auto const& d : ext_descs) {
        selected.push_back(&d);
    }

    if (selected.empty()) {
        std::cerr << "ERROR: no compositions selected (empty result; "
                     "ggf. --no-known ohne --external-composition?).\n";
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
