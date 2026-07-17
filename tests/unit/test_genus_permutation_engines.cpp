// test_genus_permutation_engines — L-76a-c (2026-06-03): die per-Gattung PermutationEngine-Spezialisierungen für
// Set/Sequence/View (analog SearchAlgorithmPermutationEngine, Doku 14 §29.2 + §32). Beweist auf TYP-Ebene (ohne
// Organ-API-Abhängigkeit — das Instanz-Treiben ist separat durch die Docks belegt, test_genus_docks):
//   1. Composition-Factory: PermTuple<V...> → GenusComposition mit korrekten Slots (INC-2c: Set=14/Sequence=10/View=6).
//   2. Genus-Marker (engine::genus == AnatomyGenus::Set/Sequence/View) + arity() == Gattungs-Arität.
//   3. Cartesian-count() == ∏ mp_size(StaticAxisVariants) + for_each_composition_type besucht ALLE Punkte.
//   4. Jede materialisierte Composition erfüllt IsGenusComposition.
// Build: cl /I libs/cache_engine + libs/cache_engine/src + Boost::mp11 (PermutationEngine nutzt mp11).

#include "anatomy/set_permutation_engine.hpp"
#include "anatomy/sequence_permutation_engine.hpp"
#include "anatomy/view_permutation_engine.hpp"
#include "anatomy/anatomy_base.hpp"

#include <src/permutations/permutation_engine.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>

namespace ana = comdare::cache_engine::anatomy;
namespace pe  = comdare::cache_engine::permutations;
namespace mp  = boost::mp11;

static int g_fail = 0;
template <class A, class B>
static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << " (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// ── Dummy-Achsen-Varianten (die Permutations-Maschinerie schiebt nur Typen in die Composition-Slots; die
//    IsGenusComposition-Concepts prüfen die named Aliase, NICHT die Organ-API → Dummy-Typen genügen) ──
struct V_a1 {};
struct V_a2 {};
struct V_a3 {}; // 3 Varianten für Slot 0
struct V_b1 {};
struct V_b2 {}; // 2 Varianten für Slot 1
struct V_x {};  // Single-Filler

// TopicConfigSets (Pflicht-Interface: StaticAxisVariants = mp_list<...>).
struct Cfg3 {
    using StaticAxisVariants = mp::mp_list<V_a1, V_a2, V_a3>;
}; // 3
struct Cfg2 {
    using StaticAxisVariants = mp::mp_list<V_b1, V_b2>;
}; // 2
struct Cfg1 {
    using StaticAxisVariants = mp::mp_list<V_x>;
}; // 1 (Filler)

// ── Set-Engine: 14 Slots, Slot0=3 × Slot1=2 × 1^12 = 6 (INC-2c: telemetry raus) ──
using SetEngine =
    ana::SetPermutationEngine<Cfg3, Cfg2, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1>;
// ── Sequence-Engine: 10 Slots, Slot0=2 × 1^9 = 2 (INC-2c) ──
using SeqEngine = ana::SequencePermutationEngine<Cfg2, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1>;
// ── View-Engine: 6 Slots, Slot0=3 × 1^5 = 3 (INC-2c) ──
using ViewEngine = ana::ViewPermutationEngine<Cfg3, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1>;

// Compile-Time-Marker + Arität
static_assert(SetEngine::genus == ana::AnatomyGenus::Set);
static_assert(SeqEngine::genus == ana::AnatomyGenus::Sequence);
static_assert(ViewEngine::genus == ana::AnatomyGenus::View);
static_assert(SetEngine::arity() == 14);
static_assert(SeqEngine::arity() == 10);
static_assert(ViewEngine::arity() == 6);
static_assert(SetEngine::count() == 6, "3 × 2 × 1^12 = 6");
static_assert(SeqEngine::count() == 2, "2 × 1^9 = 2");
static_assert(ViewEngine::count() == 3, "3 × 1^5 = 3");

// Factory-Materialisierung direkt (synthetisches PermTuple → korrekte Slots)
using SetPerm = pe::PermTuple<V_a1, V_b1, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x>;
using SetComp = ana::SetCompositionFromPermTuple<SetPerm>;
static_assert(ana::IsSetComposition<SetComp>);
static_assert(std::is_same_v<SetComp::search_algo, V_a1>);     // Slot 0
static_assert(std::is_same_v<SetComp::cache_traversal, V_b1>); // Slot 1
static_assert(std::is_same_v<SetComp::filter, V_x>);           // Slot 13 (letzter, INC-2c)
static_assert(SetComp::slot_count == 14);

using SeqPerm = pe::PermTuple<V_a1, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_b2>;
using SeqComp = ana::SequenceCompositionFromPermTuple<SeqPerm>;
static_assert(ana::IsSequenceComposition<SeqComp>);
static_assert(std::is_same_v<SeqComp::memory_layout, V_a1>); // Slot 0
static_assert(std::is_same_v<SeqComp::growth_policy, V_b2>); // Slot 9 (axis_growth, überschreibt Default; INC-2c)
static_assert(SeqComp::slot_count == 10);

using ViewPerm = pe::PermTuple<V_a2, V_x, V_x, V_x, V_x, V_b1>;
using ViewComp = ana::ViewCompositionFromPermTuple<ViewPerm>;
static_assert(ana::IsViewComposition<ViewComp>);
static_assert(std::is_same_v<ViewComp::memory_layout, V_a2>);   // Slot 0
static_assert(std::is_same_v<ViewComp::accessor_policy, V_b1>); // Slot 5 (axis_accessor, überschreibt Default; INC-2c)
static_assert(ViewComp::slot_count == 6);

int main() {
    std::cout << "==== L-76 per-Gattung PermutationEngines (Doku 14 §29.2): Set / Sequence / View ====\n";

    // ── Set ──
    std::cout << "\n-- SetPermutationEngine (Vogel) --\n";
    tr("genus == Set", SetEngine::genus == ana::AnatomyGenus::Set);
    eq("arity() == 14", SetEngine::arity(), std::size_t{14});
    eq("count() == 6 (3×2×1^12)", SetEngine::count(), std::size_t{6});
    {
        std::size_t visited     = 0;
        bool        all_conform = true;
        SetEngine::for_each_composition_type([&]<class C>() {
            ++visited;
            if (!ana::IsSetComposition<C>) all_conform = false;
        });
        eq("for_each_composition_type besucht 6", visited, std::size_t{6});
        tr("jede Composition erfüllt IsSetComposition", all_conform);
    }

    // ── Sequence ──
    std::cout << "\n-- SequencePermutationEngine (Reptil) --\n";
    tr("genus == Sequence", SeqEngine::genus == ana::AnatomyGenus::Sequence);
    eq("arity() == 10", SeqEngine::arity(), std::size_t{10});
    eq("count() == 2 (2×1^9)", SeqEngine::count(), std::size_t{2});
    {
        std::size_t visited     = 0;
        bool        all_conform = true;
        SeqEngine::for_each_composition_type([&]<class C>() {
            ++visited;
            if (!ana::IsSequenceComposition<C>) all_conform = false;
        });
        eq("for_each_composition_type besucht 2", visited, std::size_t{2});
        tr("jede Composition erfüllt IsSequenceComposition", all_conform);
    }

    // ── View ──
    std::cout << "\n-- ViewPermutationEngine (Pflanze) --\n";
    tr("genus == View", ViewEngine::genus == ana::AnatomyGenus::View);
    eq("arity() == 6", ViewEngine::arity(), std::size_t{6});
    eq("count() == 3 (3×1^5)", ViewEngine::count(), std::size_t{3});
    {
        std::size_t visited     = 0;
        bool        all_conform = true;
        ViewEngine::for_each_composition_type([&]<class C>() {
            ++visited;
            if (!ana::IsViewComposition<C>) all_conform = false;
        });
        eq("for_each_composition_type besucht 3", visited, std::size_t{3});
        tr("jede Composition erfüllt IsViewComposition", all_conform);
    }

    std::cout << "\n==== L-76 per-Gattung PermutationEngines: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
