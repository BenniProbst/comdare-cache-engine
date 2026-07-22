// K7b-4 (Section 62-B, G1/B6-Auflage 2026-07-22): der G1-Je-Binary-Selbst-Stempel des Treiber-Binary (Planer- +
// CEB-Rolle in EINEM Binary). Leichte TU (keine Registries): pinnt das Format der vier gelabelten Stempel-Zeilen und
// beweist, dass JEDES Feld non-empty ist. Der system_build_version-Anteil wird HEREINGEREICHT (Single-Source
// system_axes_version_suffix, TU-lokal in der Facade) -> hier via repraesentativem Test-Suffix simuliert.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // COMDARE_ANATOMY_ABI_MAJOR + kCebContractCodegenMinor
#include <profile_facade/build_type_stamp.hpp>             // build_type_version_suffix() (Referenz fuer build-type)
#include <profile_facade/g1_binary_version_stamp.hpp>      // Pruefling: g1_binary_version_block + Helfer
#include <profile_facade/planner/planner_version.hpp>      // planner_version_stamp() (Referenz fuer Zeile 1)

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace pf  = ::comdare::cache_engine::builder::profile_facade;
namespace pl  = ::comdare::cache_engine::planner;
namespace tlz = ::comdare::cache_engine::thesis_lazy;

namespace {
// Zerlegt den "\n"-terminierten Block in seine Zeilen (ohne die abschliessende Leer-Zeile nach dem letzten "\n").
[[nodiscard]] std::vector<std::string> split_lines(std::string const& block) {
    std::vector<std::string> lines;
    std::string              cur;
    for (char const c : block) {
        if (c == '\n') {
            lines.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) lines.push_back(cur); // (defensiv: nicht-terminierter Rest)
    return lines;
}
} // namespace

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
    // Repraesentativer, immer non-empty System-Suffix (system_axes_version_suffix beginnt stets mit "+ext=").
    std::string const              sys   = "+ext=avx2+cxx=gcc+opt=O3+ceb=6.0";
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
    std::string const other = "+ext=no_ext+cxx=clang+opt=O2+ceb=6.0+bt=Debug";
    std::string const block = pf::g1_binary_version_block(other);
    EXPECT_NE(block.find("\nbuild-version=" + other + "\n"), std::string::npos) << "block=\n" << block;
    // Der Planer-/ceb-/build-type-Kopf bleibt vom hereingereichten Suffix unberuehrt (drei feste Kopf-Zeilen davor).
    EXPECT_EQ(block.rfind(pl::planner_version_stamp(), 0), 0u);
}
