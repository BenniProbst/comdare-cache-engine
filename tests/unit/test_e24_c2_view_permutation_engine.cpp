// test_e24_c2_view_permutation_engine -- E-24 C2 (Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C2): die Test-TU der VIEW-Engine.
//
// GATE (Bauplan Paragraf 4.2, Klasse II, wortwoertlich fuer C2):
//   "je Engine: for_each_abi_adapter-Materialisierungs-Zaehlung == Slot-Permutations-Soll des Genus"
// Fixture-unabhaengiger Ableitungsweg (Bauplan Paragraf 4.3): das Soll ist das mp_size-Produkt der
// TopicConfigSets, gekreuzt mit der Gattungs-Bau-Bindung (comdare::container::type_traits<View>::slot_count
// == 5, container_framework.hpp:96). Keine handgepflegte Zahl.
//
// Die permutierten Slots sind die REAL getriebenen: axis_layout (ViewAnatomy ruft layout_.index_of) und
// axis_accessor (accessor_.access), view_anatomy.hpp:48-56. Die Policies stammen aus der echten Achsen-
// Registry (topics/view/view_policies.hpp). LayoutStrided liest physisch ANDERE Speicherzellen als
// LayoutRight -- die Erwartungen dieser TU sind deshalb bewusst layout-INVARIANT formuliert (Index 0 und ein
// weit ausserhalb liegender Index verhalten sich unter jedem Layout gleich), statt ein Layout zu bevorzugen.
//
// Registriert in tests/unit/CMakeLists.txt (COMDARE_GOALV6_BOOST_DTESTS -> add_test + COMDARE_TEST_TARGETS):
// keine Waisen-TU (Auflage 13).

#include "anatomy/view_permutation_engine.hpp"
#include "anatomy/view_tier.hpp" // IViewTier + ViewObserverSnapshotV1 (Wire-POD, NUR gelesen)
#include "builder/experiment_tree/container_type_traits.hpp" // SF-1: type_traits<View>::slot_count (Gattungs-Pin)
#include "topics/view/view_policies.hpp"                     // ECHTE axis_layout-/axis_accessor-Policies

#include <src/permutations/permutation_engine.hpp>

#include <boost/mp11.hpp>

#include <array>
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
namespace vw  = comdare::cache_engine::view;
namespace cco = comdare::cache_engine::builder::experiment; // SF-1: liefert type_traits<G> (vormals comdare::container)
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

// Nicht getriebene Achsen (Komposition-Identitaet: memory_layout / value_handle).
struct AxisAlpha {};
struct AxisBeta {};

struct CfgShared2 {
    using StaticAxisVariants = mp::mp_list<AxisAlpha, AxisBeta>;
}; // 2
struct CfgSingle {
    using StaticAxisVariants = mp::mp_list<AxisAlpha>;
}; // 1 (Filler)
struct CfgExtent {
    using StaticAxisVariants = mp::mp_list<ana::DynamicExtent>;
}; // 1
struct CfgLayout {
    using StaticAxisVariants = mp::mp_list<ana::LayoutRight, vw::LayoutLeft, vw::LayoutStrided<2>>;
}; // 3 ECHTE Layouts
struct CfgAccessor {
    using StaticAxisVariants = mp::mp_list<ana::DefaultAccessor, vw::AlignedAccessor<64>>;
}; // 2 ECHTE Accessoren

static_assert(ana::LayoutPolicy<ana::LayoutRight>);
static_assert(ana::LayoutPolicy<vw::LayoutStrided<2>>);
static_assert(ana::AccessorPolicy<ana::DefaultAccessor>);
static_assert(ana::AccessorPolicy<vw::AlignedAccessor<64>>);

/// axis_product<Cfgs...>() -- Permutations-Soll AUS DEN ACHSEN-LISTEN (kartesisches Produkt der mp_size).
template <class... Cfgs>
[[nodiscard]] constexpr std::size_t axis_product() noexcept {
    return (std::size_t{1} * ... * mp::mp_size<typename Cfgs::StaticAxisVariants>::value);
}

/// ViewEngineFixture -- Slot-Belegung GENAU EINMAL genannt; Engine + Soll leiten sich daraus ab.
template <class... Cfgs>
struct ViewEngineFixture {
    using engine_t                          = ana::ViewPermutationEngine<Cfgs...>;
    static constexpr std::size_t soll_arity = sizeof...(Cfgs);
    static constexpr std::size_t soll_count = axis_product<Cfgs...>();
};

using Fx     = ViewEngineFixture<CfgShared2, CfgSingle, CfgExtent, CfgLayout, CfgAccessor>;
using Engine = Fx::engine_t;

static_assert(Engine::genus == ana::AnatomyGenus::View, "Gattungs-Marker der View-Engine");
static_assert(Fx::soll_arity == cco::type_traits<ana::AnatomyGenus::View>::slot_count,
              "Slot-Belegung dieser TU muss der View-Slot-Zahl entsprechen (container_framework.hpp:96)");
static_assert(Engine::arity() == Fx::soll_arity, "Engine-Aritaet == Slot-Zahl des Genus");
static_assert(Engine::count() == Fx::soll_count, "Permutations-Zahl == kartesisches Produkt der Achsen-Listen");

} // namespace

int main() {
    std::cout << "==== E-24 C2: ViewPermutationEngine::for_each_abi_adapter (Gate: Materialisierung == Soll) ====\n";

    std::cout << "\n-- Soll-Herleitung (fixture-unabhaengig) --\n";
    eq("Slot-Zahl der View-Gattung (container_framework/GenusBindingTraits)",
       cco::type_traits<ana::AnatomyGenus::View>::slot_count, std::size_t{5});
    eq("Engine::arity() == Slot-Zahl des Genus", Engine::arity(), Fx::soll_arity);
    eq("Engine::count() == Produkt der Achsen-Listen (2 x 1 x 1 x 3 x 2)", Engine::count(), Fx::soll_count);

    std::cout << "\n-- for_each_abi_adapter: ABI-Materialisierung je Permutation --\n";
    std::array<std::uint64_t, 8> const buffer{11, 22, 33, 44, 55, 66, 77, 88}; // EXTERNER Puffer (non-owning)

    std::size_t                   materialized = 0;
    std::vector<std::string_view> names;
    bool                          genus_ok       = true;
    bool                          organ_count_ok = true;
    bool                          tier_ok        = true;
    bool                          observer_ok    = true;

    Engine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view name) {
        ++materialized;
        names.push_back(name);
        if (base.genus() != ana::AnatomyGenus::View) genus_ok = false;
        if (base.organ_count() != cco::type_traits<ana::AnatomyGenus::View>::slot_count) organ_count_ok = false;

        auto* tier = dynamic_cast<ana::IViewTier*>(&base);
        if (tier == nullptr) {
            tier_ok = false;
            return;
        }
        // REAL treiben (layout-INVARIANT): binden, Index 0 lesen (Offset 0 unter jedem Layout), dann weit
        // ausserhalb lesen (unter jedem Layout out-of-bounds). KEIN insert/erase/clear -- non-owning.
        tier->tier_bind(buffer.data(), buffer.size());
        std::uint64_t out = 0;
        if (!tier->tier_read(0, &out) || out != buffer[0]) tier_ok = false;
        if (tier->tier_read(100000, &out)) tier_ok = false;
        if (tier->tier_size() != buffer.size()) tier_ok = false;

        ana::ViewObserverSnapshotV1 snap{};
        tier->tier_observe_view(&snap);
        if (snap.bind_count != 1 || snap.bound_size != buffer.size() || snap.read_count != 2 ||
            snap.read_oob_count != 1 || snap.organ_count != cco::type_traits<ana::AnatomyGenus::View>::slot_count) {
            observer_ok = false;
        }
    });

    eq("GATE C2: Materialisierungs-Zaehlung == Slot-Permutations-Soll", materialized, Fx::soll_count);
    eq("Materialisierungs-Zaehlung == Engine::count()", materialized, Engine::count());
    eq("ein Kompositions-Name je Materialisierung", names.size(), Fx::soll_count);
    tr("jede Materialisierung traegt genus() == View", genus_ok);
    tr("jede Materialisierung traegt organ_count() == Slot-Zahl des Genus", organ_count_ok);
    tr("IViewTier ist an JEDER Materialisierung per dynamic_cast erreichbar + real getrieben", tier_ok);
    tr("der Wire-Observer (ViewObserverSnapshotV1) traegt die real getriebenen Zahlen", observer_ok);
    tr("Kompositions-Name ist der View-Gattungs-Name",
       !names.empty() && names.front() == std::string_view{"ViewComposition"});

    std::cout << "\n-- Gegenprobe: for_each_view / for_each_composition_type / for_each_abi_adapter --\n";
    std::size_t via_anatomy = 0;
    Engine::for_each_view([&](auto& anatomy, std::string_view) {
        ++via_anatomy;
        using AnatomyT = std::remove_reference_t<decltype(anatomy)>;
        static_assert(AnatomyT::genus() == ana::AnatomyGenus::View);
    });
    std::size_t via_type = 0;
    Engine::for_each_composition_type([&]<class C>() {
        ++via_type;
        static_assert(ana::IsViewComposition<C>);
    });
    eq("for_each_view besucht denselben Raum", via_anatomy, materialized);
    eq("for_each_composition_type besucht denselben Raum", via_type, materialized);

    std::cout << "\n==== E-24 C2 ViewPermutationEngine: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
