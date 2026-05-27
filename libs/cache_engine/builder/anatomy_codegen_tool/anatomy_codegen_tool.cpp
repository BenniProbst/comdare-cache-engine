// V41.F.6.1.R5.F + R5.H — anatomy_codegen_tool Library Implementation
//
// R5.H (2026-05-27 spaet): hardcoded Strings durch trait-driven
// descriptor_from_composition<C>() ersetzt. Tabelle behaelt nur noch
// (short_name, Type)-Mapping; cpp_type_name + header_include kommen aus
// den 11 Reference-Composition-Traits (HasCompositionLocation Concept, R5.G).
// Drift zwischen Composition-Header und Tool-Tabelle nicht mehr moeglich.
//
// @task #712 V41.F.6.1.R5.H

#include "anatomy_codegen_tool.hpp"

// R5.H: alle 11 Reference-Compositions fuer Trait-Extraktion
#include "../../compositions/art_reference.hpp"
#include "../../compositions/hot_reference.hpp"
#include "../../compositions/wormhole_reference.hpp"
#include "../../compositions/surf_reference.hpp"
#include "../../compositions/masstree_reference.hpp"
#include "../../compositions/start_reference.hpp"
#include "../../compositions/art_paper_binding_reference.hpp"
#include "../../compositions/hot_paper_binding_reference.hpp"
#include "../../compositions/start_paper_binding_reference.hpp"
#include "../../compositions/wormhole_paper_binding_reference.hpp"
#include "../../compositions/surf_paper_binding_reference.hpp"

#include <array>
#include <fstream>

namespace comdare::cache_engine::builder::codegen_tool {

// ─────────────────────────────────────────────────────────────────────────────
// kKnownCompositions Tabelle (11 Eintraege, R5.H trait-driven)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

namespace comp = ::comdare::cache_engine::compositions;

/// make_desc<C>(short_name) — extrahiert cpp_type_name + header_include via
/// Trait (descriptor_from_composition<C>()), ueberschreibt nur den User-friendly
/// CLI-Short-Name. Damit ist die Tool-Tabelle drift-frei: jede Aenderung in
/// einer Composition propagiert automatisch in die Tool-Tabelle.
template <::comdare::cache_engine::anatomy::HasCompositionLocation C>
[[nodiscard]] constexpr CompositionDescriptor make_desc(std::string_view short_name) noexcept {
    auto d = descriptor_from_composition<C>();
    d.short_name = short_name;  // User-friendly CLI-Name (z.B. "art" statt "ArtComposition")
    return d;
}

constexpr std::array<CompositionDescriptor, 11> kKnownCompositionsImpl = {{
    // 6 CE-Reimpl-Compositions (R2)
    make_desc<comp::ArtComposition>("art"),
    make_desc<comp::HotComposition>("hot"),
    make_desc<comp::WormholeComposition>("wormhole"),
    make_desc<comp::SurfComposition>("surf"),
    make_desc<comp::MasstreeComposition>("masstree"),
    make_desc<comp::StartComposition>("start"),
    // 5 PaperBinding-Compositions (R3.2 Promotion)
    make_desc<comp::ArtPaperBindingComposition>("art_pb"),
    make_desc<comp::HotPaperBindingComposition>("hot_pb"),
    make_desc<comp::StartPaperBindingComposition>("start_pb"),
    make_desc<comp::WormholePaperBindingComposition>("wormhole_pb"),
    make_desc<comp::SurfPaperBindingComposition>("surf_pb"),
}};

}  // anonymous namespace

std::span<CompositionDescriptor const> known_compositions() noexcept {
    return std::span<CompositionDescriptor const>{kKnownCompositionsImpl.data(),
                                                   kKnownCompositionsImpl.size()};
}

// ─────────────────────────────────────────────────────────────────────────────
// find_composition / select_compositions
// ─────────────────────────────────────────────────────────────────────────────

CompositionDescriptor const* find_composition(std::string_view short_name) noexcept {
    for (auto const& c : kKnownCompositionsImpl) {
        if (c.short_name == short_name) return &c;
    }
    return nullptr;
}

namespace {

std::vector<std::string_view> split_csv(std::string_view s) {
    std::vector<std::string_view> out;
    while (!s.empty()) {
        auto const pos = s.find(',');
        out.push_back(s.substr(0, pos));
        if (pos == std::string_view::npos) break;
        s.remove_prefix(pos + 1);
    }
    return out;
}

}  // anonymous namespace

std::vector<CompositionDescriptor const*>
select_compositions(std::string_view csv_names,
                    std::vector<std::string>* unknown_out)
{
    std::vector<CompositionDescriptor const*> out;
    if (csv_names.empty()) {
        out.reserve(kKnownCompositionsImpl.size());
        for (auto const& c : kKnownCompositionsImpl) {
            out.push_back(&c);
        }
        return out;
    }

    for (auto const name_view : split_csv(csv_names)) {
        auto const* d = find_composition(name_view);
        if (d) {
            out.push_back(d);
        } else if (unknown_out) {
            unknown_out->emplace_back(name_view);
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// LibraryType-Parsing
// ─────────────────────────────────────────────────────────────────────────────

LibraryType const* parse_library_type(std::string_view s) noexcept {
    static constexpr LibraryType kShared = LibraryType::Shared;
    static constexpr LibraryType kStatic = LibraryType::Static;
    if (s == "SHARED") return &kShared;
    if (s == "STATIC") return &kStatic;
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// write_cmake_snippet
// ─────────────────────────────────────────────────────────────────────────────

bool write_cmake_snippet(std::filesystem::path const& output_path,
                         std::span<CompositionDescriptor const* const> selected,
                         LibraryType lib_type)
{
    // Parent-Verzeichnis erstellen wenn noetig
    std::error_code ec;
    if (auto parent = output_path.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, ec);
    }

    std::ofstream out{output_path};
    if (!out) return false;

    out << "# Auto-generated by comdare-anatomy-codegen-tool (V41.F.6.1.R5.F) — DO NOT EDIT\n";
    out << "# Selected " << selected.size() << " composition entries.\n";
    out << "#\n";
    out << "# Verwendung (im Aufrufer-CMakeLists.txt):\n";
    out << "#   include(<this-file>)\n";
    out << "#   include(anatomy_codegen)\n";
    out << "#   comdare_codegen_anatomy_module_list(\n";
    out << "#       PILOT_PREFIX  anatomy_perm_pilot\n";
    out << "#       OUTPUT_DIR    \"${CMAKE_BINARY_DIR}/generated/anatomy_modules\"\n";
    out << "#       LIBRARY_TYPE  ${COMDARE_PERMUTATION_LIBRARY_TYPE}\n";
    out << "#       COMPOSITIONS  ${COMDARE_PERMUTATION_COMPOSITIONS})\n";
    out << "\n";
    out << "set(COMDARE_PERMUTATION_LIBRARY_TYPE \"" << library_type_name(lib_type) << "\")\n\n";
    out << "set(COMDARE_PERMUTATION_COMPOSITIONS\n";
    for (auto const* d : selected) {
        out << "    \"" << d->cpp_type_name << "|" << d->header_include << "\"\n";
    }
    out << ")\n";

    return static_cast<bool>(out);
}

}  // namespace comdare::cache_engine::builder::codegen_tool
