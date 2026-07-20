// Querschnitt M -- W12-A (Section43): X.Y.Z-Parse-Helfer (Inkrement 2) + Planer-Selbst-Stempel (Inkrement 3).
// Leichte TU (keine Registries): verifiziert die isolierten Stempel-Bausteine + ihre Byte-Trennung zur .algos-Welt.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // W12-A3: AnatomyVersionLines-POD-Layout-Wache
#include <cache_engine/abi/anatomy_version_stamp.hpp>
#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/axis_version_stamp.hpp>
#include <profile_facade/planner/planner_version.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace m  = ::comdare::cache_engine::measurement;
namespace pl = ::comdare::cache_engine::planner;

namespace {
// Mock-Achsen (name() + algo_version) + Mock-Composition (17 benannte Aliase) fuer organ_stamp_line<Comp>().
struct MockAxisV1 {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "algoA"; }
    static constexpr std::string_view               algo_version = "v1";
};
struct MockAxisV234 {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "algoB"; }
    static constexpr std::string_view               algo_version = "v2.3.4";
};
struct MockComposition {
    using search_algo        = MockAxisV1;
    using cache_traversal    = MockAxisV1;
    using mapping            = MockAxisV1;
    using path_compression   = MockAxisV1;
    using node_type          = MockAxisV1;
    using memory_layout      = MockAxisV1;
    using allocator          = MockAxisV1;
    using prefetch           = MockAxisV1;
    using concurrency        = MockAxisV1;
    using serialization      = MockAxisV1;
    using value_handle       = MockAxisV1;
    using index_organization = MockAxisV1;
    using io_dispatch        = MockAxisV1;
    using migration_policy   = MockAxisV1;
    using filter             = MockAxisV234; // abweichende X.Y.Z-Voll-Form
    using queuing_q1         = MockAxisV1;
    using queuing_q2         = MockAxisV1;
};
} // namespace

TEST(MW12StampBausteine, AlgoSemVerParsesVnAndVnnnOnly) {
    // Entscheid W12-A-2: NUR "vN" und "vN.N.N"; Kurzformen/Fremdformen -> Sentinel {0,0,0}.
    EXPECT_EQ(m::parse_algo_semver("v1"), (m::AlgoSemVer{1, 0, 0}));
    EXPECT_EQ(m::parse_algo_semver("v2.3.4"), (m::AlgoSemVer{2, 3, 4}));
    EXPECT_EQ(m::parse_algo_semver("v0"), (m::AlgoSemVer{0, 0, 0}));
    EXPECT_EQ(m::parse_algo_semver("v1.2"), (m::AlgoSemVer{0, 0, 0}));  // Kurzform verboten
    EXPECT_EQ(m::parse_algo_semver("1.0.0"), (m::AlgoSemVer{0, 0, 0})); // ohne 'v'
    EXPECT_EQ(m::parse_algo_semver(""), (m::AlgoSemVer{0, 0, 0}));
}

TEST(MW12StampBausteine, AlgoSemVerFullFormForStampsOnly) {
    // Die X.Y.Z-VOLL-Form fuer Stempel/Registry -- der heutige "v1"-Stand aller Algos wird zu "1.0.0".
    EXPECT_EQ(m::algo_semver_string("v1"), std::string{"1.0.0"});
    EXPECT_EQ(m::algo_semver_string("v2.3.4"), std::string{"2.3.4"});
    EXPECT_EQ(m::algo_semver_string("v0"), std::string{"0.0.0"});
    // Byte-Trennung zur .algos-Welt: die Voll-Form ist NICHT der rohe algo_version-String.
    EXPECT_NE(m::algo_semver_string("v1"), std::string{"v1"});
}

TEST(MW12StampBausteine, AxisVersionStampLineUsesFullSemverAndCanonicalOrder) {
    // Stempel-Zeile "achse=algorithmus@X.Y.Z;..." in Eingabe- (== compose-) Reihenfolge, Voll-Form via algo_semver.
    std::array<m::AxisVersionEntry, 2> const entries{{{"search_algo", "bst", "v1"}, {"filter", "bloom", "v2.3.4"}}};
    std::string const                        line = m::build_axis_version_stamp_line(entries);
    EXPECT_EQ(line, std::string{"search_algo=bst@1.0.0;filter=bloom@2.3.4"});
    // SEPARATE Welt zur .algos-Sig: der Stempel traegt X.Y.Z, NICHT die rohe "@v1"-Form.
    EXPECT_EQ(line.find("@v1"), std::string::npos);
    EXPECT_NE(line.find("@1.0.0"), std::string::npos);
    // leere Eingabe -> leere Zeile.
    EXPECT_TRUE(m::build_axis_version_stamp_line(std::span<m::AxisVersionEntry const>{}).empty());
}

TEST(MW12StampBausteine, OrganStampLineDerivesAll17AxesInCanonicalOrder) {
    std::string const line = ::comdare::cache_engine::abi::organ_stamp_line<MockComposition>();
    // 17 Slots -> exakt 16 Trenner ';'.
    EXPECT_EQ(std::count(line.begin(), line.end(), ';'), 16);
    // kanonische Ordnung: beginnt mit search_algo, endet mit queuing_q2.
    EXPECT_EQ(line.rfind("search_algo=algoA@1.0.0", 0), 0u);
    EXPECT_NE(line.find(";queuing_q2=algoA@1.0.0"), std::string::npos);
    // filter (Slot 15) traegt die abweichende X.Y.Z-Voll-Form:
    EXPECT_NE(line.find(";filter=algoB@2.3.4;"), std::string::npos);
    // SEPARATE Welt zur .algos-Sig: KEINE rohe "@v1"-Form im Stempel.
    EXPECT_EQ(line.find("@v1"), std::string::npos);
}

TEST(MW12StampBausteine, SystemStampLineIsStaticSystemAxisAlgoVersions) {
    // Entscheid W12-A-1: statische System-Achsen-Algo-Versionen (nicht die gewaehlten Zellwerte; W10-Anschluss).
    std::string const line = ::comdare::cache_engine::abi::system_stamp_line();
    EXPECT_EQ(std::count(line.begin(), line.end(), ';'), 4); // 5 System-Haupt-Achsen -> 4 Trenner
    EXPECT_EQ(line.rfind("compiler=code@1.0.0", 0), 0u);
    EXPECT_NE(line.find(";extension_hardware=code@1.0.0"), std::string::npos);
    EXPECT_NE(line.find(";load_framework=code@1.0.0"), std::string::npos);
    EXPECT_EQ(line.find("@v1"), std::string::npos); // separate Welt zur .algos-Sig
}

TEST(MW12StampBausteine, MeasurementStampLineCarriesOnlyToolingMain) {
    // W12-A3 (Section 43, Section 47): der Mess-Stempel traegt GENAU die gewaehlte Mess-Tooling-HAUPT-Wahl als EINEN
    // Eintrag "measurement_tooling=<tooling>@X.Y.Z" -- Voll-Form, SEPARATE Welt zur .algos-Sig, NUR die Haupt-Achse.
    std::string const line = ::comdare::cache_engine::abi::measurement_stamp_line("wallclock");
    EXPECT_EQ(line, std::string{"measurement_tooling=wallclock@1.0.0"});
    // Genau EINE Haupt-Achse -> KEIN ';'-Trenner (Ablaufmethodik/Workloads sind UNTER -> nie Bestandteil).
    EXPECT_EQ(std::count(line.begin(), line.end(), ';'), 0);
    EXPECT_EQ(line.find("@v1"), std::string::npos); // separate Welt zur .algos-Sig (X.Y.Z, nicht roh)
    // Andere Tooling-Haupt-Wahlen materialisieren analog.
    EXPECT_EQ(::comdare::cache_engine::abi::measurement_stamp_line("macro"),
              std::string{"measurement_tooling=macro@1.0.0"});
    EXPECT_EQ(::comdare::cache_engine::abi::measurement_stamp_line("micro"),
              std::string{"measurement_tooling=micro@1.0.0"});
    // Leere Wahl -> leere Zeile (ehrlich: kein Mess-Tooling einkompiliert).
    EXPECT_TRUE(::comdare::cache_engine::abi::measurement_stamp_line("").empty());
}

TEST(MW12StampBausteine, AnatomyVersionLinesPodLayoutIsStableAt56) {
    // W12-A3 Version-Line-Gate: kMeasurementAxisVersionLine ist als measurement_line/measurement_len ANS ENDE des
    // AnatomyVersionLines-POD angehaengt (append-only; organ_/system_-Offsets stabil). Der Layout-Bump 1 -> 2
    // signalisiert die neuen Trailing-Felder; der sizeof-static_assert lebt in anatomy_module_abi_v1_decl.hpp und
    // haelt build-weit -- hier zusaetzlich als literaler ctest-Beweis gespiegelt.
    using ::comdare::cache_engine::abi::AnatomyVersionLines;
    static_assert(sizeof(AnatomyVersionLines) == 56, "POD-Layout-Wache: 8 Felder, 8-aligned -> 56 Byte (x86_64).");
    static_assert(alignof(AnatomyVersionLines) == 8);
    EXPECT_EQ(sizeof(AnatomyVersionLines), 56u);
    EXPECT_EQ(alignof(AnatomyVersionLines), 8u);
    EXPECT_EQ(::comdare::cache_engine::abi::kAnatomyVersionLinesLayout, 2u);
    // Offset-Stabilitaet: organ_/system_ liegen unveraendert, die Mess-Felder folgen dahinter (append-only).
    EXPECT_EQ(offsetof(AnatomyVersionLines, organ_line), 8u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, system_line), 24u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_line), 40u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_len), 48u);
}

TEST(MW12StampBausteine, PlannerVersionStampCarriesSelfVersionAndIsaOs) {
    EXPECT_EQ(pl::kPlannerVersion, std::string_view{"1.0.0"}); // X.Y.Z initial
    EXPECT_EQ(pl::planner_target_isa(), std::string_view{"x86_64"});
    std::string const stamp = pl::planner_version_stamp();
    EXPECT_NE(stamp.find("planner@1.0.0"), std::string::npos) << "stamp='" << stamp << "'";
    EXPECT_NE(stamp.find("isa=x86_64"), std::string::npos) << "stamp='" << stamp << "'";
    EXPECT_NE(stamp.find("os="), std::string::npos) << "stamp='" << stamp << "'";
}
