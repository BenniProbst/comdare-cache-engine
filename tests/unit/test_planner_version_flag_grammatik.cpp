// test_planner_version_flag_grammatik.cpp -- CX-W5 Negativ-Probe (Codex-Doppelreview 02.08.2026).
//
// **Befund CX-W5:** kPlannerVersion war "1.0.0" -- ein FUENFTER ce-Versions-Stempelpfad, der (a) im Roh-Literal
// KEIN 'v' trug (Owner-Q10 verlangt es dort), (b) KEIN Hardware-Flag trug (Owner-Q3 'c'/'ce'), (c) von keiner
// ce-Politik-Wache erfasst war und (d) in der A13-M2/M3-Migrations-Naht fehlte. Der Fix bringt kPlannerVersion
// auf das Roh-Literal "v1.0.0" (Q10), legt die ce-Politik-Wachen (ce_owned_version_is_wellformed + gated
// ce_owned_version_satisfies_cpu_enforce) an und nimmt den Planer in die Migrations-Naht (algo_semver.hpp
// Klasse (e)) auf. A13-M3/C4: die GERENDERTE Zeile lautet "planner@1.0.0c" -- das 'v'-Praefix faellt weiter
// weg (Owner-Q10), das HARDWARE-FLAG gehoert zur Version und bleibt stehen.
//
// **Negativ-Probe (RED vor Fix / GREEN nach Fix):** PlannerVersionIsWellformedCeVersion prueft
// ce_owned_version_is_wellformed(kPlannerVersion). Vor dem Fix (kPlannerVersion=="1.0.0", ohne 'v') parst das
// Literal auf den Sentinel und ist NICHT wohlgeformt -> EXPECT_TRUE schlaegt fehl (RED). Nach dem Fix
// (kPlannerVersion=="v1.0.0") ist es wohlgeformt -> GREEN. Die Render-Neutralitaet ist die Byte-Gegenwache.
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

// CX-W5 KERN-NEGATIV-PROBE: der Planer-Stempel ist eine wohlgeformte ce-EIGENE Version. Vor dem Fix ("1.0.0"
// ohne 'v') RED, nach dem Fix ("v1.0.0") GREEN.
TEST(PlannerVersionFlagGrammatik, PlannerVersionIsWellformedCeVersion) {
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed(planner::kPlannerVersion))
        << "kPlannerVersion='" << planner::kPlannerVersion
        << "' ist keine wohlgeformte ce-Version (Owner-Q10: Roh-Literal traegt 'v'; Owner-Q3: kein 'e', "
           "nur 'c'/'ce' als HW-Flag).";
}

// Owner-Q10: das ROH-Literal traegt das 'v' (nur im Code, nie in der gerenderten Ausgabe).
// A13-M3/C4: zusaetzlich das CPU-Flag (Owner-Q3) -- das 'v' faellt beim Rendern weg, das 'c' NICHT.
TEST(PlannerVersionFlagGrammatik, RawLiteralCarriesVPrefix) {
    EXPECT_EQ(planner::kPlannerVersion, std::string_view{"v1.0.0c"});
    EXPECT_FALSE(meas::parse_algo_semver(planner::kPlannerVersion).is_sentinel())
        << "das Roh-Literal muss parsbar sein (kein Sentinel).";
}

// BYTE-WACHE (Owner-Q10): die --dump-plan-Zeile bleibt PRAEFIXFREI. A13-M3/C4: sie traegt jetzt
// "planner@1.0.0c ..." -- das 'v' schneidet algo_semver_string weg, das HARDWARE-FLAG gehoert zur Version
// selbst und bleibt stehen (deklariertes Byte-Ereignis des Migrations-Commits, KEIN Q10-Verstoss).
TEST(PlannerVersionFlagGrammatik, RenderedStampIsPrefixFreeAndCarriesHwFlag) {
    EXPECT_EQ(meas::algo_semver_string(planner::kPlannerVersion), std::string{"1.0.0c"});
    std::string const stamp = planner::planner_version_stamp();
    EXPECT_EQ(stamp.rfind("planner@1.0.0c isa=", 0), 0u)
        << "gerenderte Zeile nicht wie erwartet praefixfrei+flagbehaftet: '" << stamp << "'";
    EXPECT_EQ(stamp.find("planner@v"), std::string::npos) << "die gerenderte Form darf KEIN 'v' tragen (Owner-Q10).";
}

// Die Wache BEISST: Mutationen, die den Fix verletzen wuerden, sind als ce-Version ABGEWIESEN. (Belegt die
// Trennschaerfe der Politik-Funktion, die die static_assert in planner_version.hpp scharf schaltet.)
TEST(PlannerVersionFlagGrammatik, PolicyRejectsExperimentalAndFlaglessUnderEnforce) {
    // 'e' (Pruefling-Markierung) ist an einer ce-eigenen Version NIE zulaessig -> die static_assert-Mutation
    // 'v1.0.0e' braeche den Bau von planner_version.hpp.
    EXPECT_FALSE(meas::ce_owned_version_is_wellformed("v1.0.0e"));
    EXPECT_FALSE(meas::ce_owned_version_is_wellformed("v1.0.0ce"));
    // flaglos erfuellt die ENFORCE-Pflicht NICHT (im M2/M3-Fenster braeche der gated static_assert), waehrend
    // die Ziel-Form 'v1.0.0c' sie erfuellt.
    EXPECT_FALSE(meas::ce_owned_version_satisfies_cpu_enforce("v1.0.0"));
    EXPECT_TRUE(meas::ce_owned_version_satisfies_cpu_enforce("v1.0.0c"));
}
