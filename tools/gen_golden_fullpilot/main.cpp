// gen_golden_fullpilot — autoritativer Golden-Fixture-Generator (Bau-INC-2c/2h, BAUPLAN-INC2 §5).
//
// Materialisiert die binary_ids des FullSourceCatalog (4·4·5·4 = 320) über EXAKT denselben offiziellen
// Pfad, den die Roundtrip-/Limits-Tests fahren (catalog_static_levels<FullSourceCatalog> →
// ExperimentTree.build → StaticBinaryView, positions-getreu) und schreibt sie als eingefrorene
// Golden-Referenz. Seit Bau-INC-2c (F12iii, ABI-5) tragen die Pfade KEIN telemetry-Segment mehr
// (18 Slots; Telemetrie ist CEB-System-Achse im H-10-Sidecar). Die ABI-4-Historie bleibt additiv
// als golden_fullpilot_320_binary_ids_abi4.txt eingefroren (Messdaten nie löschen).
//
// Aufruf: gen_golden_fullpilot <ausgabe.txt>

#include <profile_facade/source_catalog.hpp>

#include <builder/experiment_tree/experiment_tree.hpp>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Aufruf: gen_golden_fullpilot <ausgabe.txt>\n";
        return 2;
    }

    std::vector<ex::AxisLevel> const levels = tlz::catalog_static_levels<tlz::FullSourceCatalog>();

    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);
    ex::StaticBinaryView const view = tree.static_binary_view();

    std::ofstream out{argv[1], std::ios::trunc};
    if (!out) {
        std::cerr << "FEHLER: kann '" << argv[1] << "' nicht schreiben\n";
        return 3;
    }
    out << "# GOLDEN-REFERENZ: " << view.size()
        << " binary_ids des FullSourceCatalog (4*4*5*4) — ABI-5 / Bau-INC-2c (F12iii: 18 Slots, ohne telemetry-"
           "Segment).\n"
        << "# EINGEFROREN. test_profile_roundtrip vergleicht den PROFIL-Pfad gegen DIESE Liste (Resume #139-Schutz).\n"
        << "# Eine Zeile = ein binary_id (positions-getreue Reihenfolge des StaticBinaryView). Regeneration NUR über\n"
        << "# gen_golden_fullpilot (tools/gen_golden_fullpilot) im koordinierten ABI-Fenster; ABI-4-Historie additiv\n"
        << "# in golden_fullpilot_320_binary_ids_abi4.txt.\n";
    for (std::size_t i = 0; i < view.size(); ++i) out << view[i].binary_id << "\n";

    std::cout << "gen_golden_fullpilot: " << view.size() << " ids -> " << argv[1] << "\n";
    return view.size() == 320 ? 0 : 4;
}
