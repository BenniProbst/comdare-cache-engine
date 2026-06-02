// test_genus_binding — Gattungs-Generik (2026-06-02, User-Option-B Schritt 2) — die Bau-Brücke ist
// gattungs-PARAMETRISCH (GenusBindingTraits<G>), SearchAlgorithm = verifizierter Spezialfall, andere Gattungen
// (Container/queuing als Adapter/Sequence, Graph) sind definierte, noch-ungebundene Erweiterungspunkte.
//
// Build: cl /std:c++latest /EHsc /I<…> (leicht: composition_factory + search_algorithm_anatomy, keine 22 Registries)

#include "builder/experiment_tree/genus_binding_traits.hpp"

#include <iostream>
#include <string>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace cea = comdare::cache_engine::anatomy;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "Gattungs-Generik (Schritt 2): GenusBindingTraits<G> — Bau-Brücke gattungs-parametrisch:\n";

    using SA = ex::GenusBindingTraits<cea::AnatomyGenus::SearchAlgorithm>;
    check_eq("SearchAlgorithm: slot_count == 17", SA::slot_count, std::size_t{17});
    check_eq("SearchAlgorithm: name", std::string{SA::name}, std::string{"SearchAlgorithm"});
    check_eq("SearchAlgorithm: axis_names() size == 17", SA::axis_names().size(), std::size_t{17});
    check_eq("axis_names[0] == search_algo", std::string{SA::axis_names()[0]}, std::string{"search_algo"});
    check_eq("axis_names[16] == filter", std::string{SA::axis_names()[16]}, std::string{"filter"});

    // Die Generik: SearchAlgorithm ist GEBUNDEN; die übrigen Gattungen sind definierte Erweiterungspunkte (noch nicht).
    check_true("GenusBound<SearchAlgorithm> == true (verifizierte Bau-Brücke)", ex::GenusBound<cea::AnatomyGenus::SearchAlgorithm>);
    // KORREKTUR 2026-06-02 (Audit aa02ec9): Adapter ist seit Schritt 3 (GenusBindingTraits<Adapter>) GEBUNDEN.
    // Die frühere Assert „Adapter==false" war nach dem Hinzufügen der Spezialisierung stale → jetzt korrekt true.
    check_true("GenusBound<Adapter> == true (Container/queuing seit Schritt 3 gebunden)", ex::GenusBound<cea::AnatomyGenus::Adapter>);
    check_true("GenusBound<Sequence> == false (NICHT gebunden — offener Erweiterungspunkt)", !ex::GenusBound<cea::AnatomyGenus::Sequence>);
    check_true("GenusBound<Set> == false (NICHT gebunden — offener Erweiterungspunkt)", !ex::GenusBound<cea::AnatomyGenus::Set>);
    check_true("GenusBound<View> == false (NICHT gebunden — offener Erweiterungspunkt)", !ex::GenusBound<cea::AnatomyGenus::View>);
    std::cout << "    Gattungs-Bindung: 2 von 5 gebunden (SearchAlgorithm + Adapter); Set/Sequence/View OFFEN\n";

    // Statische Bindungs-Identität: CompositionFor<PermTuple> ist eine AdHocComposition (BR-2 belegt die Materialisierung;
    // hier nur, dass die Traits den richtigen Composition-Typ binden). Konstruiere ein PermTuple<17> aus 17 void-Markern? —
    // nicht nötig: CompositionFromPermTuple<PermTuple<…17…>> ist via BR-2 verifiziert; das Traits delegiert nur darauf.
    std::cout << "    (CompositionFor/AnatomyFor delegieren auf die BR-2/BR-3-verifizierten Bindungen)\n";

    std::cout << "\n==== Gattungs-Generik (GenusBindingTraits): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
