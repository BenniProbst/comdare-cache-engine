// test_reflect_versions_all_registered.cpp -- CX-W6 Negativ-Probe (Codex-Doppelreview 02.08.2026).
//
// **Befund CX-W6:** die Flag-Grammatik-Wachen (parsbar / nie 'e' / Flag-konform + gated ENFORCE) liefen nur
// ueber die GEFILTERTEN Enabled*-Listen. Deaktiviert-aber-registrierte Varianten (Bestandsfall:
// Array256SearchAlgo, COMDARE_AXIS_03A_ENABLE_ARRAY256=OFF, algo_version "v1.0.0") trugen weiter ein
// algo_version-Literal, das KEINE Wache sah -- ein 'e'/falsches Flag bliebe bis zur spaeteren Aktivierung
// unentdeckt, und der A13-M2/M3-ENFORCE-Commit schluege daran nicht an. Der Fix zieht die Wache ueber die
// VOLLE registrierte Population (guard_all_registered_organ_versions ueber axes26_registered::R* = All*-Listen).
//
// **Negativ-Probe (RED vor Fix / GREEN nach Fix):** schon das BAUEN dieser TU instanziiert
// guard_all_registered_organ_versions() ueber JEDE registrierte Organ-Variante (der Bau IST die Wache; Muster
// test_reflect_versions_all17). Ein temporaeres "v1.0.0e" an einer DEAKTIVIERTEN Variante (z.B.
// axis_03a_search_algo_array256.hpp) braeche NACH dem Fix den Bau dieser TU MIT dem Typ-Namen; VOR dem Fix lief
// derselbe Bau durch (die deaktivierte Variante war unbewacht). Die Laufzeit-Checks belegen zusaetzlich, dass die
// Voll-Registry-Population ECHT groesser ist als die Enabled-Tabelle -- deaktivierte Varianten sind jetzt drin.
//
// **Separate, schwere TU (bewusst; Muster test_reflect_versions_all17):** axis_variant_version_table.hpp zieht
// registry_to_axis_levels.hpp (alle topic_config_sets). Isoliert gehalten + isoliert retriggerbar.
//
// ADDITIV & golden/ABI-NEUTRAL: reine CT-Wache + read-only-Zaehlung. KEINE Aenderung an Achsen/Registries/
// golden/binary_id/ABI (die Voll-Registry-Wache emittiert NICHTS in compose_algo_signature/compose_organ_stamp_line).

#include <builder/experiment_tree/axis_variant_version_table.hpp> // build_axis_variant_version_table / guard / Zaehler
#include <builder/experiment_tree/axis_path_serialization.hpp>    // kCompositionAxisNames (18 Slots, Single-Source)
#include <cache_engine/measurement/algo_semver.hpp>               // ce_owned_version_is_wellformed
#include <axes/lookup/axis_03a_search_algo_array256.hpp>          // Array256SearchAlgo (Default OFF) -- Bestandsfall

#include <gtest/gtest.h>

#include <vector>

namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace meas = ::comdare::cache_engine::measurement;

// Der KERN-Guard (CT): DIESE Instanziierung feuert assert_version_grammar<W>() je REGISTRIERTER Organ-Variante,
// enable-unabhaengig. Eine Fehlform an einer DEAKTIVIERTEN Variante macht bereits den BAU dieser TU rot.
TEST(ReflectVersionsAllRegistered, GuardInstantiatesOverFullRegisteredPopulation) {
    ex::guard_all_registered_organ_versions(); // no-op zur Laufzeit; der Bau ist die Wache
    SUCCEED();
}

// Die Voll-Registry-Population deckt GENAU die 18 kCompositionAxisNames-Organ-Achsen (Drift-Wache gegen die
// Enabled-Emit-Seite build_axis_variant_version_table).
TEST(ReflectVersionsAllRegistered, RegisteredAxisCountEqualsCompositionAxes) {
    EXPECT_EQ(boost::mp11::mp_size<ex::AllRegisteredOrganAxisLists>::value, ex::kCompositionAxisNames.size());
    EXPECT_EQ(boost::mp11::mp_size<ex::AllRegisteredOrganAxisLists>::value, 18u);
}

// KERN-NACHWEIS des Befunds: die VOLLE registrierte Population ist ECHT groesser als die Enabled-Tabelle --
// es GIBT also deaktiviert-aber-registrierte Varianten, die vor dem Fix unbewacht waren und jetzt bewacht sind.
TEST(ReflectVersionsAllRegistered, FullRegisteredIsStrictlyLargerThanEnabledTable) {
    std::vector<ex::AxisVariantVersion> const enabled = ex::build_axis_variant_version_table();
    ASSERT_FALSE(enabled.empty());
    EXPECT_GT(ex::kAllRegisteredOrganVariantCount, enabled.size())
        << "kein deaktivierter Bestand? dann waere der Befund gegenstandslos -- erwartet: registriert > enabled.";
    // gesunde Untergrenze (der Bestand traegt >100 registrierte Organ-Varianten; grep-Beleg 122x "v1.0.0").
    EXPECT_GE(ex::kAllRegisteredOrganVariantCount, 100u);
}

// Der konkrete Bestandsfall: Array256SearchAlgo ist DEAKTIVIERT (nicht in der Enabled-Tabelle), traegt aber ein
// algo_version-Literal -- das jetzt unter DERSELBEN ce-Politik steht wie jede aktive Variante.
TEST(ReflectVersionsAllRegistered, DisabledArray256IsRegisteredAndNowUnderCePolicy) {
    EXPECT_FALSE(::comdare::cache_engine::lookup::Array256SearchAlgo::enabled)
        << "Vorbedingung des Befunds: Array256 ist Default OFF.";
    // seine algo_version steht jetzt unter der ce-Politik (wohlgeformt, flagloser Uebergangs-Bestand toleriert).
    EXPECT_TRUE(
        meas::ce_owned_version_is_wellformed(::comdare::cache_engine::lookup::Array256SearchAlgo::algo_version));

    // und es taucht NICHT in der Enabled-Tabelle auf (Beleg, dass es ohne den Fix unbewacht war).
    std::vector<ex::AxisVariantVersion> const enabled             = ex::build_axis_variant_version_table();
    bool                                      array256_in_enabled = false;
    for (ex::AxisVariantVersion const& e : enabled)
        if (e.axis == "search_algo" && e.variant == "array256") array256_in_enabled = true;
    EXPECT_FALSE(array256_in_enabled) << "Array256 sollte in der Enabled-Tabelle FEHLEN (Default OFF).";
}
