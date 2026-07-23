// Querschnitt M -- W12-A (Section43): X.Y.Z-Parse-Helfer (Inkrement 2) + Planer-Selbst-Stempel (Inkrement 3).
// Leichte TU (keine Registries): verifiziert die isolierten Stempel-Bausteine + ihre Byte-Trennung zur .algos-Welt.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // W12-A3: AnatomyVersionLines-POD-Layout-Wache
#include <cache_engine/abi/anatomy_fingerprint.hpp> // K7b-3: anatomy_fingerprint_hex (SHA-512 der 4 Stempel-Zeilen)
#include <cache_engine/abi/anatomy_version_stamp.hpp>
#include <cache_engine/abi/system_axis_code_versions.hpp>            // A2: kSystemAxisCodeVersions (Single-Source)
#include <cache_engine/measurement/measurement_tooling_registry.hpp> // A2: version-Feld + tooling_version_for_id
#include <sha512/ctsha512.hpp> // K7b-3: Referenz-SHA-512 fuer den Fingerprint-Korrektheitstest
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

TEST(MW12StampBausteine, MeasurementStampLineSetFormCarriesToolingMenge) {
    // K7b-2 (Section 64-D1-B, 2026-07-22): die MENGEN-Form -- N Tools -> N ';'-getrennte
    // measurement_tooling=<t>@1.0.0-Eintraege (Eingabe-Reihenfolge, Section-64-Vollmengen-Provenienz). Additive
    // span-Ueberladung; die Einzel-Form oben bleibt unveraendert (der [all]/from_env-LIVE-Pfad routet ueber die Menge).
    namespace abi                            = ::comdare::cache_engine::abi;
    std::array<std::string_view, 2> const tw = {"wallclock", "macro"};
    EXPECT_EQ(abi::measurement_stamp_line(std::span<std::string_view const>{tw}),
              std::string{"measurement_tooling=wallclock@1.0.0;measurement_tooling=macro@1.0.0"});
    // Leere Tokens werden uebersprungen (ehrlich: kein Tool an der Stelle).
    std::array<std::string_view, 3> const gappy = {"wallclock", "", "micro"};
    EXPECT_EQ(abi::measurement_stamp_line(std::span<std::string_view const>{gappy}),
              std::string{"measurement_tooling=wallclock@1.0.0;measurement_tooling=micro@1.0.0"});
    // Leere Menge -> leere Zeile.
    EXPECT_TRUE(abi::measurement_stamp_line(std::span<std::string_view const>{}).empty());
    // Die Vollmenge = das volle Registry-Angebot {wallclock,macro,micro} in Registry-Reihenfolge (Single-Source).
    EXPECT_EQ(
        abi::measurement_stamp_line_full_set(),
        std::string{
            "measurement_tooling=wallclock@1.0.0;measurement_tooling=macro@1.0.0;measurement_tooling=micro@1.0.0"});
    // SEPARATE Welt zur .algos-Sig: X.Y.Z-Voll-Form, NICHT die rohe "@v1".
    EXPECT_EQ(abi::measurement_stamp_line_full_set().find("@v1"), std::string::npos);
}

TEST(MW12StampBausteine, AnatomyVersionLinesPodLayoutIsStableAt88) {
    // K7a (Section 59): kMergeAxisVersionLine als merge_line/merge_len ans POD-Ende (append-only, dritter Tier-Stempel).
    // K7b-3 (Section 62-B, 2026-07-22): sha512_line/sha512_len ans POD-Ende angehaengt = SHA-512-Fingerprint der 4
    // Stempel-Zeilen (5. Feld-Paar). Der Layout-Bump 3 -> 4 signalisiert das neue Trailing-Feld; die
    // organ_/system_/measurement_/merge_-Offsets bleiben stabil (append-only). Der sizeof-static_assert lebt in
    // anatomy_module_abi_v1_decl.hpp und haelt build-weit -- hier zusaetzlich als literaler ctest-Beweis gespiegelt.
    // binary_id/CRC UNBERUEHRT (POD-Layout != binary_id).
    using ::comdare::cache_engine::abi::AnatomyVersionLines;
    static_assert(sizeof(AnatomyVersionLines) == 88, "POD-Layout-Wache: 12 Felder, 8-aligned -> 88 Byte (x86_64).");
    static_assert(alignof(AnatomyVersionLines) == 8);
    EXPECT_EQ(sizeof(AnatomyVersionLines), 88u);
    EXPECT_EQ(alignof(AnatomyVersionLines), 8u);
    EXPECT_EQ(::comdare::cache_engine::abi::kAnatomyVersionLinesLayout, 4u);
    // Offset-Stabilitaet: organ_/system_/measurement_/merge_ liegen unveraendert, das SHA-512-Feld folgt dahinter.
    EXPECT_EQ(offsetof(AnatomyVersionLines, organ_line), 8u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, system_line), 24u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_line), 40u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_len), 48u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, merge_line), 56u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, merge_len), 64u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, sha512_line), 72u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, sha512_len), 80u);
}

TEST(MW12StampBausteine, AnatomyFingerprintHexIsSha512OfConcat) {
    // K7b-3 (Section 62-B): der 5. POD-Stempel == SHA-512(concat organ+system+measurement+merge) als 128-hex,
    // nullterminiert (D3-Reihenfolge). Selbst-konsistent gegen die K7b-1-Primitive geprueft (kein externer Vektor).
    namespace abi     = ::comdare::cache_engine::abi;
    namespace s5      = ::comdare::cache_engine::sha512;
    constexpr auto fp = abi::anatomy_fingerprint_hex("a", "b", "c", "d");
    static_assert(fp[128] == '\0', "Fingerprint-Zeile nullterminiert");
    constexpr auto ref = s5::to_hex(s5::sha512("abcd"));
    for (std::size_t i = 0; i < 128; ++i) EXPECT_EQ(fp[i], ref[i]) << "hex-Stelle " << i;
    // ce-only-/Katalog-Pfad: measurement == merge == "" -> Fingerprint von concat(organ+system) allein.
    constexpr auto fp_ceonly  = abi::anatomy_fingerprint_hex("org", "sys", "", "");
    constexpr auto ref_ceonly = s5::to_hex(s5::sha512("orgsys"));
    for (std::size_t i = 0; i < 128; ++i) EXPECT_EQ(fp_ceonly[i], ref_ceonly[i]);
}

TEST(MW12StampBausteine, MergeStampLineCarriesMergeCombinationOrEmptyForCeOnly) {
    // K6a (Section 59, 2026-07-20): der DRITTE Tier-Binary-Stempel = die Merge-Kombination. Format
    // "merge=<strategy>;pruefling=<pruefling>[;<axis>=<algo>@X.Y.Z;...]" -- dieselbe X.Y.Z-Voll-Form / SEPARATE Welt
    // zur .algos-Sig; NUR Haupt-Achsen. ce-only-/Identitaets-Fall -> LEERE Zeile (golden-Konsequenz Section 59-C).
    namespace abi = ::comdare::cache_engine::abi;
    // ce-only (Stufe1 / leere Strategie) -> leer (byte-identischer golden-Pfad).
    EXPECT_TRUE(abi::merge_stamp_line("Stufe1_CeOnly", "prt_art").empty());
    EXPECT_TRUE(abi::merge_stamp_line("", "prt_art").empty());
    // Identitaets-/self-Pruefling ("CacheEngine"/"self" / kein Pruefling) -> leer (Fork 3: identity=self ist ce).
    EXPECT_TRUE(abi::merge_stamp_line("Stufe2_PrueflingReplace", "CacheEngine").empty());
    EXPECT_TRUE(abi::merge_stamp_line("Stufe2_PrueflingReplace", "self").empty());
    EXPECT_TRUE(abi::merge_stamp_line("Stufe2_PrueflingReplace", "").empty());
    // Merge-Fall OHNE Achsen-Versionen -> nur Merge-Art + Pruefling.
    EXPECT_EQ(abi::merge_stamp_line("Stufe2_PrueflingReplace", "prt_art"),
              std::string{"merge=Stufe2_PrueflingReplace;pruefling=prt_art"});
    // Merge-Fall MIT Achsen-Versionen (Voll-Form via algo_semver; SEPARATE Welt zur .algos-Sig).
    std::array<m::AxisVersionEntry, 1> const merged{{{"path_compression", "prt_patricia", "v2.3.4"}}};
    std::string const                        line = abi::merge_stamp_line("Stufe3_FullJoin", "prt_art", merged);
    EXPECT_EQ(line, std::string{"merge=Stufe3_FullJoin;pruefling=prt_art;path_compression=prt_patricia@2.3.4"});
    EXPECT_EQ(line.find("@v2.3.4"), std::string::npos); // X.Y.Z, nicht die rohe Version
}

// A1 (G2-4a, 2026-07-23) EINGEFRORENER FINGERPRINT-TESTVEKTOR (Lager-Gate §66, Sync-Kante B3). Vier FESTE Stempel-
// Zeilen -> EIN fester 128-hex SHA-512. ZWEI Zwecke: (1) Neutralitaets-Testat der W12-Literal-Migration ("v1"->"v1.0.0")
// -- der Fingerprint FESTER Zeilen ist von der Migration unabhaengig (die Zeilen sind Literale, nicht die migrierten
// Wrapper), also identisch vor/nach A1. (2) Konsistenz-Anker fuer Lane B (G3-BinaryKeyPolicy, Scheibe B3): Impl-G3-P2
// bildet ctsha512 ueber DIESELBEN vier Zeilen in DERSELBEN Reihenfolge (organ+system+measurement+merge) und MUSS exakt
// kFrozenFingerprintV1 erhalten -- EIN Testvektor, zwei Module (keine Separator-/Whitespace-Drift). Die vier Zeilen und
// der Hex sind EINGEFROREN: NIE aendern (bricht die B3-Sync), nur bei bewusstem Fingerprint-Bruch unter Absprache.
TEST(MW12StampBausteine, FrozenFingerprintTestVectorForLagerGateB3) {
    namespace abi                       = ::comdare::cache_engine::abi;
    constexpr std::string_view kOrgan   = "search_algo=k_ary@1.0.0;path_compression=path_compression_none@1.0.0";
    constexpr std::string_view kSystem  = "compiler=code@1.0.0;isa=amd64";
    constexpr std::string_view kMeasure = "wallclock@1.0.0";
    constexpr std::string_view kMerge   = "merge=Stufe1_CeOnly;pruefling=self";
    // EINGEFROREN (Sync mit Lane-B B3): 128-hex SHA-512 von concat(kOrgan+kSystem+kMeasure+kMerge). NIE aendern.
    constexpr std::string_view kFrozenFingerprintV1 =
        "0f0c0eb44d4308c3a9d05f92abcb10a8fa68063634a5bd669ae38f8ac2272285"
        "fb594f0bbdc4547f1bb73f57a5a17d32bee21d3781be27da9577505ad5c31b93";
    constexpr auto fp = abi::anatomy_fingerprint_hex(kOrgan, kSystem, kMeasure, kMerge);
    static_assert(fp[128] == '\0', "Fingerprint-Zeile nullterminiert");
    static_assert(std::string_view{fp.data()} == kFrozenFingerprintV1,
                  "EINGEFRORENER Fingerprint (B3-Sync): die 4 Zeilen ODER der Hash haben sich geaendert -- unter "
                  "Absprache neu einfrieren, sonst bricht die Lane-B-Konsistenz");
    EXPECT_EQ(std::string_view{fp.data()}, kFrozenFingerprintV1)
        << "eingefrorener Fingerprint-Testvektor (Lager-Gate §66, Sync mit Lane-B Scheibe B3)";
}

TEST(MW12StampBausteine, PlannerVersionStampCarriesSelfVersionAndIsaOs) {
    EXPECT_EQ(pl::kPlannerVersion, std::string_view{"1.0.0"}); // X.Y.Z initial
    EXPECT_EQ(pl::planner_target_isa(), std::string_view{"x86_64"});
    std::string const stamp = pl::planner_version_stamp();
    EXPECT_NE(stamp.find("planner@1.0.0"), std::string::npos) << "stamp='" << stamp << "'";
    EXPECT_NE(stamp.find("isa=x86_64"), std::string::npos) << "stamp='" << stamp << "'";
    EXPECT_NE(stamp.find("os="), std::string::npos) << "stamp='" << stamp << "'";
}

// A2 (G2-4 Schritt 3+4): System-Achsen-Code-Versionen + Mess-Tooling-Version aus Single-Sources statt Hartkodierung.
// Render-neutral fuer die gueltigen ids/Achsen; "v0"-Sentinel nur fuer ungueltige Tooling-ids.
TEST(MW12StampBausteine, A2SystemAndToolingCodeVersionsSingleSource) {
    namespace abi = ::comdare::cache_engine::abi;
    // (a) System-Achsen-Single-Source: 5 Achsen, alle Init-Version "v1.0.0".
    EXPECT_EQ(abi::kSystemAxisCodeCount, std::size_t{5});
    for (auto const& e : abi::kSystemAxisCodeVersions) {
        EXPECT_FALSE(e.axis.empty());
        EXPECT_EQ(e.version, std::string_view{"v1.0.0"});
    }
    // Neutralitaets-Anker: system_stamp_line() rendert byte-identisch zur frueheren "v1"-Hartkodierung.
    EXPECT_EQ(abi::system_stamp_line(),
              std::string{"compiler=code@1.0.0;extension_hardware=code@1.0.0;target_isa=code@1.0.0;"
                          "scheduling=code@1.0.0;load_framework=code@1.0.0"});

    // (b) Mess-Tooling-Version-Feld + id-Lookup.
    for (auto const& t : m::kMeasurementToolingRegistry) EXPECT_EQ(t.version, std::string_view{"v1.0.0"});
    EXPECT_EQ(m::tooling_version_for_id("wallclock"), std::string_view{"v1.0.0"});
    EXPECT_EQ(m::tooling_version_for_id("macro"), std::string_view{"v1.0.0"});
    EXPECT_EQ(m::tooling_version_for_id("micro"), std::string_view{"v1.0.0"});
    EXPECT_EQ(m::tooling_version_for_id("bogus"), std::string_view{"v0"}); // unbekannt -> Sentinel

    // (c) Sentinel-Render: ungueltige Tooling-id -> @0.0.0; gueltige bleiben @1.0.0 (render-neutral).
    EXPECT_EQ(abi::measurement_stamp_line("bogus"), std::string{"measurement_tooling=bogus@0.0.0"});
    EXPECT_EQ(abi::measurement_stamp_line("wallclock"), std::string{"measurement_tooling=wallclock@1.0.0"});
}
