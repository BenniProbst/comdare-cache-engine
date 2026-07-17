// Limits-Entkopplung Vorstufe: generierter Katalog bleibt zum bestehenden
// FullSourceCatalog positionsidentisch und liefert Quellen fuer alle Golden-IDs.

#include "generated_source_catalog.hpp"

#include <builder/experiment_tree/experiment_tree.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace fs  = ::std::filesystem;
namespace tlz = ::comdare::cache_engine::thesis_lazy;

namespace {

int g_fail = 0;

void check_true(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

template <class A, class B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

std::vector<std::string> load_golden(fs::path const& p) {
    std::vector<std::string> ids;
    std::ifstream            f{p};
    std::string              line;
    while (std::getline(f, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        ids.push_back(line);
    }
    return ids;
}

std::vector<std::string> binary_ids(std::vector<ex::AxisLevel> const& levels) {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);
    ex::StaticBinaryView const view = tree.static_binary_view();

    std::vector<std::string> ids;
    ids.reserve(view.size());
    for (std::size_t i = 0; i < view.size(); ++i) ids.push_back(view[i].binary_id);
    return ids;
}

void check_level_equivalence(std::vector<ex::AxisLevel> const& generated, std::vector<ex::AxisLevel> const& reference) {
    check_eq("Stufe 1: static_levels count", generated.size(), reference.size());
    check_eq("Stufe 1: static_levels count == 18", generated.size(), std::size_t{18});

    bool axes_ok   = generated.size() == reference.size();
    bool values_ok = axes_ok;
    for (std::size_t i = 0; i < generated.size() && i < reference.size(); ++i) {
        axes_ok   = axes_ok && (generated[i].axis == reference[i].axis) &&
                    (generated[i].is_static == reference[i].is_static) &&
                    (generated[i].block_id == reference[i].block_id);
        values_ok = values_ok && (generated[i].values == reference[i].values);
    }
    check_true("Stufe 1: Achsen-Metadaten positionsidentisch zu FullSourceCatalog", axes_ok);
    check_true("Stufe 1: Wertelisten positionsidentisch zu FullSourceCatalog", values_ok);
}

void check_golden(std::vector<std::string> const& generated_ids, std::vector<std::string> const& golden_ids) {
    check_eq("Stufe 2: generated binary_count", generated_ids.size(), std::size_t{320});
    check_eq("Stufe 2: golden binary_count", golden_ids.size(), std::size_t{320});

    std::size_t mismatches = 0;
    for (std::size_t i = 0; i < generated_ids.size() && i < golden_ids.size(); ++i)
        if (generated_ids[i] != golden_ids[i]) ++mismatches;
    check_eq("Stufe 2: Golden-Positionsvergleich Mismatch", mismatches, std::size_t{0});
    if (mismatches == 0 && generated_ids.size() == golden_ids.size() && generated_ids.size() == 320) {
        std::cout << "Golden-Positionsvergleich OK: 320 Positionen identisch\n";
    }
}

void check_source_gen(std::vector<std::string> const& golden_ids) {
    ex::SourceGenFn gen = tlz::generated_make_catalog_source_gen();

    std::size_t empty = 0;
    for (std::string const& id : golden_ids) {
        std::string const source = gen(id);
        if (source.empty()) ++empty;
    }
    check_eq("Stufe 3: leere Quellen fuer Golden-IDs", empty, std::size_t{0});
    check_true("Stufe 3: unbekannte ID liefert leere Quelle", gen("__unknown_binary_id__").empty());
}

} // namespace

int main(int argc, char** argv) {
    fs::path const golden_txt =
        (argc >= 2) ? fs::path(argv[1])
                    : (fs::path("tests") / "unit" / "thesis_tiere" / "golden_fullpilot_320_binary_ids.txt");

    std::cout << "==== Limits-Entkopplung Vorstufe ====\n";
    std::cout << "Golden: " << golden_txt.string() << "\n";

    std::vector<ex::AxisLevel> const generated = tlz::generated_catalog_static_levels();
    std::vector<ex::AxisLevel> const reference = tlz::catalog_static_levels<tlz::FullSourceCatalog>();
    check_level_equivalence(generated, reference);

    std::vector<std::string> const golden_ids    = load_golden(golden_txt);
    std::vector<std::string> const generated_ids = binary_ids(generated);
    check_golden(generated_ids, golden_ids);

    if (golden_ids.size() == 320) check_source_gen(golden_ids);

    std::cout << "\n==== Limits-Entkopplung Vorstufe: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
