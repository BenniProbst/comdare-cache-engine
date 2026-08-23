// O2-STANDARD-Default-Kette (Owner-Entscheid 21.08.2026: "O2 ist Standard fuer alle Builds,
// O3 wird unter Warnung angeboten"; supersediert das Ruling 2026-07-18 "CEB-Default = O3",
// s. system_axes/optimization_level_sub_axis.hpp Kopf + UEBERHOLT-Nachtrag).
//
// T-2-Aussage: der bewegliche CEB-Default (DefaultOptLevelOption) ist die O2-Auspraegung.
// T-3 Nenner fremd: die erwarteten Literale ("O2", "-O2", "/O2") sind IM TEST EINGEFROREN
// und werden NICHT aus dem Pruefling gezogen -- eine Drift des Defaults trifft auf einen
// unabhaengigen Massstab. T-4 Gegeneingang: O3 bleibt waehlbare, valide Auspraegung der
// Unter-Achse; das Vokabular {O0,O1,O2,O3,Ofast} bleibt voll (kAllOptLevelIds-Nenner 5).
// Die "unter Warnung"-Haelfte des Entscheids lebt in der Haus-Bauwelt
// (cmake/compiler_flags.cmake, COMDARE_OPT_O3=ON mit Configure-WARNING) und ist dort
// Configure-zeitlich; sie ist hier bewusst NICHT Gegenstand (kein CMake-Prozess-Test).
//
// Der Default wird mit Laufzeit-EXPECTs geprueft, mit Absicht KEIN static_assert: der Test
// musste vor dem Default-Dreh literal ROT LAUFEN koennen (T-1 rot-zuerst), nicht den Bau
// brechen. Die CT-Wachen auf den Default tragen optimization_level_sub_axis.hpp selbst und
// test_striktheit_axis_dach_guard.cpp.

#include <system_axes/optimization_level_sub_axis.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <string_view>

namespace cem = ::comdare::cache_engine::measurement;

// T-2: die Default-Kette der opt_level-Unter-Achse steht auf O2 -- id, alle drei
// Compiler-Dialekt-Flags und die IEEE-754-Determinismus-Zusage.
TEST(O2StandardOptDefault, DefaultOptLevelIstO2) {
    EXPECT_EQ(cem::DefaultOptLevelOption::opt_level_id(), std::string_view{"O2"});
    EXPECT_EQ(cem::DefaultOptLevelOption::gcc_opt_flag(), std::string_view{"-O2"});
    EXPECT_EQ(cem::DefaultOptLevelOption::clang_opt_flag(), std::string_view{"-O2"});
    EXPECT_EQ(cem::DefaultOptLevelOption::msvc_opt_flag(), std::string_view{"/O2"});
    EXPECT_TRUE(cem::DefaultOptLevelOption::is_ieee754_deterministic());
}

// T-4: O3 bleibt vollwertig waehlbar (Concept-konform, Flags unveraendert) und steht
// weiter im Vokabular; auch Ofast bleibt als Vergleichs-Extrem gelistet. Der Nenner 5
// ist eingefroren -- ein stilles Schrumpfen des Design-Space-Vokabulars wird rot.
TEST(O2StandardOptDefault, GegeneingangO3BleibtWaehlbar) {
    static_assert(cem::OptimizationLevelSubAxisConcept<cem::OptO3Option>);
    EXPECT_EQ(cem::OptO3Option::opt_level_id(), std::string_view{"O3"});
    EXPECT_EQ(cem::OptO3Option::gcc_opt_flag(), std::string_view{"-O3"});
    EXPECT_EQ(cem::OptO3Option::clang_opt_flag(), std::string_view{"-O3"});
    EXPECT_TRUE(cem::OptO3Option::is_ieee754_deterministic());
    ASSERT_EQ(cem::kAllOptLevelIds.size(), 5u);
    auto const enthalten = [](std::string_view id) {
        return std::find(cem::kAllOptLevelIds.begin(), cem::kAllOptLevelIds.end(), id) != cem::kAllOptLevelIds.end();
    };
    EXPECT_TRUE(enthalten(std::string_view{"O2"}));
    EXPECT_TRUE(enthalten(std::string_view{"O3"}));
    EXPECT_TRUE(enthalten(std::string_view{"Ofast"}));
}
