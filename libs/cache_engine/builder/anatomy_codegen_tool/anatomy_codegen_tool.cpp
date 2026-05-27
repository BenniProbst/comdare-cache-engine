// V41.F.6.1.R5.F + R5.H + R5.J — anatomy_codegen_tool Library Implementation
//
// R5.H (2026-05-27 spaet): hardcoded Strings durch trait-driven
// descriptor_from_composition<C>() ersetzt.
// R5.J (2026-05-27 nacht): hardcoded std::array durch mp_for_each-Iteration
// ueber KnownReferenceCompositions mp_list ersetzt. Reine Compile-Time-
// Cartesian-Iteration; jede neue Composition wird durch Hinzufuegen eines
// Entry-Wrappers zur mp_list automatisch im Tool sichtbar.
//
// @task #714 V41.F.6.1.R5.J

#include "anatomy_codegen_tool.hpp"

// R5.J: zentrale mp_list aller Reference-Compositions (Entry-Wrapper-Pattern).
// Inkludiert transitiv alle 11 Composition-Header.
#include "../../compositions/known_compositions_list.hpp"

#include <boost/mp11.hpp>

#include <fstream>
#include <vector>

namespace comdare::cache_engine::builder::codegen_tool {

namespace comp = ::comdare::cache_engine::compositions;
namespace mp   = ::boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// kKnownCompositions — Init-on-first-use ueber KnownReferenceCompositions mp_list
// ─────────────────────────────────────────────────────────────────────────────
//
// Strategie: anstatt einer hardcoded std::array nutzen wir mp_for_each ueber
// die zentrale mp_list. Pro Entry extrahieren wir den CompositionDescriptor
// via descriptor_from_composition<C>() (R5.G Trait) und ueberschreiben nur
// den short_name aus dem Entry-Wrapper. Statisch initialisiert beim ersten
// Aufruf von known_compositions().

namespace {

[[nodiscard]] std::vector<CompositionDescriptor> const& known_compositions_storage() {
    static std::vector<CompositionDescriptor> const tbl = [] {
        std::vector<CompositionDescriptor> v;
        v.reserve(comp::kKnownReferenceCompositionsCount);
        mp::mp_for_each<comp::KnownReferenceCompositions>([&v]<class Entry>(Entry) {
            using C = typename Entry::composition;
            auto d = descriptor_from_composition<C>();
            d.short_name = Entry::short_name;  // Tool-spezifischer CLI-Override
            v.push_back(d);
        });
        return v;
    }();
    return tbl;
}

}  // anonymous namespace

std::span<CompositionDescriptor const> known_compositions() noexcept {
    auto const& tbl = known_compositions_storage();
    return std::span<CompositionDescriptor const>{tbl.data(), tbl.size()};
}

// ─────────────────────────────────────────────────────────────────────────────
// find_composition / select_compositions
// ─────────────────────────────────────────────────────────────────────────────

CompositionDescriptor const* find_composition(std::string_view short_name) noexcept {
    for (auto const& c : known_compositions_storage()) {
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
    auto const& tbl = known_compositions_storage();
    if (csv_names.empty()) {
        out.reserve(tbl.size());
        for (auto const& c : tbl) {
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
