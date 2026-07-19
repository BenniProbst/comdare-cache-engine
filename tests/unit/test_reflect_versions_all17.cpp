// test_reflect_versions_all17.cpp -- L2 stale-skip-Guard (GN-5, Roadmap 2026-07-19): die
// reflect_versions-Instanziierung ueber ALLE 17 Kompositions-Achsen als Default-ctest.
//
// **Zweck (#50-Stale-Green-Wurzel):** build_axis_variant_version_table() (axis_variant_version_table.hpp)
// ist die universelle compile-time-Durchsetzung "jede registrierte Organ-Variante traegt algo_version" --
// der W::algo_version-Zugriff im mp_for_each ist ill-formed, sobald einer Variante das Member fehlt, und
// bricht die Kompilation MIT dem Typ-Namen. Bisher war der EINZIGE Exerciser die CEB-Runtime (Facade):
// ein Default-ctest-Lauf ohne Facade-Rebuild konnte eine version-lose Variante nie bemerken (stale-skip ->
// stale-green; skaliert mit golden-N ueber alle Achsen-Varianten). Diese TU zieht die Instanziierung in den
// Default-ctest: schon das BAUEN dieser Test-Binary ist der Guard; die Laufzeit-Checks belegen zusaetzlich
// die 17-Achsen-Vollstaendigkeit gegen kCompositionAxisNames (Single-Source der Slot-Reihenfolge).
//
// **Separate, schwere TU (bewusst; Muster test_striktheit_axes_guard):** axis_variant_version_table.hpp
// zieht registry_to_axis_levels.hpp (die 23 topic_config_sets). Isoliert gehalten + isoliert retriggerbar.
//
// **ADDITIV & golden/ABI-NEUTRAL:** reine Instanziierung + read-only-Reflexion ueber Bestands-Registries.
// KEINE Aenderung an Achsen/Registries/golden/binary_id/ABI.

#include <builder/experiment_tree/axis_variant_version_table.hpp> // build_axis_variant_version_table / lookup / compose
#include <builder/experiment_tree/axis_path_serialization.hpp>    // kCompositionAxisNames (17 Slots, Single-Source)

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;

// Der Kern-Guard: DIESE Zeile instanziiert reflect_versions ueber alle 17 Kompositions-Achsen-Registries.
// Eine Organ-Variante ohne algo_version macht bereits den BUILD dieser TU rot (nicht erst den Testlauf).
TEST(ReflectVersionsAll17, TableCoversAll17CompositionAxesInCanonicalOrder) {
    std::vector<ex::AxisVariantVersion> const table = ex::build_axis_variant_version_table();
    ASSERT_FALSE(table.empty());

    // Erst-Auftritts-Reihenfolge der Achsen in der Tabelle == kCompositionAxisNames (17, kanonisch).
    std::vector<std::string_view> axes_in_order;
    for (ex::AxisVariantVersion const& e : table) {
        if (axes_in_order.empty() || axes_in_order.back() != e.axis) axes_in_order.push_back(e.axis);
    }
    ASSERT_EQ(axes_in_order.size(), ex::kCompositionAxisNames.size()); // exakt 17, keine Doppel-Bloecke
    for (std::size_t i = 0; i < ex::kCompositionAxisNames.size(); ++i) {
        EXPECT_EQ(axes_in_order[i], ex::kCompositionAxisNames[i])
            << "Slot " << i << ": Tabellen-Reihenfolge weicht von kCompositionAxisNames ab.";
    }
}

TEST(ReflectVersionsAll17, EveryVariantCarriesNonEmptyNameAndVersion) {
    std::vector<ex::AxisVariantVersion> const table = ex::build_axis_variant_version_table();
    for (ex::AxisVariantVersion const& e : table) {
        EXPECT_FALSE(e.variant.empty()) << "Achse '" << e.axis << "': Variante ohne name().";
        EXPECT_FALSE(e.version.empty()) << "Achse '" << e.axis << "', Variante '" << e.variant
                                        << "': leeres algo_version (stale-skip-Risiko #50).";
    }
}

TEST(ReflectVersionsAll17, LookupHitsRegisteredAndMissesUnknown) {
    std::vector<ex::AxisVariantVersion> const table = ex::build_axis_variant_version_table();
    ASSERT_FALSE(table.empty());
    // Treffer: der allererste Tabellen-Eintrag ist per Definition nachschlagbar.
    EXPECT_EQ(ex::lookup_algo_version(table, table.front().axis, table.front().variant), table.front().version);
    // Fehlschlag ist EHRLICH leer (der Aufrufer emittiert @v0-Sentinel, raet nie).
    EXPECT_TRUE(ex::lookup_algo_version(table, "search_algo", "gibt_es_nicht").empty());
    EXPECT_TRUE(ex::lookup_algo_version(table, "keine_achse", table.front().variant).empty());
}

TEST(ReflectVersionsAll17, ComposeSignatureUsesTableAndValuesetTail) {
    std::vector<ex::AxisVariantVersion> const table = ex::build_axis_variant_version_table();
    ASSERT_FALSE(table.empty());
    // Eine minimale Spec aus dem ERSTEN registrierten Eintrag: die Signatur muss dessen echte Version tragen
    // (kein @v0-Sentinel) und mit dem Sub-Achsen-Werteset-Schwanz enden (Bauplan §2).
    std::vector<std::pair<std::string, std::string>> const axes = {
        {std::string{table.front().axis}, table.front().variant}};
    std::string const sig = ex::compose_algo_signature(axes, table);
    std::string const expected_slot =
        std::string{table.front().axis} + "=" + table.front().variant + "@" + table.front().version;
    EXPECT_NE(sig.find(expected_slot), std::string::npos) << "sig='" << sig << "'";
    EXPECT_NE(sig.find(";sub=cacheline@v"), std::string::npos) << "sig='" << sig << "'";
}
