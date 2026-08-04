// test_e24_c2_adapter_permutation_engine -- E-24 C2 (Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C2 + OP-3-ENTSCHEID Paragraf 1.3): die Test-TU der
// NEUEN ADAPTER-Engine (anatomy/adapter_permutation_engine.hpp).
//
// GATE (Bauplan Paragraf 4.2, Klasse II, wortwoertlich fuer C2):
//   "je Engine: for_each_abi_adapter-Materialisierungs-Zaehlung == Slot-Permutations-Soll des Genus"
// Fixture-unabhaengiger Ableitungsweg (Bauplan Paragraf 4.3): das Soll ist das mp_size-Produkt der
// TopicConfigSets, gekreuzt mit der Gattungs-Bau-Bindung (comdare::container::type_traits<Adapter>::
// slot_count == 11, container_framework.hpp:93). Keine handgepflegte Zahl.
//
// Der permutierte Slot ist der REAL getriebene: inner_container -- die EINE Adapter-spezifische Achse
// (Doku 14 Paragraf 28; adapter_anatomy.hpp:31-97). Alle drei Bestands-Substrate laufen mit:
// DequeInner (stack/queue-Default), VectorInner (kontiguierlich), HeapInner (echte priority_queue-
// Disziplin). Die Erwartungen sind bewusst DISZIPLIN-INVARIANT formuliert -- HeapInner liefert bei
// pop_front das MAXIMUM, DequeInner das VORDERSTE Element; geprueft werden deshalb Fuellstand, Erfolg der
// Entnahme und die Observer-Zaehler, nicht ein substrat-spezifischer Wert. Der Werte-Unterschied selbst
// ist bereits in tests/unit/test_container_genus.cpp:90-106 belegt und wird hier nicht doppelt gefuehrt.
//
// Registriert in tests/unit/CMakeLists.txt (COMDARE_GOALV6_BOOST_DTESTS -> add_test +
// COMDARE_TEST_TARGETS): keine Waisen-TU (Auflage 13).

#include "anatomy/adapter_permutation_engine.hpp"
#include "anatomy/adapter_tier.hpp"        // IAdapterTier + AdapterObserverSnapshotV1 (Wire-POD, NUR gelesen)
#include "anatomy/container_framework.hpp" // comdare::container::type_traits<Adapter>::slot_count (Gattungs-Pin)

#include <src/permutations/permutation_engine.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen der
// gleichnamigen Fixture-Typen ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace ana = comdare::cache_engine::anatomy;
namespace cco = comdare::container;
namespace mp  = boost::mp11;

static int g_fail = 0;
template <class A, class B>
static void eq(char const* w, A const& g, B const& e) {
    bool const ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << "  (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Nicht getriebene Achsen (die 10 geteilten/delegierten Paragraf-28-Achsen; AdapterAnatomy traegt sie,
// treibt real nur inner_container -- gleiche Ehrlichkeits-Grenze wie tests/unit/test_container_genus.cpp:21-23).
struct AxisAlpha {};
struct AxisBeta {};

struct CfgShared2 {
    using StaticAxisVariants = mp::mp_list<AxisAlpha, AxisBeta>;
}; // 2
struct CfgSingle {
    using StaticAxisVariants = mp::mp_list<AxisAlpha>;
}; // 1 (Filler)
struct CfgInner {
    using StaticAxisVariants = mp::mp_list<ana::DequeInner<>, ana::VectorInner<>, ana::HeapInner<>>;
}; // 3 ECHTE Inner-Substrate

// Pruefling-Slots fuer den Gattungs-Constraint der neuen Engine (Muster
// tests/unit/test_v41_search_algorithm_permutation_engine.cpp:182-206).
struct AdapterSlot {
    using PrueflingVariants                          = mp::mp_list<>;
    static constexpr bool              has_pruefling = false;
    static constexpr ana::AnatomyGenus genus         = ana::AnatomyGenus::Adapter;
};
struct SetSlot {
    using PrueflingVariants                          = mp::mp_list<>;
    static constexpr bool              has_pruefling = false;
    static constexpr ana::AnatomyGenus genus         = ana::AnatomyGenus::Set;
};
struct ImplicitSlot { // ohne explizites genus -> Default SearchAlgorithm, also NICHT Adapter
    using PrueflingVariants             = mp::mp_list<>;
    static constexpr bool has_pruefling = false;
};

/// axis_product<Cfgs...>() -- Permutations-Soll AUS DEN ACHSEN-LISTEN (kartesisches Produkt der mp_size).
template <class... Cfgs>
[[nodiscard]] constexpr std::size_t axis_product() noexcept {
    return (std::size_t{1} * ... * mp::mp_size<typename Cfgs::StaticAxisVariants>::value);
}

/// AdapterEngineFixture -- Slot-Belegung GENAU EINMAL genannt; Engine + Soll leiten sich daraus ab.
template <class... Cfgs>
struct AdapterEngineFixture {
    using engine_t                          = ana::AdapterPermutationEngine<Cfgs...>;
    static constexpr std::size_t soll_arity = sizeof...(Cfgs);
    static constexpr std::size_t soll_count = axis_product<Cfgs...>();
};

using Fx = AdapterEngineFixture<CfgShared2, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle,
                                CfgSingle, CfgSingle, CfgInner>;
using Engine = Fx::engine_t;

// -- Compile-Time-Teil des Gates --
static_assert(Engine::genus == ana::AnatomyGenus::Adapter, "Gattungs-Marker (Ebene 2) der Adapter-Engine");
static_assert(Engine::gattung == ana::AnatomyGattung::Container, "Aussen-Interface (Ebene 1) == Container");
static_assert(ana::gattung_of(Engine::genus) == Engine::gattung, "Ebenen-Abbildung konsistent (anatomy_base.hpp)");
static_assert(Fx::soll_arity == cco::type_traits<ana::AnatomyGenus::Adapter>::slot_count,
              "Slot-Belegung dieser TU muss der Adapter-Slot-Zahl entsprechen (container_framework.hpp:93)");
static_assert(Engine::arity() == Fx::soll_arity, "Engine-Aritaet == Slot-Zahl des Genus");
static_assert(Engine::count() == Fx::soll_count, "Permutations-Zahl == kartesisches Produkt der Achsen-Listen");

// Die Composition-Factory materialisiert die Adapter-Komposition slot-treu (Slot 0 geteilt, Slot 10 = inner).
using SamplePerm =
    comdare::cache_engine::permutations::PermTuple<AxisBeta, AxisAlpha, AxisAlpha, AxisAlpha, AxisAlpha, AxisAlpha,
                                                   AxisAlpha, AxisAlpha, AxisAlpha, AxisAlpha, ana::HeapInner<>>;
using SampleComp = ana::AdapterCompositionFromPermTuple<SamplePerm>;
static_assert(ana::IsAdapterComposition<SampleComp>);
static_assert(std::is_same_v<SampleComp::search_algo, AxisBeta>);             // Slot 0
static_assert(std::is_same_v<SampleComp::inner_container, ana::HeapInner<>>); // Slot 10 (spezifische Achse)
static_assert(SampleComp::slot_count == 11);

} // namespace

int main() {
    std::cout << "==== E-24 C2: AdapterPermutationEngine::for_each_abi_adapter (Gate: Materialisierung == Soll) ====\n";

    std::cout << "\n-- Soll-Herleitung (fixture-unabhaengig) --\n";
    eq("Slot-Zahl der Adapter-Gattung (container_framework/GenusBindingTraits)",
       cco::type_traits<ana::AnatomyGenus::Adapter>::slot_count, std::size_t{11});
    eq("Engine::arity() == Slot-Zahl des Genus", Engine::arity(), Fx::soll_arity);
    eq("Engine::count() == Produkt der Achsen-Listen (2 x 1^9 x 3)", Engine::count(), Fx::soll_count);

    std::cout << "\n-- for_each_abi_adapter: ABI-Materialisierung je Permutation --\n";
    std::size_t                   materialized = 0;
    std::vector<std::string_view> names;
    bool                          genus_ok       = true;
    bool                          organ_count_ok = true;
    bool                          tier_ok        = true;
    bool                          observer_ok    = true;

    Engine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view name) {
        ++materialized;
        names.push_back(name);
        if (base.genus() != ana::AnatomyGenus::Adapter) genus_ok = false;
        if (base.organ_count() != cco::type_traits<ana::AnatomyGenus::Adapter>::slot_count) organ_count_ok = false;

        auto* tier = dynamic_cast<ana::IAdapterTier*>(&base);
        if (tier == nullptr) {
            tier_ok = false;
            return;
        }
        // REAL treiben (disziplin-invariant): 3 put, 1 get, dann clear.
        tier->tier_put(10);
        tier->tier_put(20);
        tier->tier_put(30);
        if (tier->tier_size() != 3) tier_ok = false;
        std::uint64_t out = 0;
        if (!tier->tier_get(&out)) tier_ok = false; // Wert je Inner-Disziplin verschieden -> nicht gepinnt
        if (tier->tier_size() != 2) tier_ok = false;

        ana::AdapterObserverSnapshotV1 snap{};
        tier->tier_observe_container(&snap);
        if (snap.push_count != 3 || snap.pop_count != 1 || snap.front_reads != 1 || snap.back_reads != 0 ||
            snap.current_occupancy != 2 || snap.peak_occupancy != 3 ||
            snap.organ_count != cco::type_traits<ana::AnatomyGenus::Adapter>::slot_count) {
            observer_ok = false;
        }

        tier->tier_clear();
        if (tier->tier_size() != 0) tier_ok = false;
    });

    eq("GATE C2: Materialisierungs-Zaehlung == Slot-Permutations-Soll", materialized, Fx::soll_count);
    eq("Materialisierungs-Zaehlung == Engine::count()", materialized, Engine::count());
    eq("ein Kompositions-Name je Materialisierung", names.size(), Fx::soll_count);
    tr("jede Materialisierung traegt genus() == Adapter", genus_ok);
    tr("jede Materialisierung traegt organ_count() == Slot-Zahl des Genus", organ_count_ok);
    tr("IAdapterTier ist an JEDER Materialisierung per dynamic_cast erreichbar + real getrieben", tier_ok);
    tr("der Wire-Observer (AdapterObserverSnapshotV1) traegt die real getriebenen Zahlen", observer_ok);
    tr("Kompositions-Name ist der Adapter-Gattungs-Name",
       !names.empty() && names.front() == std::string_view{"AdapterComposition"});

    std::cout << "\n-- Gegenprobe: for_each_adapter / for_each_composition_type / for_each_abi_adapter --\n";
    std::size_t via_anatomy = 0;
    Engine::for_each_adapter([&](auto& anatomy, std::string_view) {
        ++via_anatomy;
        using AnatomyT = std::remove_reference_t<decltype(anatomy)>;
        static_assert(AnatomyT::genus() == ana::AnatomyGenus::Adapter);
        static_assert(AnatomyT::gattung() == ana::AnatomyGattung::Container);
    });
    std::size_t via_type = 0;
    Engine::for_each_composition_type([&]<class C>() {
        ++via_type;
        static_assert(ana::IsAdapterComposition<C>);
    });
    eq("for_each_adapter besucht denselben Raum", via_anatomy, materialized);
    eq("for_each_composition_type besucht denselben Raum", via_type, materialized);

    // -- Gattungs-Constraint: die Slot-Validierung der neuen Engine ist scharf (Cross-Genus type-unmoeglich) --
    std::cout << "\n-- Gattungs-Constraint der neuen Engine --\n";
    Engine::assert_pruefling_slot_genus<AdapterSlot>();                   // uebersetzt -> Slot passt zur Gattung
    Engine::assert_all_pruefling_slots_genus<AdapterSlot, AdapterSlot>(); // variadische Bulk-Form
    tr("slots_match_genus_v<AdapterSlot> == true", Engine::slots_match_genus_v<AdapterSlot>);
    tr("slots_match_genus_v<SetSlot> == false (Cross-Genus)", !Engine::slots_match_genus_v<SetSlot>);
    tr("slots_match_genus_v<AdapterSlot, SetSlot> == false (ein Fremd-Slot genuegt)",
       !Engine::slots_match_genus_v<AdapterSlot, SetSlot>);
    tr("Default-Slot ohne explizites genus gehoert NICHT zur Adapter-Gattung",
       !Engine::slots_match_genus_v<ImplicitSlot>);

    std::cout << "\n==== E-24 C2 AdapterPermutationEngine: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
