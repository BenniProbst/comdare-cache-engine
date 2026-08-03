// test_genus_permutation_engines — L-76a-c (2026-06-03): die per-Gattung PermutationEngine-Spezialisierungen für
// Set/Sequence/View (analog SearchAlgorithmPermutationEngine, Doku 14 §29.2 + §32). Beweist auf TYP-Ebene (ohne
// Organ-API-Abhängigkeit — das Instanz-Treiben ist separat durch die Docks belegt, test_genus_docks):
//   1. Composition-Factory: PermTuple<V...> → GenusComposition mit korrekten Slots (INC-2c: Set=14/Sequence=10/View=6).
//   2. Genus-Marker (engine::genus == AnatomyGenus::Set/Sequence/View) + arity() == Gattungs-Arität.
//   3. Cartesian-count() == ∏ mp_size(StaticAxisVariants) + for_each_composition_type besucht ALLE Punkte.
//   4. Jede materialisierte Composition erfüllt IsGenusComposition.
// Build: cl /I libs/cache_engine + libs/cache_engine/src + Boost::mp11 (PermutationEngine nutzt mp11).
//
// FORTSCHREIBUNG E-24 C2 (2026-08-04, Auflage 13 "Bestands-Tests FORTSCHREIBEN statt umgehen" + Memory-Lehre
// "gruene Tests zementieren alte Ordnung"): diese TU deckte bis heute DREI der vier Container-Engines ab und
// haette damit den Zustand "Adapter hat als einzige Gattung keine eigene Engine" still gruen zementiert. Mit
// dem OP-3-Entscheid (Bauplan-Dossier 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 1.3) existiert
// anatomy/adapter_permutation_engine.hpp -- sie ist hier NACHGEZOGEN, auf derselben TYP-Ebene und mit
// denselben Fragen wie ihre drei Geschwister.
// Zusaetzlich traegt die TU jetzt eine STRUKTUR-Wache: die Zahl der hier geprueften Container-Engines wird
// gegen comdare::container::type_count gekreuzt. Kommt je ein weiterer Container-TYP dazu, bricht diese TU,
// bis er eine eigene per-Gattung-Engine UND eine Zeile hier hat -- statt still unvollstaendig gruen zu sein.
// Die ABI-Materialisierung (for_each_abi_adapter) gehoert bewusst NICHT hierher: sie braucht echte Organe am
// getriebenen Slot und liegt je Engine in einer eigenen TU (tests/unit/test_e24_c2_*_permutation_engine.cpp).

#include "anatomy/set_permutation_engine.hpp"
#include "anatomy/sequence_permutation_engine.hpp"
#include "anatomy/view_permutation_engine.hpp"
#include "anatomy/adapter_permutation_engine.hpp" // E-24 C2 (OP-3): die vierte Container-Engine
#include "anatomy/container_framework.hpp"        // E-24 C2: comdare::container::type_count (Struktur-Wache)
#include "anatomy/anatomy_base.hpp"

#include <src/permutations/permutation_engine.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>

namespace ana = comdare::cache_engine::anatomy;
namespace cco = comdare::container; // E-24 C2: Gattungs-Slot-Pins (Struktur-Wache)
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

// ── Set-Engine: 13 Slots, Slot0=3 × Slot1=2 × 1^11 = 6 (INC-2c telemetry / INC-2d isa raus) ──
using SetEngine =
    ana::SetPermutationEngine<Cfg3, Cfg2, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1>;
// ── Sequence-Engine: 9 Slots, Slot0=2 × 1^8 = 2 (INC-2d) ──
using SeqEngine = ana::SequencePermutationEngine<Cfg2, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1>;
// ── View-Engine: 5 Slots, Slot0=3 × 1^4 = 3 (INC-2d) ──
using ViewEngine = ana::ViewPermutationEngine<Cfg3, Cfg1, Cfg1, Cfg1, Cfg1>;
// -- Adapter-Engine (E-24 C2 / OP-3, nachgezogen): 11 Slots, Slot0=3 x Slot1=2 x 1^9 = 6 --
using AdapterEngine = ana::AdapterPermutationEngine<Cfg3, Cfg2, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1, Cfg1>;

// Compile-Time-Marker + Arität
static_assert(SetEngine::genus == ana::AnatomyGenus::Set);
static_assert(SeqEngine::genus == ana::AnatomyGenus::Sequence);
static_assert(ViewEngine::genus == ana::AnatomyGenus::View);
static_assert(AdapterEngine::genus == ana::AnatomyGenus::Adapter);
static_assert(SetEngine::arity() == 13);
static_assert(SeqEngine::arity() == 9);
static_assert(ViewEngine::arity() == 5);
static_assert(AdapterEngine::arity() == 11);
static_assert(SetEngine::count() == 6, "3 × 2 × 1^11 = 6");
static_assert(SeqEngine::count() == 2, "2 × 1^8 = 2");
static_assert(ViewEngine::count() == 3, "3 × 1^4 = 3");
static_assert(AdapterEngine::count() == 6, "3 x 2 x 1^9 = 6");

// -- E-24 C2 STRUKTUR-WACHE: so viele per-Gattung-Engines wie Container-TYPEN (4/4, kein stiller Rest) --
// Die vier Aritaeten muessen den vier Bau-Bindungen folgen (container_framework.hpp:93-96); waere eine
// Engine an eine falsche Slot-Zahl gebunden, faellt es hier auf und nicht erst im Voll-Bau.
inline constexpr std::size_t kContainerEnginesUnderTest = 4;
static_assert(cco::type_count == kContainerEnginesUnderTest,
              "E-24 C2 / OP-3-Kanon: JEDER Container-TYP traegt eine eigene per-Gattung-PermutationEngine. "
              "Kommt ein Typ dazu, braucht er eine Engine UND eine Zeile in dieser TU.");
static_assert(SetEngine::arity() == cco::type_traits<ana::AnatomyGenus::Set>::slot_count);
static_assert(SeqEngine::arity() == cco::type_traits<ana::AnatomyGenus::Sequence>::slot_count);
static_assert(ViewEngine::arity() == cco::type_traits<ana::AnatomyGenus::View>::slot_count);
static_assert(AdapterEngine::arity() == cco::type_traits<ana::AnatomyGenus::Adapter>::slot_count);

// Factory-Materialisierung direkt (synthetisches PermTuple → korrekte Slots)
using SetPerm = pe::PermTuple<V_a1, V_b1, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x>;
using SetComp = ana::SetCompositionFromPermTuple<SetPerm>;
static_assert(ana::IsSetComposition<SetComp>);
static_assert(std::is_same_v<SetComp::search_algo, V_a1>);     // Slot 0
static_assert(std::is_same_v<SetComp::cache_traversal, V_b1>); // Slot 1
static_assert(std::is_same_v<SetComp::filter, V_x>);           // Slot 12 (letzter, INC-2d)
static_assert(SetComp::slot_count == 13);

using SeqPerm = pe::PermTuple<V_a1, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_b2>;
using SeqComp = ana::SequenceCompositionFromPermTuple<SeqPerm>;
static_assert(ana::IsSequenceComposition<SeqComp>);
static_assert(std::is_same_v<SeqComp::memory_layout, V_a1>); // Slot 0
static_assert(std::is_same_v<SeqComp::growth_policy, V_b2>); // Slot 8 (axis_growth, überschreibt Default; INC-2d)
static_assert(SeqComp::slot_count == 9);

using ViewPerm = pe::PermTuple<V_a2, V_x, V_x, V_x, V_b1>;
using ViewComp = ana::ViewCompositionFromPermTuple<ViewPerm>;
static_assert(ana::IsViewComposition<ViewComp>);
static_assert(std::is_same_v<ViewComp::memory_layout, V_a2>);   // Slot 0
static_assert(std::is_same_v<ViewComp::accessor_policy, V_b1>); // Slot 4 (axis_accessor, überschreibt Default; INC-2d)
static_assert(ViewComp::slot_count == 5);

// E-24 C2 (OP-3): dieselbe Factory-Frage an die vierte Engine.
using AdapterPerm = pe::PermTuple<V_a1, V_b1, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_x, V_b2>;
using AdapterComp = ana::AdapterCompositionFromPermTuple<AdapterPerm>;
static_assert(ana::IsAdapterComposition<AdapterComp>);
static_assert(std::is_same_v<AdapterComp::search_algo, V_a1>);     // Slot 0 (geteilt/delegiert)
static_assert(std::is_same_v<AdapterComp::cache_traversal, V_b1>); // Slot 1 (geteilt/delegiert)
static_assert(std::is_same_v<AdapterComp::inner_container, V_b2>); // Slot 10 (die spezifische Achse)
static_assert(AdapterComp::slot_count == 11);

int main() {
    std::cout << "==== L-76 per-Gattung PermutationEngines (Doku 14 §29.2): Set / Sequence / View ====\n";

    // ── Set ──
    std::cout << "\n-- SetPermutationEngine (Vogel) --\n";
    tr("genus == Set", SetEngine::genus == ana::AnatomyGenus::Set);
    eq("arity() == 13", SetEngine::arity(), std::size_t{13});
    eq("count() == 6 (3×2×1^11)", SetEngine::count(), std::size_t{6});
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
    eq("arity() == 9", SeqEngine::arity(), std::size_t{9});
    eq("count() == 2 (2×1^8)", SeqEngine::count(), std::size_t{2});
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
    eq("arity() == 5", ViewEngine::arity(), std::size_t{5});
    eq("count() == 3 (3×1^4)", ViewEngine::count(), std::size_t{3});
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

    // -- Adapter (E-24 C2 / OP-3, nachgezogen) --
    std::cout << "\n-- AdapterPermutationEngine (Wirbelloses; E-24 C2 / OP-3 nachgezogen) --\n";
    tr("genus == Adapter", AdapterEngine::genus == ana::AnatomyGenus::Adapter);
    tr("gattung == Container (Ebene-1-Aussen-Interface)", AdapterEngine::gattung == ana::AnatomyGattung::Container);
    eq("arity() == 11", AdapterEngine::arity(), std::size_t{11});
    eq("count() == 6 (3x2x1^9)", AdapterEngine::count(), std::size_t{6});
    {
        std::size_t visited     = 0;
        bool        all_conform = true;
        AdapterEngine::for_each_composition_type([&]<class C>() {
            ++visited;
            if (!ana::IsAdapterComposition<C>) all_conform = false;
        });
        eq("for_each_composition_type besucht 6", visited, std::size_t{6});
        tr("jede Composition erfuellt IsAdapterComposition", all_conform);
    }

    // -- Struktur-Wache: 4 Container-TYPEN, 4 per-Gattung-Engines (kein stiller Rest) --
    std::cout << "\n-- Struktur-Wache (E-24 C2): Container-TYPEN vs. per-Gattung-Engines --\n";
    eq("comdare::container::type_count", cco::type_count, kContainerEnginesUnderTest);
    tr("alle vier Engine-Aritaeten folgen ihrer Bau-Bindung (13/9/5/11)",
       SetEngine::arity() == cco::type_traits<ana::AnatomyGenus::Set>::slot_count &&
           SeqEngine::arity() == cco::type_traits<ana::AnatomyGenus::Sequence>::slot_count &&
           ViewEngine::arity() == cco::type_traits<ana::AnatomyGenus::View>::slot_count &&
           AdapterEngine::arity() == cco::type_traits<ana::AnatomyGenus::Adapter>::slot_count);

    std::cout << "\n==== L-76 per-Gattung PermutationEngines: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
