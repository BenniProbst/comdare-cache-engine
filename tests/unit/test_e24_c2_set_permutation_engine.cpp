// test_e24_c2_set_permutation_engine -- E-24 C2 (Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C2): die Test-TU der SET-Engine.
//
// GATE (Bauplan Paragraf 4.2, Klasse II, wortwoertlich fuer C2):
//   "je Engine: for_each_abi_adapter-Materialisierungs-Zaehlung == Slot-Permutations-Soll des Genus"
// Genau das wird hier belegt -- auf einem FIXTURE-UNABHAENGIGEN ABLEITUNGSWEG (Memory-Lehre "gruene Tests
// zementieren alte Ordnung", Bauplan Paragraf 4.3): das Soll wird
//   (1) aus den Achsen-Listen der TopicConfigSets hergeleitet (mp_size-Produkt), NICHT aus einer im Test
//       handgepflegten Zahl, und
//   (2) gegen die Gattungs-Bau-Bindung gekreuzt (comdare::container::type_traits<Set>::slot_count == 13,
//       container_framework.hpp:94) -- aendert jemand die Slot-Zahl der Set-Gattung, bricht dieser Test,
//       statt eine alte Ordnung gruen zu zementieren.
//
// WARUM eine EIGENE TU je Engine (Auflage 13 / Waisen-TU-Lehre): test_genus_permutation_engines.cpp beweist
// die Engines auf TYP-Ebene mit Dummy-Achsen und instantiiert bewusst KEINE Anatomie. for_each_abi_adapter
// materialisiert dagegen echte Anatomien + ABI-Adapter, braucht also am getriebenen Slot ECHTE Organe
// (SetAnatomy treibt Composition::search_algo real, set_anatomy.hpp:49-77) -- ein Dummy-Typ genuegt dort
// NICHT. Die TU ist in tests/unit/CMakeLists.txt registriert (COMDARE_GOALV6_BOOST_DTESTS -> add_test +
// COMDARE_TEST_TARGETS), also weder Waise noch nur-Quelle-im-Tree.

#include "anatomy/set_permutation_engine.hpp"
#include "anatomy/set_tier.hpp"            // ISetTier + SetObserverSnapshotV1 (Wire-POD, NUR gelesen)
#include "anatomy/container_framework.hpp" // comdare::container::type_traits<Set>::slot_count (Gattungs-Pin)

#include <src/permutations/permutation_engine.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

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

// ---------------------------------------------------------------------------------------------------
// Zwei ECHTE K-only-Such-Organe fuer den getriebenen Slot 0 (search_algo). Pflicht-API der Set-Anatomie:
// key_type + insert(k,v) / lookup(k) / erase(k) / occupied_count() / clear() (set_anatomy.hpp:49-81).
// Muster uebernommen aus tests/unit/test_d9_set.cpp:21-31 -- zwei Substrate, damit der permutierte Slot
// echte Algorithmus-Vielfalt traegt und nicht nur zwei Namen.
// ---------------------------------------------------------------------------------------------------
struct OrderedKeySet {
    using key_type = std::uint64_t;
    std::set<std::uint64_t>                    s;
    void                                       insert(std::uint64_t k, std::uint64_t /*v*/) { s.insert(k); }
    [[nodiscard]] std::optional<std::uint64_t> lookup(std::uint64_t k) const {
        return s.count(k) ? std::optional<std::uint64_t>{k} : std::nullopt;
    }
    void                      erase(std::uint64_t k) { s.erase(k); }
    [[nodiscard]] std::size_t occupied_count() const { return s.size(); }
    void                      clear() { s.clear(); }
};

struct HashedKeySet {
    using key_type = std::uint64_t;
    std::unordered_set<std::uint64_t>          s;
    void                                       insert(std::uint64_t k, std::uint64_t /*v*/) { s.insert(k); }
    [[nodiscard]] std::optional<std::uint64_t> lookup(std::uint64_t k) const {
        return s.count(k) ? std::optional<std::uint64_t>{k} : std::nullopt;
    }
    void                      erase(std::uint64_t k) { s.erase(k); }
    [[nodiscard]] std::size_t occupied_count() const { return s.size(); }
    void                      clear() { s.clear(); }
};

// Nicht getriebene Achsen (Komposition-Identitaet; SetAnatomy traegt sie, treibt sie aber nicht -- R5.B-Grenze).
struct AxisAlpha {};
struct AxisBeta {};
struct AxisGamma {};

struct CfgSearchAlgo {
    using StaticAxisVariants = mp::mp_list<OrderedKeySet, HashedKeySet>;
}; // 2 ECHTE Mengen-Organe
struct CfgTraversal {
    using StaticAxisVariants = mp::mp_list<AxisAlpha, AxisBeta, AxisGamma>;
}; // 3
struct CfgSingle {
    using StaticAxisVariants = mp::mp_list<AxisAlpha>;
}; // 1 (Filler)

/// axis_product<Cfgs...>() -- das Permutations-Soll AUS DEN ACHSEN-LISTEN (kartesisches Produkt der
/// mp_size-Werte). Der fixture-unabhaengige Ableitungsweg: keine Zahl im Test, sondern eine Rechnung.
template <class... Cfgs>
[[nodiscard]] constexpr std::size_t axis_product() noexcept {
    return (std::size_t{1} * ... * mp::mp_size<typename Cfgs::StaticAxisVariants>::value);
}

/// SetEngineFixture -- nennt die Slot-Belegung GENAU EINMAL; Engine, Soll-Aritaet und Soll-Zahl leiten
/// sich daraus ab. Damit kann Slot-Liste und Erwartung nicht auseinanderlaufen.
template <class... Cfgs>
struct SetEngineFixture {
    using engine_t                          = ana::SetPermutationEngine<Cfgs...>;
    static constexpr std::size_t soll_arity = sizeof...(Cfgs);
    static constexpr std::size_t soll_count = axis_product<Cfgs...>();
};

using Fx     = SetEngineFixture<CfgSearchAlgo, CfgTraversal, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle,
                                CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle>;
using Engine = Fx::engine_t;

// ---------------------------------------------------------------------------------------------------
// Compile-Time-Teil des Gates
// ---------------------------------------------------------------------------------------------------
static_assert(Engine::genus == ana::AnatomyGenus::Set, "Gattungs-Marker der Set-Engine");
static_assert(Fx::soll_arity == cco::type_traits<ana::AnatomyGenus::Set>::slot_count,
              "Slot-Belegung dieser TU muss der Set-Gattungs-Slot-Zahl entsprechen (container_framework.hpp:94)");
static_assert(Engine::arity() == Fx::soll_arity, "Engine-Aritaet == Slot-Zahl des Genus");
static_assert(Engine::count() == Fx::soll_count, "Permutations-Zahl == kartesisches Produkt der Achsen-Listen");

int main() {
    std::cout << "==== E-24 C2: SetPermutationEngine::for_each_abi_adapter (Gate: Materialisierung == Soll) ====\n";

    std::cout << "\n-- Soll-Herleitung (fixture-unabhaengig) --\n";
    eq("Slot-Zahl der Set-Gattung (container_framework/GenusBindingTraits)",
       cco::type_traits<ana::AnatomyGenus::Set>::slot_count, std::size_t{13});
    eq("Engine::arity() == Slot-Zahl des Genus", Engine::arity(), Fx::soll_arity);
    eq("Engine::count() == Produkt der Achsen-Listen (2 x 3 x 1^11)", Engine::count(), Fx::soll_count);

    // -- Gate: Materialisierungs-Zaehlung ueber for_each_abi_adapter --
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
        if (base.genus() != ana::AnatomyGenus::Set) genus_ok = false;
        if (base.organ_count() != cco::type_traits<ana::AnatomyGenus::Set>::slot_count) organ_count_ok = false;

        // Die Gattungs-Antriebs-Flaeche liegt am selben Objekt (genau der Weg des Set-Docks an einer
        // geladenen .dll: dynamic_cast<ISetTier*> auf den IAnatomyBase* aus comdare_create_anatomy()).
        auto* tier = dynamic_cast<ana::ISetTier*>(&base);
        if (tier == nullptr) {
            tier_ok = false;
            return;
        }
        // REAL treiben, nicht nur zaehlen: 3 inserts (davon 1 Duplikat), 2 contains (1 hit / 1 miss), 1 erase.
        if (!tier->tier_set_insert(1) || !tier->tier_set_insert(2)) tier_ok = false;
        if (tier->tier_set_insert(2)) tier_ok = false; // Duplikat -> false (Mengen-Semantik)
        if (!tier->tier_set_contains(1)) tier_ok = false;
        if (tier->tier_set_contains(99)) tier_ok = false;
        if (!tier->tier_set_erase(1)) tier_ok = false;
        if (tier->tier_set_size() != 1) tier_ok = false;

        ana::SetObserverSnapshotV1 snap{};
        tier->tier_observe_set(&snap);
        if (snap.insert_count != 2 || snap.contains_count != 2 || snap.contains_hit_count != 1 ||
            snap.contains_miss_count != 1 || snap.erase_count != 1 || snap.current_size != 1 || snap.peak_size != 2 ||
            snap.organ_count != cco::type_traits<ana::AnatomyGenus::Set>::slot_count) {
            observer_ok = false;
        }
    });

    eq("GATE C2: Materialisierungs-Zaehlung == Slot-Permutations-Soll", materialized, Fx::soll_count);
    eq("Materialisierungs-Zaehlung == Engine::count()", materialized, Engine::count());
    eq("ein Kompositions-Name je Materialisierung", names.size(), Fx::soll_count);
    tr("jede Materialisierung traegt genus() == Set", genus_ok);
    tr("jede Materialisierung traegt organ_count() == Slot-Zahl des Genus", organ_count_ok);
    tr("ISetTier ist an JEDER Materialisierung per dynamic_cast erreichbar + real getrieben", tier_ok);
    tr("der Wire-Observer (SetObserverSnapshotV1) traegt die real getriebenen Zahlen", observer_ok);
    tr("Kompositions-Name ist der Set-Gattungs-Name",
       !names.empty() && names.front() == std::string_view{"SetComposition"});

    // -- Gegenprobe: die drei Iterations-Wege besuchen DENSELBEN Raum (kein stiller Sonderweg) --
    std::cout << "\n-- Gegenprobe: for_each_set / for_each_composition_type / for_each_abi_adapter --\n";
    std::size_t via_anatomy = 0;
    Engine::for_each_set([&](auto& anatomy, std::string_view) {
        ++via_anatomy;
        using AnatomyT = std::remove_reference_t<decltype(anatomy)>;
        static_assert(AnatomyT::genus() == ana::AnatomyGenus::Set);
    });
    std::size_t via_type = 0;
    Engine::for_each_composition_type([&]<class C>() {
        ++via_type;
        static_assert(ana::IsSetComposition<C>);
    });
    eq("for_each_set besucht denselben Raum", via_anatomy, materialized);
    eq("for_each_composition_type besucht denselben Raum", via_type, materialized);

    std::cout << "\n==== E-24 C2 SetPermutationEngine: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
