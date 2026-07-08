// #29 Schritt 1 (2026-07-08, User-Plan AP-15) -- comdare::container Kopf-Framework.
//
// Beweist additiv (golden/ABI-neutral): die bereits gebaute Container-Gattung (4 Genus Adapter/Set/
// Sequence/View) ist unter EINEM generischen comdare::container-Interface als TYPEN ansprechbar, jeder
// TYP behaelt seinen bisherigen Achsen-Satz. KEIN Umbau von Enum/Anatomien/golden (Neutralitaets-Guard).

#include <anatomy/anatomy_base.hpp>
#include <anatomy/container_framework.hpp>

#include <boost/mp11.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>
#include <type_traits>
#include <vector>

namespace cc  = ::comdare::container;
namespace cea = ::comdare::cache_engine::anatomy;
namespace mp  = boost::mp11;

// ── Trait-Beweise (compile-time; spiegeln die Header-static_asserts als Test-Doku) ──────────────
static_assert(cc::type_count == 4);
static_assert(cc::ContainerType<cea::AnatomyGenus::Set>);
static_assert(!cc::ContainerType<cea::AnatomyGenus::SearchAlgorithm>);

TEST(V29ContainerFramework, FourContainerTypesUnderOneInterface) {
    // Die comdare::container-Typ-Liste iteriert generisch ueber genau die 4 Container-Tier-Unterklassen.
    std::vector<cea::AnatomyGenus> seen;
    std::vector<std::size_t>       slots;
    mp::mp_for_each<cc::type_list>([&](auto tag) {
        constexpr cea::AnatomyGenus G = decltype(tag)::value;
        seen.push_back(G);
        slots.push_back(cc::type_traits<G>::slot_count);
        // Jeder iterierte Typ traegt das Container-Aussen-Interface (Ebene 1).
        EXPECT_EQ(cc::type_traits<G>::gattung, cea::AnatomyGattung::Container);
        // Die Gattungs-Zuordnung des Enums stimmt mit dem Framework ueberein.
        EXPECT_EQ(cea::gattung_of(G), cea::AnatomyGattung::Container);
    });
    ASSERT_EQ(seen.size(), 4u);
    EXPECT_EQ(seen[0], cea::AnatomyGenus::Adapter);
    EXPECT_EQ(seen[1], cea::AnatomyGenus::Set);
    EXPECT_EQ(seen[2], cea::AnatomyGenus::Sequence);
    EXPECT_EQ(seen[3], cea::AnatomyGenus::View);
    // "exakt die bisherigen Container-Achsen": jeder Typ behaelt seinen Slot-Satz (keine Vereinheitlichung).
    EXPECT_EQ(slots, (std::vector<std::size_t>{13u, 15u, 11u, 7u}));
}

TEST(V29ContainerFramework, AxisNamesArePreservedPerType) {
    // Der Achsen-Satz je Typ = exakt die bestehende GenusBindingTraits-Definition (re-exportiert, nicht umgebaut).
    EXPECT_EQ(cc::type_traits<cea::AnatomyGenus::Adapter>::axis_names().size(), 13u);
    EXPECT_EQ(cc::type_traits<cea::AnatomyGenus::Set>::axis_names().size(), 15u);
    EXPECT_EQ(cc::type_traits<cea::AnatomyGenus::Sequence>::axis_names().size(), 11u);
    EXPECT_EQ(cc::type_traits<cea::AnatomyGenus::View>::axis_names().size(), 7u);
    // Beispiel-Beleg: Sequence traegt growth_policy als letzte Achse (SequenceComposition-Growth-Slot).
    EXPECT_EQ(cc::type_traits<cea::AnatomyGenus::Sequence>::axis_names().back(), std::string_view{"growth_policy"});
    EXPECT_EQ(cc::type_traits<cea::AnatomyGenus::Adapter>::axis_names().back(), std::string_view{"inner_container"});
}

TEST(V29ContainerFramework, NeutralityGuardsStayIntact) {
    // Der Anatomie-/ABI-Kern bleibt unberuehrt: die 3 Gattungen + 5 Genus unveraendert; SearchAlgorithm
    // ist KEIN Container-Typ (eigene Gattung). Namen konstant (golden-relevante Strings unangetastet).
    static_assert(!cc::ContainerType<cea::AnatomyGenus::SearchAlgorithm>);
    EXPECT_EQ(cea::gattung_name(cea::AnatomyGattung::Container), std::string_view{"Container"});
    EXPECT_EQ(cea::gattung_name(cea::AnatomyGattung::SearchAlgorithm), std::string_view{"SearchAlgorithm"});
    EXPECT_EQ(cea::gattung_of(cea::AnatomyGenus::SearchAlgorithm), cea::AnatomyGattung::SearchAlgorithm);
}
