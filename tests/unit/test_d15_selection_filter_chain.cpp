// D15 (Doc 20 §C) — CoR-Filterkette im CEB: die mess-getriebene Reduktionsstufe S4 schliesst die Feedback-Kante
// Auswertung -> Generierung. Beweis-Kern: aus Kandidaten + Messergebnissen (MeasurementRow) entsteht eine
// reduzierte BuildSelection; der ResumeFilter wirft bereits-valide-gemessene Permutationen aus dem Bau-Set,
// unmess/invalide ueberleben. Header-only, RAM-leicht, keine Cluster-Daten (Mock-Kandidaten).
//
// Build: cl /std:c++latest /EHsc test_d15_selection_filter_chain.cpp /I libs/cache_engine

#include "builder/experiment_tree/selection_filter_chain.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace comdare::cache_engine::builder::experiment;
namespace bb = comdare::cache_engine::best_binary;

static int  g_fail = 0;
static void check(bool cond, std::string const& msg) {
    std::cout << (cond ? "  [OK]  " : "  [FAIL] ") << msg << "\n";
    if (!cond) ++g_fail;
}

[[nodiscard]] static bool contains(std::vector<std::size_t> const& v, std::size_t x) {
    for (std::size_t e : v)
        if (e == x) return true;
    return false;
}

[[nodiscard]] static PermutationCandidate valid_measured(std::size_t idx, std::string const& id) {
    bb::MeasurementRow r;
    r.binary_id       = id;
    r.two_phase_valid = true;
    r.ns_per_op       = 42.0;
    return {idx, id, std::optional<bb::MeasurementRow>{r}};
}
[[nodiscard]] static PermutationCandidate invalid_measured(std::size_t idx, std::string const& id) {
    bb::MeasurementRow r;
    r.binary_id       = id;
    r.two_phase_valid = false; // Messung fehlgeschlagen -> Neubau/Neumessung noetig
    return {idx, id, std::optional<bb::MeasurementRow>{r}};
}
[[nodiscard]] static PermutationCandidate unmeasured(std::size_t idx, std::string const& id) {
    return {idx, id, std::nullopt};
}

int main() {
    std::cout << "==== D15 CoR-Filterkette (Doc 20 §C) ====\n";

    std::vector<PermutationCandidate> const candidates = {
        valid_measured(10u, "A"),   // -> Reject (resume-skip)
        invalid_measured(11u, "B"), // -> ueberlebt (invalide -> bauen)
        unmeasured(12u, "C"),       // -> ueberlebt (unmess -> bauen)
        valid_measured(13u, "D"),   // -> Reject
    };

    // (1) ResumeFilter-Verdikt je Fall (Handler-Ebene).
    ResumeFilter rf;
    check(rf.dispatch(candidates[0]).decision == FilterVerdict::Decision::Reject, "valide-gemessen -> Reject");
    check(rf.dispatch(candidates[1]).decision == FilterVerdict::Decision::PassToNext, "invalide -> PassToNext");
    check(rf.dispatch(candidates[2]).decision == FilterVerdict::Decision::PassToNext, "unmess -> PassToNext");

    // (2) Ketten-Runner: reduzierte BuildSelection (Ueberlebende = dispatch != Reject).
    std::vector<FilterHandler*> chain = {&rf};
    BuildSelection const        out   = run_selection_filter_chain(candidates, chain, "one_wise");
    check(out.size() == 2, "reduzierte Selektion: 2 Ueberlebende (11,12) von 4");
    check(!contains(out.indices, 10u) && !contains(out.indices, 13u), "valide-gemessene (10,13) verworfen");
    check(contains(out.indices, 11u) && contains(out.indices, 12u), "unmess/invalide (11,12) behalten");
    check(out.provenance == "one_wise|filtered:cor", "provenance fortgeschrieben (kein stilles Truncaten)");

    // (3) Survive-Regel-Rand: leere Kette = Identitaet (alle ueberleben).
    BuildSelection const id_out = run_selection_filter_chain(candidates, {}, "");
    check(id_out.size() == 4, "leere Kette -> Identitaet (alle 4 ueberleben)");
    check(id_out.provenance == "filtered:cor", "provenance-Default gesetzt");

    // (4) Kandidatenlose Runde -> leere, aber ehrlich provenienzierte Selektion.
    BuildSelection const empty_out = run_selection_filter_chain({}, chain, "explicit");
    check(empty_out.empty() && empty_out.provenance == "explicit|filtered:cor", "leere Kandidatenmenge -> leer");

    std::cout << "\n==== D15: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
