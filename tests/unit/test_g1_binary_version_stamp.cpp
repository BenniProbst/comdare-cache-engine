// K7b-4 (Section 62-B, G1/B6-Auflage 2026-07-22): der G1-Je-Binary-Selbst-Stempel des Treiber-Binary (Planer- +
// CEB-Rolle in EINEM Binary). Leichte TU (keine Registries): pinnt das Format der vier gelabelten Stempel-Zeilen und
// beweist, dass JEDES Feld non-empty ist. Der system_build_version-Anteil wird HEREINGEREICHT (Single-Source
// system_axes_version_suffix, TU-lokal in der Facade) -> hier via repraesentativem Test-Suffix simuliert.

#include "support/oeb_stempel_zeilen.hpp" // split_lines + OE-B-Stempel-Fixture (EINE Quelle, LB-6)

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // COMDARE_ANATOMY_ABI_MAJOR + kCebContractCodegenMinor
#include <profile_facade/build_type_stamp.hpp>             // build_type_version_suffix() (Referenz fuer build-type)
#include <profile_facade/g1_binary_version_stamp.hpp>      // Pruefling: g1_binary_version_block + Helfer
#include <profile_facade/planner/planner_version.hpp>      // planner_version_stamp() (Referenz fuer Zeile 1)
#include <profile_facade/system_version_suffix.hpp>        // compose_system_version_suffix (Suffix-Single-Source, S10)

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace pf = ::comdare::cache_engine::builder::profile_facade;
// Die Suffix-Single-Source liegt in einem ANDEREN Namespace als der G1-Stempel-Header (kein builder::) --
// eigenes Alias, damit die Verwechslung nicht als "pf::" durchrutscht.
namespace svs = ::comdare::cache_engine::profile_facade;
namespace pl  = ::comdare::cache_engine::planner;
namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace ct  = ::comdare::test;

// split_lines() stand bis 2026-08-06 HIER (Zeilen 28-40) im anonymen Namensraum. Seit LB-6 wird derselbe
// Zerleger auch von den beiden Lager-TUs gebraucht (OE-B-Ruecklese) -- er ist deshalb nach
// tests/unit/support/oeb_stempel_zeilen.hpp ausgelagert statt dort ein zweites Mal hingeschrieben.
using ct::split_lines;

TEST(G1BinaryVersionStamp, CebContractVersionIsAbiMajorDotCodegenMinor) {
    // ABI-Major AUTOMATISCH ueber COMDARE_ANATOMY_ABI_MAJOR, codegen-Minor manuell ueber kCebContractCodegenMinor.
    // Bump-neutral aus den Konstanten abgeleitet (nicht hart "6.0"), damit ein ABI-Bump nicht diesen Test bricht.
    std::string const expected = std::to_string(COMDARE_ANATOMY_ABI_MAJOR) + "." +
                                 std::to_string(::comdare::cache_engine::abi::kCebContractCodegenMinor);
    EXPECT_EQ(pf::g1_ceb_contract_version(), expected);
    // Format-Pin: immer non-empty und traegt genau EINEN '.'-Trenner (MAJOR.minor).
    EXPECT_FALSE(pf::g1_ceb_contract_version().empty());
    EXPECT_NE(pf::g1_ceb_contract_version().find('.'), std::string::npos);
}

TEST(G1BinaryVersionStamp, BuildTypeLabelMirrorsSuffixAndIsAlwaysNonEmpty) {
    // g1_build_type_label() macht das leere build_type_version_suffix()-Default explizit sichtbar: leerer Suffix
    // (Release/measure) -> "Release", "+bt=Debug" -> "Debug". Env-robust aus der Suffix-Quelle abgeleitet.
    std::string const label = pf::g1_build_type_label();
    std::string const expected =
        tlz::build_type_version_suffix().empty() ? std::string{"Release"} : std::string{"Debug"};
    EXPECT_EQ(label, expected);
    EXPECT_FALSE(label.empty());
    EXPECT_TRUE(label == "Release" || label == "Debug") << "label='" << label << "'";
}

TEST(G1BinaryVersionStamp, BinaryVersionBlockHasFourLabeledNonEmptyLines) {
    // Repraesentativer, immer non-empty System-Suffix. O-8 Schritt 12: der Suffix wird NICHT mehr als Literal
    // hingeschrieben, sondern aus der Suffix-Single-Source zusammengesetzt (system_version_suffix.hpp, Schritt 10).
    // Grund: der frueher hier stehende Fixture "+ext=avx2+cxx=gcc+opt=O3+ceb=6.0" trug die ALTE Segment-Ordnung und
    // darueber die inzwischen falsche Behauptung "beginnt stets mit '+ext='" -- die bindende Form beginnt mit "+cxx=".
    // Der Test war dabei nie rot (er reicht den Fixture nur durch), und genau das ist die Falle: ein gruener Test,
    // der eine abgeschaffte Ordnung als Referenz zeigt. Aus der Single-Source gebaut kann er nicht wieder driften.
    std::string const sys =
        svs::compose_system_version_suffix({.cxx = "gcc", .opt = "O3", .simd = "avx2", .ceb = "6.0"});
    ASSERT_EQ(sys, "+cxx=gcc+opt=O3+ext=avx2+ceb=6.0") << "bindende Segment-Ordnung (kSuffixSegmentOrder)";
    std::string const              block = pf::g1_binary_version_block(sys);
    std::vector<std::string> const lines = split_lines(block);

    // Vier gelabelte Zeilen, Block "\n"-terminiert (= 4 Trenner, kein Rest dahinter).
    EXPECT_EQ(std::count(block.begin(), block.end(), '\n'), 4);
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(block.back(), '\n');

    // Zeile 1: der Planer-Selbst-Stempel (selbst-gelabelt "planner@...", Referenz = planner_version_stamp()).
    EXPECT_EQ(lines[0], pl::planner_version_stamp());
    EXPECT_EQ(lines[0].rfind("planner@", 0), 0u);
    EXPECT_NE(lines[0].find(" isa="), std::string::npos);
    EXPECT_NE(lines[0].find(" os="), std::string::npos);

    // Zeile 2: ceb-contract=<MAJOR.minor> -- Label + non-empty Wert (== g1_ceb_contract_version()).
    EXPECT_EQ(lines[1], "ceb-contract=" + pf::g1_ceb_contract_version());
    EXPECT_EQ(lines[1].rfind("ceb-contract=", 0), 0u);
    EXPECT_FALSE(lines[1].substr(std::string_view{"ceb-contract="}.size()).empty());

    // Zeile 3: build-type=<Release|Debug> -- Label + non-empty Wert (== g1_build_type_label()).
    EXPECT_EQ(lines[2], "build-type=" + pf::g1_build_type_label());
    EXPECT_EQ(lines[2].rfind("build-type=", 0), 0u);
    EXPECT_FALSE(lines[2].substr(std::string_view{"build-type="}.size()).empty());

    // Zeile 4: build-version=<system-suffix> -- Label + non-empty Wert (== hereingereichter Suffix, Single-Source).
    EXPECT_EQ(lines[3], "build-version=" + sys);
    EXPECT_EQ(lines[3].rfind("build-version=", 0), 0u);
    EXPECT_EQ(lines[3].substr(std::string_view{"build-version="}.size()), sys);
    EXPECT_FALSE(sys.empty());
}

TEST(G1BinaryVersionStamp, BuildVersionLineMirrorsExactlyThePassedSuffix) {
    // Single-Source-Beweis: der Header erfindet die build_version NICHT, er reicht den hereingegebenen Suffix DURCH.
    // Zweiter Fixture, ebenfalls aus der Single-Source (S12): eine ANDERE Belegung inkl. +bt=Debug, damit der
    // Durchreich-Beweis nicht am selben String haengt wie der Test darueber. "no_ext" ist hier ein gewoehnlicher
    // simd-Wert -- die D2.8(ii)-Leer-Regel (no_extension emittiert GAR KEIN +ext=) greift nur bei leerem Glied.
    std::string const other = svs::compose_system_version_suffix(
        {.cxx = "clang", .opt = "O2", .simd = "no_ext", .ceb = "6.0", .build_type = "Debug"});
    ASSERT_EQ(other, "+cxx=clang+opt=O2+ext=no_ext+ceb=6.0+bt=Debug") << "bindende Ordnung inkl. +bt am Ende";
    std::string const block = pf::g1_binary_version_block(other);
    EXPECT_NE(block.find("\nbuild-version=" + other + "\n"), std::string::npos) << "block=\n" << block;
    // Der Planer-/ceb-/build-type-Kopf bleibt vom hereingereichten Suffix unberuehrt (drei feste Kopf-Zeilen davor).
    EXPECT_EQ(block.rfind(pl::planner_version_stamp(), 0), 0u);
}

TEST(G1BinaryVersionStamp, OeBDummyStempelFixtureTraegtDieFormDesEchtenBlocks) {
    // LB-6: die beiden Lager-TUs legen "Binaries" als TEXTDATEIEN mit Stempel-String ein und lesen sie
    // ZEILENWEISE zurueck. Ihr Fixture (support/oeb_stempel_zeilen.hpp) ist literal und maschinen-
    // UNABHAENGIG -- er MUSS es sein, weil ein byte-genau zurueckgelesenes Blatt sonst je Distro anders
    // aussaehe. Ein literaler Fixture driftet aber, sobald der echte Block seine Form aendert, und ein
    // gedrifteter Fixture ist genau der gruene Test, der eine abgeschaffte Ordnung zementiert.
    // DIESER Test ist die Klammer: er laeuft in der EINZIGEN TU, die den echten Erzeuger sieht, und
    // vergleicht FORM gegen FORM -- Zeilenzahl und Label je Position, nicht Werte.
    std::string const sys =
        svs::compose_system_version_suffix({.cxx = "gcc", .opt = "O2", .simd = "avx2", .ceb = "6.0"});
    std::vector<std::string> const echt    = split_lines(pf::g1_binary_version_block(sys));
    std::vector<std::string> const fixture = split_lines(ct::oeb_stempel_block());

    ASSERT_EQ(echt.size(), ct::kOeBStempelZeilenZahl) << "Der echte Block hat seine Zeilenzahl geaendert -- der "
                                                         "OE-B-Fixture muss mitgezogen werden (LB-6).";
    ASSERT_EQ(fixture.size(), ct::kOeBStempelZeilenZahl);
    EXPECT_EQ(ct::oeb_stempel_block().back(), '\n') << "wie der echte Block: '\\n'-terminiert";

    for (std::size_t i = 0; i < ct::kOeBStempelZeilenZahl; ++i) {
        auto const label = ct::kOeBStempelLabels[i];
        EXPECT_TRUE(echt[i].starts_with(label))
            << "echte Zeile " << i << " = '" << echt[i] << "', erwartetes Label '" << label << "'";
        EXPECT_TRUE(fixture[i].starts_with(label)) << "Fixture-Zeile " << i << " = '" << fixture[i] << "'";
        EXPECT_GT(fixture[i].size(), label.size()) << "Ein Label ohne Wert waere kein Stempel";
    }
    // Und die vier Fixture-Zeilen sind die vier EINZELN benannten Konstanten -- kein fuenfter,
    // unbenannter Anhang, der beim zeilenweisen Vergleich unbemerkt mitliefe.
    EXPECT_EQ(fixture[0], ct::kOeBStempelZeile0);
    EXPECT_EQ(fixture[1], ct::kOeBStempelZeile1);
    EXPECT_EQ(fixture[2], ct::kOeBStempelZeile2);
    EXPECT_EQ(fixture[3], ct::kOeBStempelZeile3);
}
