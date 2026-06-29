#pragma once
// D14 / L-CLUSTER-E2E (gate-frei, 2026-06-02) — e2e_pipeline: die durchgängige LOKALE Mess-Pipeline, die die
// schon gebauten Bausteine verkettet (Doc 28 §5 „lokal zählen / Cluster bauen / Dock messen"):
//   StaticBinaryView (lazy, ∏) → BuildSelection (endliche K Indizes) → je Binary: MeasureFn(binary_id)
//   → result_ingest → ExperimentTree-NodeValue (sparse).
//
// O(K) — NIE über die ganze ∏-View (Doc 26 §2). Der MESS-SCHRITT ist INJIZIERBAR (MeasureFn): im echten Pfad =
// Source generieren (ceb_generator/adhoc_emitter) → DLL kompilieren (BuildOrchestrator) → laden (AnatomyModuleLoader)
// → treiben (perm_runner) → result-Zeile; deterministisch testbar via Mock-MeasureFn. Echte DLL-Mess-Strecke ist
// separat belegt (test_dgenus_dll/test_d4b/test_d14b). Header-only, C++23.

#include "experiment_tree.hpp" // StaticBinaryView / BinarySpec / ExperimentTree
#include "result_ingest.hpp"   // ingest_result_line

#include <cstddef>
#include <functional>
#include <span>
#include <string>

namespace comdare::cache_engine::builder::experiment {

/// MeasureFn: bildet eine binary_id auf eine result_ingest-Zeile ab. Echt = perm_runner über geladene DLL;
/// Test = deterministisch. Leerer Rückgabe-String → Binary übersprungen (Bau-/Lade-Fehler), kein Baum-Eintrag.
using MeasureFn = std::function<std::string(std::string const& binary_id)>;

/// Ergebnis der durchgängigen Pipeline (rein zählend — kein ∏-Vektor).
struct E2EPipelineStats {
    std::size_t selected = 0; // Zahl der selektierten Binaries (== selection.size())
    std::size_t measured = 0; // erfolgreich gemessen + in den Baum eingespielt
    std::size_t skipped  = 0; // MeasureFn lieferte leer / Index außerhalb
};

/// Fährt die Pipeline über die SELEKTIERTEN View-Indizes (NICHT die ganze ∏-View): je Index → BinarySpec →
/// MeasureFn(binary_id) → result_ingest → Baum-NodeValue. O(K=selection.size()). Liefert die Statistik.
[[nodiscard]] inline E2EPipelineStats run_e2e_pipeline(ExperimentTree& tree, StaticBinaryView const& view,
                                                       std::span<const std::size_t> selection, MeasureFn measure) {
    E2EPipelineStats st;
    st.selected = selection.size();
    for (std::size_t j = 0; j < selection.size(); ++j) {
        std::size_t const i = selection[j];
        if (i >= view.size()) {
            ++st.skipped;
            continue;
        }
        BinarySpec const  spec = view[i]; // on-demand mixed-radix dekodiert (O(Tiefe))
        std::string const line = measure(spec.binary_id);
        if (!line.empty() && ingest_result_line(tree, line))
            ++st.measured;
        else
            ++st.skipped;
    }
    return st;
}

} // namespace comdare::cache_engine::builder::experiment
