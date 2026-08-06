// test_reflect_versions_all_registered.cpp -- CX-W6 Negativ-Probe (Codex-Doppelreview 02.08.2026).
//
// **Befund CX-W6:** die Flag-Grammatik-Wachen (parsbar / nie 'e' / Flag-konform + gated ENFORCE) liefen nur
// ueber die GEFILTERTEN Enabled*-Listen. Deaktiviert-aber-registrierte Varianten (Bestandsfall:
// Array256SearchAlgo, COMDARE_AXIS_03A_ENABLE_ARRAY256=OFF, algo_version "v1.0.0") trugen weiter ein
// algo_version-Literal, das KEINE Wache sah -- ein 'e'/falsches Flag bliebe bis zur spaeteren Aktivierung
// unentdeckt, und der A13-M3/C4-ENFORCE-Commit haette daran nicht angeschlagen. Der Fix zieht die Wache ueber die
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

#include <builder/pruef_dock/pruef_dock_version.hpp> // E-24 C4: die ce-eigenen Dock-Versionen (Klasse III)

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>
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
    // gesunde Untergrenze (der Bestand traegt >100 registrierte Organ-Varianten). Grep-Beleg zum Stand
    // 06.08.2026 (nach dem 2. Bump): 96x "v1.0.0c" + 2x "v1.0.1c" + 24x "v1.0.2c" == 122 -- der
    // A1-Wurf-Vertrag hat zunaechst alle 26 Strategien der Allokator-Achse auf "v1.0.1c" gehoben
    // (1. Bump), danach die 24 Strategien mit eigener reallocate()-Implementierung auf "v1.0.2c"
    // (2. Bump, Owner-Entscheid nach Lens-Pass); PmrResourceAllocator/VampirNfpAllocator (kein eigenes
    // reallocate()) blieben auf "v1.0.1c" stehen (axis_06_allocator_strategy_base.hpp, Abschnitt
    // "A1-VERSIONS-BUMP" / "A1-VERSIONS-BUMP, 2. BUMP"; gepinnt in test_a1_algo_version_pin_alloc_axis).
    // Die frueher hier notierte Zahl "122x v1.0.0c" beschrieb den Stand VOR dem 1. Bump.
    EXPECT_GE(ex::kAllRegisteredOrganVariantCount, 100u);
}

// Der konkrete Bestandsfall: Array256SearchAlgo ist DEAKTIVIERT (nicht in der Enabled-Tabelle), traegt aber ein
// algo_version-Literal -- das jetzt unter DERSELBEN ce-Politik steht wie jede aktive Variante.
TEST(ReflectVersionsAllRegistered, DisabledArray256IsRegisteredAndNowUnderCePolicy) {
    EXPECT_FALSE(::comdare::cache_engine::lookup::Array256SearchAlgo::enabled)
        << "Vorbedingung des Befunds: Array256 ist Default OFF.";
    // seine algo_version steht unter derselben ce-Politik wie jede aktive Variante -- seit A13-M3/C4 traegt
    // auch sie das CPU-Flag ("v1.0.0c" fuer search_algo; die Allokator-Achse steht seit dem A1-Schnitt auf
    // "v1.0.1c"), sonst haette die gated ENFORCE-Wache den Bau gebrochen.
    EXPECT_TRUE(
        meas::ce_owned_version_is_wellformed(::comdare::cache_engine::lookup::Array256SearchAlgo::algo_version));

    // und es taucht NICHT in der Enabled-Tabelle auf (Beleg, dass es ohne den Fix unbewacht war).
    std::vector<ex::AxisVariantVersion> const enabled             = ex::build_axis_variant_version_table();
    bool                                      array256_in_enabled = false;
    for (ex::AxisVariantVersion const& e : enabled)
        if (e.axis == "search_algo" && e.variant == "array256") array256_in_enabled = true;
    EXPECT_FALSE(array256_in_enabled) << "Array256 sollte in der Enabled-Tabelle FEHLEN (Default OFF).";
}

// ------------------------------------------------------------------------------------------------
// E-24 C4 (b/5) -- Bauplan Paragraf 4.3, Klasse III: "test_reflect_versions_all_registered.cpp MUSS
// die neuen Genus-/Dock-Eintraege mitpruefen (Flag-Grammatik-Naht)".
//
// WARUM HIER UND NICHT IN EINER NEUEN TU (Auflage 13, Bestands-TU FORTSCHREIBEN statt umgehen): diese
// TU ist die Stelle, an der die Flag-Grammatik ueber die VOLLE ce-eigene Versions-Population laeuft.
// Die Dock-Versionen sind ce-eigene Versionen (Bauplan Paragraf 4.6 nennt sie ausdruecklich), aber
// KEINE Organ-Achsen-Varianten -- sie stehen deshalb nicht in AllRegisteredOrganAxisLists und waeren
// ohne diesen Block genau die Luecke, die CX-W6 an den deaktivierten Achsen gefunden hat: ein
// Literal, das keine Wache sieht.
//
// Der Bau von pruef_dock_version.hpp traegt die static_asserts bereits; die Laufzeit-Checks hier sind
// die zweite, fixture-unabhaengige Ableitung (Memory-Lehre "gruene Tests zementieren alte Ordnung"):
// die Liste der geprueften Versionen kommt aus pruef_dock_version_for() ueber ALLE fuenf Gattungen,
// nicht aus einer im Test gepflegten Aufzaehlung von Literalen.
// ------------------------------------------------------------------------------------------------
TEST(ReflectVersionsAllRegistered, PruefDockVersionsFollowCeFlagGrammar) {
    namespace pd = ::comdare::cache_engine::builder::pruef_dock;
    namespace an = ::comdare::cache_engine::anatomy;

    constexpr std::array<an::AnatomyGenus, 5> kAllDockGenera{an::AnatomyGenus::SearchAlgorithm, an::AnatomyGenus::Set,
                                                             an::AnatomyGenus::Sequence, an::AnatomyGenus::Adapter,
                                                             an::AnatomyGenus::View};
    for (an::AnatomyGenus const g : kAllDockGenera) {
        std::string_view const raw = pd::pruef_dock_version_for(g);
        SCOPED_TRACE(std::string{an::genus_name(g)});
        // (a) jede Ebene-2-Gattung traegt eine bezifferte Dock-Version (kein stiller Leerstempel).
        ASSERT_FALSE(raw.empty()) << "Gattung ohne Dock-Version -- die Mess-Provenienz waere unbeziffert.";
        // (b) Owner-Q10: das 'v' gehoert ins ROH-Literal.
        EXPECT_EQ(raw.front(), 'v');
        // (c) ce-Politik ungated: wohlgeformt und NIE 'e' (Pruefling-Markierung, Owner-E2).
        EXPECT_TRUE(meas::ce_owned_version_is_wellformed(raw));
        // (d) ce-Politik gated: CPU-only-Flotte -> Flag 'c' (Owner-Q3). Dieselbe Wache, die
        //     COMDARE_VERSION_HW_FLAG_ENFORCE compile-hart zieht -- hier zusaetzlich zur Laufzeit belegt.
        EXPECT_TRUE(meas::ce_owned_version_satisfies_cpu_enforce(raw));
        // (e) Render-Neutralitaet: die GERENDERTE Form ist praefixfrei, behaelt aber das Flag.
        std::string const stamp = pd::pruef_dock_version_stamp(an::genus_name(g), raw);
        EXPECT_EQ(stamp, std::string{an::genus_name(g)} + "@" + std::string{raw.substr(1)});
        EXPECT_EQ(stamp.find("@v"), std::string::npos) << "Praefix 'v' darf NICHT in die gerenderte Form (Q10).";
        EXPECT_EQ(stamp.back(), 'c') << "CPU-only: die gerenderte Version MUSS auf 'c' enden (Q3).";
    }
}
