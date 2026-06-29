// gen_golden_fullpilot — STRANG A KORRIGIERT, Increment 4 / S4a (2026-06-18). EINMAL-GENERATOR der GOLDEN-REFERENZ.
//
// Schreibt die 320 binary_ids des heutigen Code-/Katalog-Pfads (catalog_static_levels<FullSourceCatalog> →
// StaticBinaryView) als committete golden-Liste (golden_fullpilot_320_binary_ids.txt). Diese Liste IST ab jetzt die
// Regressions-Referenz: test_profile_roundtrip vergleicht den PROFIL-Pfad gegen DIESE Datei (statt gegen den zu
// entfernenden Live-Code-Pfad). So überlebt die Round-Trip-Gate (Resume #139-Schutz) die Entfernung der Code-Selektion.
//
// Aufruf: ein einziges Mal (S4a), das Ergebnis wird committet. Danach wird die Datei NICHT neu generiert (sonst wäre
// die Gate zirkulär) — sie ist die eingefrorene Golden-Referenz.
//
// Build (innerhalb vcvars64, Include-Satz wie thesis_tiere-Harness):
//   cl /std:c++latest /EHsc /Od /bigobj <inc> gen_golden_fullpilot.cpp /Fe:gen_golden.exe
//   gen_golden.exe <out_txt>

#include "source_catalog.hpp" // FullSourceCatalog / catalog_static_levels

#include <builder/experiment_tree/experiment_tree.hpp> // ExperimentTree / StaticBinaryView

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

int main(int argc, char** argv) {
    fs::path out =
        (argc >= 2) ? fs::path(argv[1]) : fs::path("tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt");

    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(tlz::catalog_static_levels<tlz::FullSourceCatalog>()); // nur statisch → reine binary_id-Quelle
    ex::StaticBinaryView const view = tree.static_binary_view();

    std::vector<std::string> ids;
    ids.reserve(view.size());
    for (std::size_t i = 0; i < view.size(); ++i) ids.push_back(view[i].binary_id);

    std::cout << "FullSourceCatalog binary_count = " << ids.size() << "\n";
    if (ids.size() != 320) {
        std::cerr << "ERWARTET 320 binary_ids (4*4*5*4), bekam " << ids.size() << " — ABBRUCH (keine golden-Datei).\n";
        return 1;
    }

    std::ofstream f{out, std::ios::trunc};
    if (!f) {
        std::cerr << "kann golden-Datei nicht schreiben: " << out.string() << "\n";
        return 2;
    }
    f << "# GOLDEN-REFERENZ: 320 binary_ids des FullSourceCatalog (4*4*5*4) — STRANG A Inc4/S4a (2026-06-18).\n";
    f << "# EINGEFROREN. test_profile_roundtrip vergleicht den PROFIL-Pfad gegen DIESE Liste (Resume #139-Schutz).\n";
    f << "# Eine Zeile = ein binary_id (positions-getreue Reihenfolge des StaticBinaryView). NICHT neu generieren.\n";
    for (auto const& id : ids) f << id << "\n";
    f.close();

    std::cout << "golden geschrieben: " << out.string() << "  (" << ids.size() << " binary_ids)\n";
    std::cout << "  golden[0]   = " << ids.front() << "\n";
    std::cout << "  golden[319] = " << ids.back() << "\n";
    return 0;
}
