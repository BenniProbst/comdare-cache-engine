// test_e24_c2_sequence_permutation_engine -- E-24 C2 (Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C2): die Test-TU der SEQUENCE-Engine.
//
// GATE (Bauplan Paragraf 4.2, Klasse II, wortwoertlich fuer C2):
//   "je Engine: for_each_abi_adapter-Materialisierungs-Zaehlung == Slot-Permutations-Soll des Genus"
// Fixture-unabhaengiger Ableitungsweg (Bauplan Paragraf 4.3): das Soll ist das mp_size-Produkt der
// TopicConfigSets, gekreuzt mit der Gattungs-Bau-Bindung (comdare::container::type_traits<Sequence>::
// slot_count == 9, container_framework.hpp:95). Keine handgepflegte Zahl.
//
// Der permutierte Slot ist der REAL getriebene: axis_growth (SequenceAnatomy ruft growth_.next_capacity bei
// Ueberlauf, sequence_anatomy.hpp:45-55). Die Policies stammen aus der echten Achsen-Registry
// (topics/sequence/axis_growth/axis_growth_policies.hpp), nicht aus Test-Attrappen -- damit steht hinter
// jeder Materialisierung ein anderes Wachstums-Verhalten und nicht nur ein anderer Typname.
//
// Registriert in tests/unit/CMakeLists.txt (COMDARE_GOALV6_BOOST_DTESTS -> add_test + COMDARE_TEST_TARGETS):
// keine Waisen-TU (Auflage 13).

#include "anatomy/sequence_permutation_engine.hpp"
#include "anatomy/sequence_tier.hpp"       // ISequenceTier + SequenceObserverSnapshotV1 (Wire-POD, NUR gelesen)
#include "anatomy/container_framework.hpp" // comdare::container::type_traits<Sequence>::slot_count (Gattungs-Pin)
#include "topics/sequence/axis_growth/axis_growth_policies.hpp" // ECHTE axis_growth-Policies

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
namespace ag  = comdare::cache_engine::sequence::axis_growth;
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

// Nicht getriebene Achsen (Komposition-Identitaet; SequenceAnatomy traegt sie, treibt nur axis_growth real).
struct AxisAlpha {};
struct AxisBeta {};

struct CfgShared2 {
    using StaticAxisVariants = mp::mp_list<AxisAlpha, AxisBeta>;
}; // 2
struct CfgSingle {
    using StaticAxisVariants = mp::mp_list<AxisAlpha>;
}; // 1 (Filler)
struct CfgGrowth {
    using StaticAxisVariants = mp::mp_list<ana::DoublingGrowth, ag::GoldenRatioGrowth, ag::ExactGrowth>;
}; // 3 ECHTE Policies

static_assert(ana::GrowthPolicy<ana::DoublingGrowth>);
static_assert(ana::GrowthPolicy<ag::GoldenRatioGrowth>);
static_assert(ana::GrowthPolicy<ag::ExactGrowth>);

/// axis_product<Cfgs...>() -- Permutations-Soll AUS DEN ACHSEN-LISTEN (kartesisches Produkt der mp_size).
template <class... Cfgs>
[[nodiscard]] constexpr std::size_t axis_product() noexcept {
    return (std::size_t{1} * ... * mp::mp_size<typename Cfgs::StaticAxisVariants>::value);
}

/// SequenceEngineFixture -- Slot-Belegung GENAU EINMAL genannt; Engine + Soll leiten sich daraus ab.
template <class... Cfgs>
struct SequenceEngineFixture {
    using engine_t                          = ana::SequencePermutationEngine<Cfgs...>;
    static constexpr std::size_t soll_arity = sizeof...(Cfgs);
    static constexpr std::size_t soll_count = axis_product<Cfgs...>();
};

using Fx     = SequenceEngineFixture<CfgShared2, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle, CfgSingle,
                                     CfgSingle, CfgGrowth>;
using Engine = Fx::engine_t;

static_assert(Engine::genus == ana::AnatomyGenus::Sequence, "Gattungs-Marker der Sequence-Engine");
static_assert(Fx::soll_arity == cco::type_traits<ana::AnatomyGenus::Sequence>::slot_count,
              "Slot-Belegung dieser TU muss der Sequence-Slot-Zahl entsprechen (container_framework.hpp:95)");
static_assert(Engine::arity() == Fx::soll_arity, "Engine-Aritaet == Slot-Zahl des Genus");
static_assert(Engine::count() == Fx::soll_count, "Permutations-Zahl == kartesisches Produkt der Achsen-Listen");

} // namespace

int main() {
    std::cout
        << "==== E-24 C2: SequencePermutationEngine::for_each_abi_adapter (Gate: Materialisierung == Soll) ====\n";

    std::cout << "\n-- Soll-Herleitung (fixture-unabhaengig) --\n";
    eq("Slot-Zahl der Sequence-Gattung (container_framework/GenusBindingTraits)",
       cco::type_traits<ana::AnatomyGenus::Sequence>::slot_count, std::size_t{9});
    eq("Engine::arity() == Slot-Zahl des Genus", Engine::arity(), Fx::soll_arity);
    eq("Engine::count() == Produkt der Achsen-Listen (2 x 1^7 x 3)", Engine::count(), Fx::soll_count);

    std::cout << "\n-- for_each_abi_adapter: ABI-Materialisierung je Permutation --\n";
    std::size_t                   materialized = 0;
    std::vector<std::string_view> names;
    bool                          genus_ok       = true;
    bool                          organ_count_ok = true;
    bool                          tier_ok        = true;
    bool                          observer_ok    = true;
    std::uint64_t                 growth_sum     = 0;

    Engine::for_each_abi_adapter([&](ana::IAnatomyBase& base, std::string_view name) {
        ++materialized;
        names.push_back(name);
        if (base.genus() != ana::AnatomyGenus::Sequence) genus_ok = false;
        if (base.organ_count() != cco::type_traits<ana::AnatomyGenus::Sequence>::slot_count) organ_count_ok = false;

        auto* tier = dynamic_cast<ana::ISequenceTier*>(&base);
        if (tier == nullptr) {
            tier_ok = false;
            return;
        }
        // REAL treiben: 5 push_back (loest die axis_growth-Policy mehrfach aus) + 2 at (1 gueltig / 1 oob).
        for (std::uint64_t i = 0; i < 5; ++i) tier->tier_push_back(i * 10);
        std::uint64_t out = 0;
        if (!tier->tier_at(3, &out) || out != 30) tier_ok = false;
        if (tier->tier_at(99, &out)) tier_ok = false;
        if (tier->tier_size() != 5) tier_ok = false;

        ana::SequenceObserverSnapshotV1 snap{};
        tier->tier_observe_sequence(&snap);
        growth_sum += snap.growth_events;
        if (snap.push_count != 5 || snap.at_count != 2 || snap.at_oob_count != 1 || snap.current_size != 5 ||
            snap.peak_size != 5 || snap.growth_events == 0 ||
            snap.organ_count != cco::type_traits<ana::AnatomyGenus::Sequence>::slot_count) {
            observer_ok = false;
        }
    });

    eq("GATE C2: Materialisierungs-Zaehlung == Slot-Permutations-Soll", materialized, Fx::soll_count);
    eq("Materialisierungs-Zaehlung == Engine::count()", materialized, Engine::count());
    eq("ein Kompositions-Name je Materialisierung", names.size(), Fx::soll_count);
    tr("jede Materialisierung traegt genus() == Sequence", genus_ok);
    tr("jede Materialisierung traegt organ_count() == Slot-Zahl des Genus", organ_count_ok);
    tr("ISequenceTier ist an JEDER Materialisierung per dynamic_cast erreichbar + real getrieben", tier_ok);
    tr("der Wire-Observer (SequenceObserverSnapshotV1) traegt die real getriebenen Zahlen", observer_ok);
    tr("axis_growth war real wirksam (Summe growth_events > Materialisierungs-Zahl)",
       growth_sum > static_cast<std::uint64_t>(materialized));
    tr("Kompositions-Name ist der Sequence-Gattungs-Name",
       !names.empty() && names.front() == std::string_view{"SequenceComposition"});

    std::cout << "\n-- Gegenprobe: for_each_sequence / for_each_composition_type / for_each_abi_adapter --\n";
    std::size_t via_anatomy = 0;
    Engine::for_each_sequence([&](auto& anatomy, std::string_view) {
        ++via_anatomy;
        using AnatomyT = std::remove_reference_t<decltype(anatomy)>;
        static_assert(AnatomyT::genus() == ana::AnatomyGenus::Sequence);
    });
    std::size_t via_type = 0;
    Engine::for_each_composition_type([&]<class C>() {
        ++via_type;
        static_assert(ana::IsSequenceComposition<C>);
    });
    eq("for_each_sequence besucht denselben Raum", via_anatomy, materialized);
    eq("for_each_composition_type besucht denselben Raum", via_type, materialized);

    std::cout << "\n==== E-24 C2 SequencePermutationEngine: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
