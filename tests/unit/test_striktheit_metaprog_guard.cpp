// Prospektiver Metaprogrammierungs-Striktheits-Guard (Goal-V3, Striktheit der Metaprogrammierung).
//
// **Zweck:** Die Direktiven zur compile-time-Striktheit ([[feedback_metaprogrammierung_compile_time_zwingend_durchsetzen]],
// [[feedback_no_runtime_switch]], [[feedback_compile_time_only_no_runtime]]) waren bisher nur post-hoc (Review-Linse)
// + verstreut (POD-static_asserts je Header) gesichert. Dieser Guard verankert sie PROSPEKTIV als stehende
// compile-time-Invariante: er BEWEIST die Striktheit positiv und FAENGT jede kuenftige compile-time->runtime-
// Degradation (versehentliche vtable/virtual im Hot-Path) automatisch beim Build/ctest.
//
// **ADDITIV & golden/ABI-NEUTRAL:** reiner Test ueber BESTEHENDE Typen (static_assert). KEINE Aenderung an
// anatomy_base-Enums, den Anatomien, GenusBindingTraits, golden_fullpilot_320, permutation_axes, POD/ABI-4.
//
// **Scope (robuster Kern):** die 11 Referenz-Anatomien (SearchAlgorithm-Gattung) + 3-Ebenen-Totalitaet +
// ABI-Grenze-Positiv-Beweis. Das flache 26-Achsen-Strategie-Aggregat (registry_to_axis_levels.hpp axes26)
// + die zentrale POD-Snapshot-Buendelung bleiben ein FOLGE-Increment (schwerer Header, separater Commit).

#include <anatomy/anatomy_base.hpp>                 // AnatomyGattung/Genus, gattung_of, AnatomyConcept, IAnatomyBase
#include <anatomy/composition_concept.hpp>          // IsComposition
#include <anatomy/idriveable_tier.hpp>              // IDriveableTier (bewusste ABI-Grenze)
#include <anatomy/measurable_workload.hpp>          // IMeasurableWorkload (bewusste ABI-Grenze)
#include <anatomy/search_algorithm_anatomy.hpp>     // SearchAlgorithmAnatomy<Composition>
#include <compositions/known_compositions_list.hpp> // KnownReferenceCompositions (11 Entry-Wrapper)

#include <boost/mp11.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace cea  = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace mp   = ::boost::mp11;

// ── Block A: 3-Ebenen-Anatomie-Totalitaet (Ebene 2 Genus -> Ebene 1 Gattung, gattung_of konsistent) ──────
static_assert(cea::gattung_of(cea::AnatomyGenus::SearchAlgorithm) == cea::AnatomyGattung::SearchAlgorithm);
static_assert(cea::gattung_of(cea::AnatomyGenus::Set) == cea::AnatomyGattung::Container);
static_assert(cea::gattung_of(cea::AnatomyGenus::Sequence) == cea::AnatomyGattung::Container);
static_assert(cea::gattung_of(cea::AnatomyGenus::Adapter) == cea::AnatomyGattung::Container);
static_assert(cea::gattung_of(cea::AnatomyGenus::View) == cea::AnatomyGattung::Container);
// golden-relevante Ebene-1-Namen konstant (Aussen-Interface-Strings).
static_assert(cea::gattung_name(cea::AnatomyGattung::SearchAlgorithm) == std::string_view{"SearchAlgorithm"});
static_assert(cea::gattung_name(cea::AnatomyGattung::Container) == std::string_view{"Container"});
static_assert(cea::gattung_name(cea::AnatomyGattung::Graph) == std::string_view{"Graph"});

// ── Block B: Hot-Path-Striktheit — die 11 Referenz-Anatomien sind NICHT-polymorph + AnatomyConcept-erfuellt ──
// anatomy_of<Entry> = die konkrete SearchAlgorithm-Anatomie ueber der Composition dieses Referenz-Eintrags.
template <class Entry>
using anatomy_of = cea::SearchAlgorithmAnatomy<typename Entry::composition>;

// is_strict_hot_path_anatomy<Entry> == true gdw. die Anatomie compile-time-strikt ist:
//   (1) nicht-polymorph  -> keine vtable  -> compile-time-Dispatch (kein Runtime-Switch)
//   (2) kein virtueller Destruktor
//   (3) AnatomyConcept erfuellt -> vollstaendiger compile-time-API-Vertrag
//   (4) die Composition erfuellt IsComposition (compile-time-Composition-Concept)
template <class Entry>
struct is_strict_hot_path_anatomy
    : std::bool_constant<!std::is_polymorphic_v<anatomy_of<Entry>> &&
                         !std::has_virtual_destructor_v<anatomy_of<Entry>> && cea::AnatomyConcept<anatomy_of<Entry>> &&
                         cea::IsComposition<typename Entry::composition>> {};

static_assert(mp::mp_size<comp::KnownReferenceCompositions>::value == 11,
              "Guard-Basis: 11 Referenz-Compositions (6 CE-Reimpl + 5 PaperBinding).");
static_assert(mp::mp_all_of<comp::KnownReferenceCompositions, is_strict_hot_path_anatomy>::value,
              "Metaprog-Striktheit: ALLE 11 Referenz-Anatomien muessen nicht-polymorph (keine vtable im "
              "Hot-Path) + AnatomyConcept-erfuellt sein. Eine compile-time->runtime-Degradation "
              "(virtual/vtable im Hot-Path, IAnatomyBase-Erbe einer konkreten Anatomie) bricht diesen Guard.");

// ── Block C: ABI-Grenze bewusst polymorph — die EINE erlaubte vtable-Zone (verhindert De-Virtualisieren) ──
static_assert(std::is_polymorphic_v<cea::IAnatomyBase>,
              "ABI-Grenze: IAnatomyBase ist bewusst polymorph (Modul-Loader-Interface, NICHT Hot-Path).");
static_assert(std::is_polymorphic_v<cea::IDriveableTier>,
              "ABI-Grenze: IDriveableTier ist bewusst polymorph (Tier-Treiber-Interface).");
static_assert(std::is_polymorphic_v<cea::IMeasurableWorkload>,
              "ABI-Grenze: IMeasurableWorkload ist bewusst polymorph (Batch-Treiber, 1 Call je Batch).");

// ── GTEST-Traeger (dokumentiert die compile-time-Beweise + prueft Kardinalitaet/Konsistenz zur Laufzeit) ──
TEST(MetaprogStriktheitGuard, ElevenReferenceAnatomiesAreNonPolymorphicHotPath) {
    std::size_t count               = 0;
    bool        all_non_polymorphic = true;
    bool        all_concept         = true;
    mp::mp_for_each<comp::KnownReferenceCompositions>([&]<class Entry>(Entry) {
        ++count;
        if (std::is_polymorphic_v<anatomy_of<Entry>>) { all_non_polymorphic = false; }
        if (!cea::AnatomyConcept<anatomy_of<Entry>>) { all_concept = false; }
    });
    EXPECT_EQ(count, 11u);
    EXPECT_TRUE(all_non_polymorphic); // keine konkrete Referenz-Anatomie traegt eine vtable
    EXPECT_TRUE(all_concept);         // alle erfuellen den compile-time-AnatomyConcept-Vertrag
}

TEST(MetaprogStriktheitGuard, ThreeLevelAnatomyIsTotalAndConsistent) {
    // Ebene 2 -> Ebene 1: alle 5 Tier-Unterklassen auf genau eine der 3 Gattungen abgebildet.
    EXPECT_EQ(cea::gattung_of(cea::AnatomyGenus::SearchAlgorithm), cea::AnatomyGattung::SearchAlgorithm);
    EXPECT_EQ(cea::gattung_of(cea::AnatomyGenus::Set), cea::AnatomyGattung::Container);
    EXPECT_EQ(cea::gattung_of(cea::AnatomyGenus::Sequence), cea::AnatomyGattung::Container);
    EXPECT_EQ(cea::gattung_of(cea::AnatomyGenus::Adapter), cea::AnatomyGattung::Container);
    EXPECT_EQ(cea::gattung_of(cea::AnatomyGenus::View), cea::AnatomyGattung::Container);
}

TEST(MetaprogStriktheitGuard, AbiBoundaryIsDeliberatelyPolymorphic) {
    // Positiv-Beweis: die Interface-Schicht IST bewusst polymorph — der einzige erlaubte vtable-Ort.
    EXPECT_TRUE(std::is_polymorphic_v<cea::IAnatomyBase>);
    EXPECT_TRUE(std::is_polymorphic_v<cea::IDriveableTier>);
    EXPECT_TRUE(std::is_polymorphic_v<cea::IMeasurableWorkload>);
}
