// test_planner_version_flag_grammatik.cpp -- CX-W5 Negativ-Probe (Codex-Doppelreview 02.08.2026),
// FORTGESCHRIEBEN auf die FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026).
//
// WAS DIE v2 AN DIESEM TEST GEAENDERT HAT -- und warum es nicht bloss ein Umschreiben war:
//   * Das 'v'-PRAEFIX IST ENTFALLEN (Owner-E4 Frage 5: "Ja das v faellt jetzt"). Der Test hiess deshalb
//     RawLiteralCarriesVPrefix und pruefte GENAU DAS GEGENTEIL dessen, was heute gilt. Er ist umbenannt und
//     dreht die Aussage um: das Roh-Literal traegt KEIN 'v' mehr, und rohe wie gerenderte Form fallen
//     zusammen.
//   * Die Mutations-Probe stand auf "'e' ist an einer ce-eigenen Version NIE zulaessig". Diese Regel gibt es
//     nicht mehr ('e' == EFFICIENCY CORE). Sie ist ERSETZT durch die Regel, die an ihre Stelle getreten ist
//     (Owner-F-10 "traegt mindestens 'c'") plus die Alt-Schreibweise als unparsbaren Fall -- ein bloss
//     umgeschriebenes 'e'-Literal waere gruen geblieben und haette eine tote Regel bezeugt.
//
// **Befund CX-W5:** kPlannerVersion war "1.0.0" -- ein FUENFTER ce-Versions-Stempelpfad, der (a) im Roh-Literal
// KEIN 'v' trug (Owner-Q10 verlangt es dort), (b) KEIN Hardware-Flag trug (Owner-Q3 'c'/'ce'), (c) von keiner
// ce-Politik-Wache erfasst war und (d) in der A13-M2/M3-Migrations-Naht fehlte. Der Fix bringt kPlannerVersion
// auf das Roh-Literal "1.0.0" (Q10), legt die ce-Politik-Wachen (ce_owned_version_is_wellformed + gated
// ce_owned_version_satisfies_cpu_enforce) an und nimmt den Planer in die Migrations-Naht (algo_semver.hpp
// Klasse (e)) auf. A13-M3/C4: die GERENDERTE Zeile lautet "planner@1.0.0.c" -- das 'v'-Praefix faellt weiter
// weg (Owner-Q10), das HARDWARE-FLAG gehoert zur Version und bleibt stehen.
//
// **Negativ-Probe (RED vor Fix / GREEN nach Fix):** PlannerVersionIsWellformedCeVersion prueft
// ce_owned_version_is_wellformed(kPlannerVersion). Vor dem Fix (kPlannerVersion=="1.0.0", ohne 'v') parst das
// Literal auf den Sentinel und ist NICHT wohlgeformt -> EXPECT_TRUE schlaegt fehl (RED). Nach dem Fix
// (kPlannerVersion=="1.0.0", seit A13-M3/C4 "1.0.0.c") ist es wohlgeformt -> GREEN. Die Render-Neutralitaet
// war beim CX-W5-Schritt die Byte-Gegenwache; das 'c' hat C4 als deklariertes Byte-Ereignis nachgezogen.
//
// ADDITIV & golden/binary_id-neutral: reine Lese-Reflexion ueber den Planer-Stempel; kein Achsen-/Registry-/
// golden-/ABI-Byte beruehrt.

#include <cache_engine/measurement/algo_semver.hpp>
#include <profile_facade/planner/planner_version.hpp>

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace planner = ::comdare::cache_engine::planner;
namespace meas    = ::comdare::cache_engine::measurement;

// KERN-PROBE: der Planer-Stempel ist eine wohlgeformte ce-EIGENE Version.
TEST(PlannerVersionFlagGrammatik, PlannerVersionIsWellformedCeVersion) {
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed(planner::kPlannerVersion))
        << "kPlannerVersion='" << planner::kPlannerVersion
        << "' ist keine wohlgeformte ce-Version (Flag-Grammatik v2: 'X.Y.Z' plus null bis n punkt-getrennte "
           "Flags; traegt sie Flags, MUSS 'c' darunter sein).";
}

// FLAG-GRAMMATIK v2 R1: das Roh-Literal traegt KEIN 'v'-Praefix mehr. Dieser Test hiess bis zum Owner-KERN
// vom 07.08.2026 RawLiteralCarriesVPrefix und behauptete das Gegenteil.
TEST(PlannerVersionFlagGrammatik, RawLiteralHatKeinVPraefixMehr) {
    EXPECT_EQ(planner::kPlannerVersion, std::string_view{"1.0.0.c"});
    EXPECT_EQ(planner::kPlannerVersion.find('v'), std::string_view::npos);
    EXPECT_FALSE(meas::parse_algo_semver(planner::kPlannerVersion).is_sentinel())
        << "das Roh-Literal muss parsbar sein (kein Sentinel).";
    EXPECT_TRUE(meas::parse_algo_semver(planner::kPlannerVersion).has_top_level_flag("c"));
}

// BYTE-WACHE: rohe und gerenderte Form FALLEN ZUSAMMEN (R1) -- der Renderer ist fuer ein wohlgeformtes
// Literal die Identitaet. Die --dump-plan-Zeile lautet "planner@1.0.0.c ...".
TEST(PlannerVersionFlagGrammatik, RenderedStampFaelltMitDemRohLiteralZusammen) {
    EXPECT_EQ(meas::algo_semver_string(planner::kPlannerVersion), std::string{"1.0.0.c"});
    std::string const stamp = planner::planner_version_stamp();
    EXPECT_EQ(stamp.rfind("planner@1.0.0.c isa=", 0), 0u) << "gerenderte Zeile nicht wie erwartet: '" << stamp << "'";
    EXPECT_EQ(stamp.find("planner@v"), std::string::npos) << "die gerenderte Form traegt nie ein 'v'.";
}

// Die Wache BEISST: Mutationen, die den Fix verletzen wuerden, sind als ce-Version ABGEWIESEN. (Belegt die
// Trennschaerfe der Politik-Funktion, die die static_assert in planner_version.hpp scharf schaltet.)
TEST(PlannerVersionFlagGrammatik, PolitikWeistAltformFlagloseUndFremdflaggeAb) {
    // (1) Die ALTE Q3-Schreibweise ist unparsbar -- keine Uebergangs-Toleranz. Eine static_assert-Mutation
    //     auf "v1.0.0c" braeche den Bau von planner_version.hpp.
    EXPECT_FALSE(meas::ce_owned_version_is_wellformed("v1.0.0c"));
    EXPECT_FALSE(meas::ce_owned_version_is_wellformed("v1.0.0ce"));
    EXPECT_FALSE(meas::ce_owned_version_is_wellformed("1.0.0c")); // Flag ohne fuehrenden Punkt (R2)
    // (2) Owner-F-10: Flags OHNE CPU-Basis sind an einer ce-eigenen Version unzulaessig.
    EXPECT_FALSE(meas::ce_owned_version_is_wellformed("1.0.0.g"));
    EXPECT_FALSE(meas::ce_owned_version_is_wellformed("1.0.0.x512{f}"));
    // (3) flaglos erfuellt die ENFORCE-Pflicht NICHT, die Bestands-Form sehr wohl.
    EXPECT_FALSE(meas::ce_owned_version_satisfies_cpu_enforce("1.0.0"));
    EXPECT_TRUE(meas::ce_owned_version_satisfies_cpu_enforce("1.0.0.c"));
    // (4) GEGENPROBE zur Bedeutungsaenderung von 'e': ein efficiency-core-Flag ist LEGITIM. Hier stand bis
    //     zur v2 das Gegenteil ("'e' ist an einer ce-eigenen Version NIE zulaessig").
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed("1.0.0.c{e}"));
    EXPECT_TRUE(meas::ce_owned_version_satisfies_cpu_enforce("1.0.0.c{p.e}"));
}
