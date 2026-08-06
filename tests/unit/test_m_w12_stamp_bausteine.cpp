// Querschnitt M -- W12-A (Section43): X.Y.Z-Parse-Helfer (Inkrement 2) + Planer-Selbst-Stempel (Inkrement 3).
// Leichte TU: verifiziert die isolierten Stempel-Bausteine + ihre Byte-Trennung zur .algos-Welt.
// EHRLICHKEITS-NACHZUG (NB-3/T2-D): der Kopf sagte "keine Registries". Das stimmt seit NB/CX-4 nicht mehr --
// profile_facade/toolchain_stamp_naht.hpp zieht driver_build_variant_signature.hpp und damit die drei
// Build-Achsen-Registries samt CMake-generierter Flags-Header nach. Die Abhaengigkeit ist ab hier EXPLIZIT
// inkludiert statt transitiv geerbt; die Beschreibung nennt den Ist-Stand.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // W12-A3: AnatomyVersionLines-POD-Layout-Wache
#include <cache_engine/abi/anatomy_fingerprint.hpp>        // K7b-3: anatomy_fingerprint_hex (A13-M3: 3 Stempel-Zeilen)
#include <cache_engine/abi/anatomy_stamp_entries.hpp>      // A3: count/parse_stamp_entries + AnatomyStampEntryV1
#include <cache_engine/abi/anatomy_version_stamp.hpp>
#include <cache_engine/abi/meta_meta_stamp_suffix.hpp>               // A13-M2: Klammer-Anhang der Meta-Metas (Owner-Q1)
#include <cache_engine/measurement/external_utils_family_axis.hpp>   // A13-M2: ExternalUtilsHub (System-Realm)
#include <cache_engine/abi/system_axis_code_versions.hpp>            // A2: kSystemAxisCodeVersions (Single-Source)
#include <cache_engine/measurement/measurement_tooling_registry.hpp> // A2: version-Feld + tooling_version_for_id
#include <sha512/ctsha512.hpp> // K7b-3: Referenz-SHA-512 fuer den Fingerprint-Korrektheitstest
#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/axis_version_stamp.hpp>
#include "builder/ceb_version_stamp.hpp"              // A5: CEB-Selbst-Stempel (consteval Mess-Array + SHA-512)
#include <cache_engine/abi/toolchain_stamp_glied.hpp> // O-2/C-2: Renderer + kToolchainAxisVersions
#include <profile_facade/planner/planner_version.hpp>
#include <profile_facade/system_version_suffix.hpp> // O-2/C-2: Doppel-Wahrheits-Wache gegen den Suffix
#include <profile_facade/toolchain_stamp_naht.hpp> // NB/CX-4: die LIVE-Naht der Glieder [5]/[6]

#include "builder/build_variant_set_signature.hpp"  // NB-3/T2-D: die Paar-Wache (Praedikat-Haelfte)
#include "builder/driver_build_variant_signature.hpp" // NB-3/T2-D: die realen Registry-Listen (All*)

#include <gtest/gtest.h>

#include <boost/mp11.hpp> // NB-3/T2-D: mp_list fuer die synthetischen Negativ-Listen

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint> // A13-M1: uint32_t/uint64_t explizit (reserved-Flag-Wachen)
#include <span>
#include <stdexcept> // NB/CX-1/CX-2: die Injektivitaets-Wachen sind FAIL-LOUD (std::invalid_argument)
#include <string>
#include <string_view>
#include <type_traits> // NB-3/T2-D: is_constructible_v-Beweis des geloeschten Rvalue-Konstruktors
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp> // STRUKT-R ORG-18

namespace m  = ::comdare::cache_engine::measurement;
namespace pl = ::comdare::cache_engine::planner;

namespace {
// Mock-Achsen (name() + algo_version) + Mock-Composition (17 benannte Aliase) fuer organ_stamp_line<Comp>().
// A13-M1b (Owner-Q3 02.08.2026, Kurzform-Rueckbau): die Mock-Version stand als Kurzform "v1" da und ist auf die
// dreistellige Form gezogen. RENDER-NEUTRAL -- "v1" und "v1.0.0" rendern beide "1.0.0", die Golden-Strings der
// Organ-/Stempel-Zeilen unten bleiben damit byte-identisch (kein Anker wurde nachgezogen).
struct MockAxisV1 {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "algoA"; }
    static constexpr std::string_view               algo_version = "v1.0.0";
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
    // STRUKT-R ORG-18: 18. Organ-Slot (Pflicht, kein Default). MemoryOnlyTarget = Durchreich-Wert:
    // kein Rueckschreib-Pfad. VOLL qualifiziert, weil der Member-Alias den Namespace sonst verdeckt.
    using persistence_target = ::comdare::cache_engine::persistence_target::MemoryOnlyTarget;
};
} // namespace

TEST(MW12StampBausteine, AlgoSemVerParsesVxyzOnly) {
    // A13-M1b (Owner-Q3 02.08.2026): "Die Kurzform ist verboten, Versionierungen sind einheitlich und immer
    // 3-Stellig und beginnen mit 'v'." -> NUR "vX.Y.Z" (plus Flag-Schwanz); die frueher erlaubte Kurzform "vN"
    // ist ZURUECKGEBAUT und faellt jetzt in den Sentinel {0,0,0}.
    EXPECT_EQ(m::parse_algo_semver("v1.0.0"), (m::AlgoSemVer{1, 0, 0}));
    EXPECT_EQ(m::parse_algo_semver("v2.3.4"), (m::AlgoSemVer{2, 3, 4}));
    EXPECT_EQ(m::parse_algo_semver("v0.0.0"), (m::AlgoSemVer{0, 0, 0}));
    EXPECT_EQ(m::parse_algo_semver("v1"), (m::AlgoSemVer{0, 0, 0}));    // KURZFORM-RUECKBAU (frueher {1,0,0})
    EXPECT_EQ(m::parse_algo_semver("v0"), (m::AlgoSemVer{0, 0, 0}));    // war und bleibt Sentinel
    EXPECT_EQ(m::parse_algo_semver("v12"), (m::AlgoSemVer{0, 0, 0}));   // auch mehrstellig
    EXPECT_EQ(m::parse_algo_semver("v1.2"), (m::AlgoSemVer{0, 0, 0}));  // Kurzform verboten
    EXPECT_EQ(m::parse_algo_semver("1.0.0"), (m::AlgoSemVer{0, 0, 0})); // ohne 'v'
    EXPECT_EQ(m::parse_algo_semver(""), (m::AlgoSemVer{0, 0, 0}));
}

TEST(MW12StampBausteine, AlgoSemVerFullFormForStampsOnly) {
    // Die X.Y.Z-VOLL-Form fuer Stempel/Registry -- der heutige "v1.0.0"-Stand aller Algos rendert "1.0.0".
    EXPECT_EQ(m::algo_semver_string("v1.0.0"), std::string{"1.0.0"});
    EXPECT_EQ(m::algo_semver_string("v2.3.4"), std::string{"2.3.4"});
    EXPECT_EQ(m::algo_semver_string("v0.0.0"), std::string{"0.0.0"});
    // A13-M1b: die zurueckgebaute Kurzform ist Sentinel und rendert deshalb "0.0.0" statt frueher "1.0.0".
    EXPECT_EQ(m::algo_semver_string("v1"), std::string{"0.0.0"});
    // Byte-Trennung zur .algos-Welt: die Voll-Form ist NICHT der rohe algo_version-String.
    EXPECT_NE(m::algo_semver_string("v1.0.0"), std::string{"v1.0.0"});
}

// A13-M1b (Owner-Antwort Q3 vom 02.08.2026, verbatim): "Die Kurzform ist verboten, Versionierungen sind
// einheitlich und immer 3-Stellig und beginnen mit 'v'. Das 'e' ist eine Flag und kann spaeter gegen andere
// Falgs wie 'g' fuer GPU, 'c' fuer CPU, 'f' fuer FPGA und 'n' fuer NPU code erweitert werden. Wir produzieren
// nur CPU code, daher muessen alle Versionen mit 'c' oder 'ce' enden."
// Ersetzt die A13-M1-Erwartungen ("vNe"-Kurzform). Hier: PARSER-Semantik der flaglosen Form, volle Flag-Familie, die
// FIXE Reihenfolge (HW-Flag, dann 'e') und die Review-Auflage K-5 (Sentinel-Batterie mit Flags).
TEST(MW12StampBausteine, A13M1bFlagGrammarParsesInRawAndDottedForm) {
    // (a) PARSER-Semantik: die flaglose dreistellige Form parst weiter und traegt KEIN Flag. Bis A13-M3/C4
    //     war das die Uebergangs-Toleranz des BESTANDS; seither ist der Bestand flagbehaftet ("v1.0.0c").
    EXPECT_FALSE(m::parse_algo_semver("v1.0.0").has_hardware_flag());
    EXPECT_FALSE(m::parse_algo_semver("v1.0.0").experimental);
    EXPECT_FALSE(m::parse_algo_semver("v2.3.4").experimental);
    EXPECT_FALSE(m::parse_dotted_semver("1.0.0").has_hardware_flag());

    // (b) Die volle Flag-Familie c/g/f/n an der ROHEN Form.
    EXPECT_EQ(m::parse_algo_semver("v1.0.0c"), (m::AlgoSemVer{1, 0, 0, m::HardwareFlag::cpu}));
    EXPECT_EQ(m::parse_algo_semver("v1.0.0g"), (m::AlgoSemVer{1, 0, 0, m::HardwareFlag::gpu}));
    EXPECT_EQ(m::parse_algo_semver("v1.0.0f"), (m::AlgoSemVer{1, 0, 0, m::HardwareFlag::fpga}));
    EXPECT_EQ(m::parse_algo_semver("v1.0.0n"), (m::AlgoSemVer{1, 0, 0, m::HardwareFlag::npu}));
    EXPECT_TRUE(m::parse_algo_semver("v10.0.1c").has_hardware_flag());

    // (c) Reihenfolge FIX: erst das Hardware-Flag, dann optional 'e' ("v2.3.4ce").
    EXPECT_EQ(m::parse_algo_semver("v2.3.4ce"), (m::AlgoSemVer{2, 3, 4, m::HardwareFlag::cpu, true}));
    EXPECT_EQ(m::parse_algo_semver("v2.3.4ge"), (m::AlgoSemVer{2, 3, 4, m::HardwareFlag::gpu, true}));
    EXPECT_TRUE(m::parse_algo_semver("v2.3.4ce").experimental);

    // (d) Gerenderte Form MIT Flags (die Stempel-Zeilen-Ruecklesung).
    EXPECT_EQ(m::parse_dotted_semver("1.0.0c"), (m::AlgoSemVer{1, 0, 0, m::HardwareFlag::cpu}));
    EXPECT_EQ(m::parse_dotted_semver("2.3.4ce"), (m::AlgoSemVer{2, 3, 4, m::HardwareFlag::cpu, true}));
    EXPECT_EQ(m::parse_dotted_semver("2.3.4n"), (m::AlgoSemVer{2, 3, 4, m::HardwareFlag::npu}));

    // (e) GENAU EIN Hardware-Flag; 'e' NUR NACH einem Hardware-Flag; nur Kleinbuchstaben; kein Rest.
    EXPECT_EQ(m::parse_algo_semver("v1.0.0cc"), (m::AlgoSemVer{}));  // zweites Flag
    EXPECT_EQ(m::parse_algo_semver("v1.0.0cg"), (m::AlgoSemVer{}));  // zwei verschiedene Flags
    EXPECT_EQ(m::parse_algo_semver("v1.0.0ec"), (m::AlgoSemVer{}));  // Reihenfolge verdreht
    EXPECT_EQ(m::parse_algo_semver("v1.0.0e"), (m::AlgoSemVer{}));   // 'e' OHNE Hardware-Flag
    EXPECT_EQ(m::parse_algo_semver("v2.3.4e"), (m::AlgoSemVer{}));   // die A13-M1-Form ohne HW-Flag
    EXPECT_EQ(m::parse_algo_semver("v1.0.0C"), (m::AlgoSemVer{}));   // Gross-'C'
    EXPECT_EQ(m::parse_algo_semver("v1.0.0cE"), (m::AlgoSemVer{}));  // Gross-'E'
    EXPECT_EQ(m::parse_algo_semver("v1.0.0cee"), (m::AlgoSemVer{})); // doppeltes 'e'
    EXPECT_EQ(m::parse_algo_semver("v1.0.0ce5"), (m::AlgoSemVer{})); // Rest nach dem 'e'
    EXPECT_EQ(m::parse_algo_semver("v1.0.0x"), (m::AlgoSemVer{}));   // unbekanntes Flag-Zeichen
    EXPECT_EQ(m::parse_algo_semver("v1.2ce"), (m::AlgoSemVer{}));    // Kurzform bleibt verboten
    EXPECT_EQ(m::parse_algo_semver("v1ce"), (m::AlgoSemVer{}));      // Kurzform mit Flags -> Sentinel
    EXPECT_EQ(m::parse_dotted_semver("1.0c"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_dotted_semver("2.3.4E"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_dotted_semver("2.3.4e"), (m::AlgoSemVer{}));   // 'e' ohne Hardware-Flag
    EXPECT_EQ(m::parse_dotted_semver("2.3.4cc"), (m::AlgoSemVer{}));  // zweites Flag
    EXPECT_EQ(m::parse_dotted_semver("v2.3.4ce"), (m::AlgoSemVer{})); // rohe Form in der gerenderten Wache

    // (f) AUFLAGE K-5 -- "v0c"/"v0.0.0c"/"v0.0.0ce"/"0.0.0ce" sind der REINE Sentinel: das Null-Tripel bleibt
    //     der Sentinel, die Flags fallen weg. Sonst haette ein "v0.0.0c" die Registry-Wache ausgehebelt.
    EXPECT_EQ(m::parse_algo_semver("v0c"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("v0.0.0c"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("v0.0.0ce"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_dotted_semver("0.0.0c"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_dotted_semver("0.0.0ce"), (m::AlgoSemVer{}));
    EXPECT_FALSE(m::parse_algo_semver("v0.0.0ce").experimental);
    EXPECT_FALSE(m::parse_algo_semver("v0.0.0ce").has_hardware_flag());
    EXPECT_FALSE(m::parse_dotted_semver("0.0.0ce").experimental);
    // is_sentinel prueft NUR das x/y/z-Tripel (K-5: Wachen nicht am Struct-Vergleich haengen).
    EXPECT_TRUE(m::parse_algo_semver("v0.0.0c").is_sentinel());
    EXPECT_TRUE(m::parse_algo_semver("v0").is_sentinel());
    EXPECT_TRUE(m::parse_dotted_semver("0.0.0ce").is_sentinel());
    EXPECT_FALSE(m::parse_algo_semver("v1.0.0c").is_sentinel());
    EXPECT_FALSE(m::parse_algo_semver("v2.3.4ce").is_sentinel());
    // compile-time-Beweis derselben Wache (die Registry-Wachen laufen consteval).
    static_assert(m::parse_algo_semver("v0.0.0ce") == m::AlgoSemVer{});
    static_assert(m::parse_dotted_semver("0.0.0ce").is_sentinel());
    static_assert(!m::parse_algo_semver("v2.3.4ce").is_sentinel());

    // (g) UNTERSCHEIDBARKEIT: jedes Flag erzeugt ein anderes Stempel-Segment -> eigener Fingerprint/Lager-Key.
    EXPECT_NE(m::parse_algo_semver("v1.0.0c"), m::parse_algo_semver("v1.0.0"));
    EXPECT_NE(m::parse_algo_semver("v1.0.0c"), m::parse_algo_semver("v1.0.0g"));
    EXPECT_NE(m::parse_algo_semver("v1.0.0c"), m::parse_algo_semver("v1.0.0ce"));
    EXPECT_NE(m::parse_dotted_semver("2.3.4ce"), m::parse_dotted_semver("2.3.4c"));
    // (h) Round-Trip rohe <-> gerenderte Form INKLUSIVE beider Flags.
    EXPECT_EQ(m::parse_dotted_semver("1.0.0c"), m::parse_algo_semver("v1.0.0c"));
    EXPECT_EQ(m::parse_dotted_semver("2.3.4ce"), m::parse_algo_semver("v2.3.4ce"));

    // (i) Die Owner-PFLICHT-Wache selbst (immer gebaut, unabhaengig von COMDARE_VERSION_HW_FLAG_ENFORCE):
    //     nur "...c"/"...ce" erfuellen den CPU-only-Scope. Bis A13-M3/C4 erfuellte der Bestand sie bewusst
    //     NICHT und das Define stand auf OFF; seit C4 erfuellt er sie, und das Define ist scharf (s. unten).
    EXPECT_TRUE(m::version_satisfies_cpu_only_policy("v1.0.0c"));
    EXPECT_TRUE(m::version_satisfies_cpu_only_policy("v2.3.4ce"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1.0.0")); // flaglos: seit C4 kein Bestand mehr
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1.0.0g"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1.0.0f"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1.0.0n"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v0.0.0c")); // Sentinel erfuellt nie eine Politik
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1c"));     // Kurzform erfuellt nie eine Politik
    // A13-M3/C4: der Pin DREHT. Bis zur Migration war der Bestand flaglos und dieser Test schrieb das fest
    // ("gruene Tests zementieren alte Ordnung" -- seine eigene Fehlermeldung hat den Umschlag angekuendigt).
    // Seit dem Migrations-Commit traegt JEDE ce-eigene Version das CPU-Flag, und ENFORCE ist scharf.
    static_assert(COMDARE_VERSION_HW_FLAG_ENFORCE == 1, "A13-M3/C4: die Pflicht ist scharf geschaltet.");
    for (auto const& t : m::kMeasurementToolingRegistry)
        EXPECT_TRUE(m::version_satisfies_cpu_only_policy(t.version))
            << "Bestands-Version '" << t.version << "' ohne CPU-Flag -- die C4-Migration hat sie ausgelassen.";
}

TEST(MW12StampBausteine, A13M1bFlagsRenderAndStayGoldenNeutral) {
    // Render: erst das Hardware-Flag, dann das 'e'; der Sentinel rendert IMMER nackt "0.0.0".
    EXPECT_EQ(m::algo_semver_string("v1.0.0c"), std::string{"1.0.0c"});
    EXPECT_EQ(m::algo_semver_string("v2.3.4ce"), std::string{"2.3.4ce"});
    EXPECT_EQ(m::algo_semver_string("v2.3.4g"), std::string{"2.3.4g"});
    EXPECT_EQ(m::algo_semver_string("v2.3.4ne"), std::string{"2.3.4ne"});
    EXPECT_EQ(m::algo_semver_string("v0.0.0c"), std::string{"0.0.0"});
    EXPECT_EQ(m::algo_semver_string("v0.0.0ce"), std::string{"0.0.0"});
    // GOLDEN-NEUTRALITAET der FLAGLOSEN Form ("v1.0.0"/"v2.3.4"): sie rendert byte-identisch wie vor A13-M1b.
    // Sie beschreibt seit A13-M3/C4 keinen Bestands-Fall mehr, bleibt aber Parser-/Renderer-Semantik.
    EXPECT_EQ(m::algo_semver_string("v1.0.0"), std::string{"1.0.0"});
    EXPECT_EQ(m::algo_semver_string("v2.3.4"), std::string{"2.3.4"});
    EXPECT_EQ(m::algo_semver_string("v0.0.0"), std::string{"0.0.0"});
    // Jede Flag-Belegung ist gerendert verschieden von der flaglosen und von jeder anderen.
    EXPECT_NE(m::algo_semver_string("v2.3.4c"), m::algo_semver_string("v2.3.4"));
    EXPECT_NE(m::algo_semver_string("v2.3.4c"), m::algo_semver_string("v2.3.4g"));
    EXPECT_NE(m::algo_semver_string("v2.3.4ce"), m::algo_semver_string("v2.3.4c"));
    // Stempel-Zeile end-to-end: die Flags erscheinen im Segment (und damit im SHA-512-Preimage).
    std::array<m::AxisVersionEntry, 2> const entries{
        {{"path_compression", "prt_patricia", "v2.3.4ce"}, {"filter", "bloom", "v1.0.0"}}};
    EXPECT_EQ(m::build_axis_version_stamp_line(entries),
              std::string{"path_compression=prt_patricia@2.3.4ce;filter=bloom@1.0.0"});
}

TEST(MW12StampBausteine, AxisVersionStampLineUsesFullSemverAndCanonicalOrder) {
    // Stempel-Zeile "achse=algorithmus@X.Y.Z;..." in Eingabe- (== compose-) Reihenfolge, Voll-Form via algo_semver.
    // A13-M1b: die Eingabe steht dreistellig ("v1" war Kurzform) -- RENDER-NEUTRAL, der Golden-String unten
    // ist unveraendert.
    std::array<m::AxisVersionEntry, 2> const entries{{{"search_algo", "bst", "v1.0.0"}, {"filter", "bloom", "v2.3.4"}}};
    std::string const                        line = m::build_axis_version_stamp_line(entries);
    EXPECT_EQ(line, std::string{"search_algo=bst@1.0.0;filter=bloom@2.3.4"});
    // SEPARATE Welt zur .algos-Sig: der Stempel traegt X.Y.Z, NICHT die rohe "@v1"-Form.
    EXPECT_EQ(line.find("@v1"), std::string::npos);
    EXPECT_NE(line.find("@1.0.0"), std::string::npos);
    // leere Eingabe -> leere Zeile.
    EXPECT_TRUE(m::build_axis_version_stamp_line(std::span<m::AxisVersionEntry const>{}).empty());
}

TEST(MW12StampBausteine, OrganStampLineDerivesAll18AxesInCanonicalOrder) {
    std::string const line = ::comdare::cache_engine::abi::organ_stamp_line<MockComposition>();
    // O-8 Schritt 7 (A8.2, OP-11): die Organ-Zeile traegt ACHTZEHN Haupt-Achsen, nicht mehr siebzehn
    // (persistence_target als 18. Slot). 18 Slots -> exakt 17 Trenner ';'. Neu geankert aus dem
    // Werkzeug-Output des Fenster-Laufs (Ist 17), nicht aus der Zaehlung von Hand.
    EXPECT_EQ(std::count(line.begin(), line.end(), ';'), 17);
    // kanonische Ordnung: beginnt mit search_algo. Das ENDE ist mit A8.2 von queuing_q2 auf
    // persistence_target gewandert -- queuing_q2 steht weiter in der Zeile, aber nicht mehr zuletzt.
    EXPECT_EQ(line.rfind("search_algo=algoA@1.0.0", 0), 0u);
    EXPECT_NE(line.find(";queuing_q2=algoA@1.0.0"), std::string::npos);
    EXPECT_NE(line.find(";persistence_target="), std::string::npos);
    EXPECT_EQ(line.find(';', line.find(";persistence_target=") + 1), std::string::npos)
        << "persistence_target ist der LETZTE Slot -- danach darf kein Trenner mehr folgen. line=" << line;
    // filter (Slot 15) traegt die abweichende X.Y.Z-Voll-Form:
    EXPECT_NE(line.find(";filter=algoB@2.3.4;"), std::string::npos);
    // SEPARATE Welt zur .algos-Sig: KEINE rohe "@v1"-Form im Stempel.
    EXPECT_EQ(line.find("@v1"), std::string::npos);
}

TEST(MW12StampBausteine, SystemStampLineIsStaticSystemAxisAlgoVersions) {
    // Entscheid W12-A-1: statische System-Achsen-Algo-Versionen (nicht die gewaehlten Zellwerte; W10-Anschluss).
    // O-8 Schritt 4 (A3-Kern + K1-Umzug): die System-Haupt-Achsen sind von FUENF auf DREI zurueckgebaut --
    // target_isa, operating_system, external_utils. Dabei drei getrennte Bewegungen, die hier alle sichtbar
    // werden: compiler und scheduling sind KEINE System-Haupt-Achsen mehr, extension_hardware heisst seit
    // A2 external_utils, und load_framework ist in den MESS-Realm umgezogen (K1) -- es steht deshalb nicht
    // mehr in dieser Zeile, sondern als erstes Segment der Mess-Zeile (siehe die Mess-Tests unten).
    // A13-M2 (Owner-E2 + Q1 vom 02.08.2026): HINTER die drei Haupt-Achsen haengt jetzt der KLAMMER-ANHANG der
    // System-Meta-Metas -- heute "[simd=code@1.0.0c]". Der Trenner-Anker steigt deshalb BEWUSST von 2 auf 3
    // (drei Haupt-Achsen + ein Klammer-Anhang == VIER Eintraege). Neu geankert aus dem Ist-Output des
    // A13-M2-Laufs, nicht aus der Zaehlung von Hand.
    std::string const line = ::comdare::cache_engine::abi::system_stamp_line();
    EXPECT_EQ(std::count(line.begin(), line.end(), ';'), 3); // 3 Haupt-Achsen + 1 Klammer-Anhang -> 3 Trenner
    EXPECT_EQ(line.rfind("target_isa=code@1.0.0c", 0), 0u);
    EXPECT_NE(line.find(";operating_system=code@1.0.0c"), std::string::npos);
    EXPECT_NE(line.find(";external_utils=code@1.0.0c"), std::string::npos);
    // Die drei abgewanderten Namen duerfen hier NICHT mehr auftauchen -- sonst waere der Umbau nur halb.
    EXPECT_EQ(line.find("compiler="), std::string::npos);
    EXPECT_EQ(line.find("scheduling="), std::string::npos);
    EXPECT_EQ(line.find("extension_hardware="), std::string::npos);
    EXPECT_EQ(line.find("load_framework="), std::string::npos);
    EXPECT_EQ(line.find("@v1"), std::string::npos); // separate Welt zur .algos-Sig
    // A13-M2: der Anhang steht ANS ENDE der Kette (Owner-E2) und ist geklammert (Owner-Q1) -- NICHT als
    // Punkt-Pfad "external_utils.simd=" (die verworfene Design-Empfehlung) und NICHT vor den Haupt-Achsen.
    EXPECT_TRUE(line.ends_with(";[simd=code@1.0.0c]")) << "line=" << line;
    EXPECT_EQ(line.find("external_utils.simd"), std::string::npos)
        << "Punkt-Pfad-Form ist VERWORFEN (Owner-Q1 = Klammer-Form). line=" << line;
    EXPECT_EQ(std::count(line.begin(), line.end(), '['), 1);
    EXPECT_EQ(std::count(line.begin(), line.end(), ']'), 1);
}

// A13-M2 (Owner-Antwort Q1 vom 02.08.2026, verbatim): "Q1 - Wie empfohlen nach Klammern (derzeit auch so
// geplant, bitte nachlesen)." Die Klammer-Anzahl kodiert die EBENE (Q-A-Auflage, hardware_meta_meta_axis.hpp
// Kopf). Hier: Renderer und consteval-Parser treffen sich -- was die Zeile schreibt, liest der POD zurueck.
TEST(MW12StampBausteine, A13M2MetaMetaKlammerAnhangRoundtripsThroughParser) {
    namespace abi = ::comdare::cache_engine::abi;
    // (a) Der Renderer: der System-Hub liefert genau seine Typlisten-Glieder, geklammert.
    EXPECT_EQ(abi::meta_meta_stamp_suffix<m::ExternalUtilsHub>(), std::string{"[simd=code@1.0.0c]"});
    // Leerer Hub (ein Blatt ist kein Hub) -> LEERER Anhang, kein "[]": daran haengt die Byte-Neutralitaet
    // des heute leeren Organ-Realms.
    EXPECT_TRUE(abi::meta_meta_stamp_suffix<m::SimdExternalUtilsFamily>().empty());
    // Die ENTRY-getriebene Form (Mess-Realm: die Wahl kommt aus der Registry, nicht aus dem Typ) schreibt
    // dieselbe Klammer ueber dieselbe Stelle.
    std::array<m::AxisVersionEntry, 1> const lf{{{"load_framework", "ycsb", "v1.0.0c"}}};
    EXPECT_EQ(abi::meta_meta_stamp_suffix_from(std::span<m::AxisVersionEntry const>{lf}),
              std::string{"[load_framework=ycsb@1.0.0c]"});
    EXPECT_TRUE(abi::meta_meta_stamp_suffix_from(std::span<m::AxisVersionEntry const>{}).empty());

    // (b) append_meta_meta_suffix: ANS ENDE; leere Zeile bleibt leer; leerer Anhang laesst die Zeile gleich.
    std::string line = "a=b@1.0.0";
    abi::append_meta_meta_suffix(line, "[c=d@2.0.0]");
    EXPECT_EQ(line, std::string{"a=b@1.0.0;[c=d@2.0.0]"});
    std::string leer;
    abi::append_meta_meta_suffix(leer, "[c=d@2.0.0]");
    EXPECT_TRUE(leer.empty()) << "eine leere Realm-Zeile darf nie ein einsames Rahmen-Segment bekommen";
    std::string unveraendert = "a=b@1.0.0";
    abi::append_meta_meta_suffix(unveraendert, "");
    EXPECT_EQ(unveraendert, std::string{"a=b@1.0.0"});

    // (c) Der consteval-Parser liest die reale System-Zeile mit den EBENEN zurueck (Klammer-Tiefe == Ebene).
    static constexpr char kSys[] =
        "target_isa=code@1.0.0c;operating_system=code@1.0.0c;external_utils=code@1.0.0c;[simd=code@1.0.0c]";
    constexpr auto se = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kSys})>(kSys);
    static_assert(se.size() == 4, "3 Haupt-Achsen + 1 geklammerte Meta-Meta == 4 Eintraege.");
    EXPECT_EQ(se.size(), std::size_t{4});
    EXPECT_EQ(abi::stamp_entry_meta_level(se[0]), 0u);
    EXPECT_EQ(abi::stamp_entry_meta_level(se[2]), 0u);
    EXPECT_EQ(abi::stamp_entry_meta_level(se[3]), 1u);
    EXPECT_TRUE(abi::stamp_entry_is_meta_meta(se[3]));
    EXPECT_FALSE(abi::stamp_entry_is_meta_meta(se[2]));
    EXPECT_EQ(std::string_view(se[3].axis, se[3].axis_len), std::string_view{"simd"});
    EXPECT_EQ(std::string_view(se[3].algorithm, se[3].algo_len), std::string_view{"code"});
    EXPECT_EQ(se[3].x, 1u);
    // Die Klammern gehoeren NIE in einen Namen -- sonst waere der Anhang ein Namens-Praefix statt einer Ebene.
    EXPECT_EQ(std::string_view(se[2].axis, se[2].axis_len), std::string_view{"external_utils"});

    // (d) OFFENE REKURSION (Layer-Modell D4 / Owner Q-D): die zweite Ebene braucht keine Code-Zeile.
    static constexpr char kNested[] = "external_utils=code@1.0.0;[gpu=code@2.0.0;[nvlink=code@3.0.0]]";
    constexpr auto        ne = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kNested})>(kNested);
    static_assert(ne.size() == 3);
    EXPECT_EQ(abi::stamp_entry_meta_level(ne[0]), 0u);
    EXPECT_EQ(abi::stamp_entry_meta_level(ne[1]), 1u);
    EXPECT_EQ(abi::stamp_entry_meta_level(ne[2]), 2u);
    // Die Ebenen sind im reserved-Feld paarweise verschieden (sonst waeren zwei Ebenen im POD gleich).
    EXPECT_NE(ne[0].reserved, ne[1].reserved);
    EXPECT_NE(ne[1].reserved, ne[2].reserved);
    // Die drei Bit-Gruppen ueberlappen NICHT: Bit 0 ('e') / Bits 1-2 (HW-Flag) / Bits 3-5 (Ebene).
    EXPECT_EQ(abi::kStampEntryFlagExperimental & abi::kStampEntryMetaLevelMask, std::uint32_t{0});
    EXPECT_EQ(abi::kStampEntryHwFlagMask & abi::kStampEntryMetaLevelMask, std::uint32_t{0});

    // (e) Die KLAMMERLOSE Bestands-Zeile bleibt Ebene 0 und reserved == 0 -- die Erweiterung ist byte-neutral.
    static constexpr char kPlain[] = "search_algo=k_ary@1.0.0;filter=bloom@2.3.4";
    constexpr auto        pe = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kPlain})>(kPlain);
    for (auto const& x : pe) {
        EXPECT_EQ(abi::stamp_entry_meta_level(x), 0u);
        EXPECT_EQ(x.reserved, std::uint32_t{0});
    }

    // (f) A13-M2/B2-HAERTUNG (Review-BEFUND-2): die Grammatik ist STRENG, nicht bloss tolerant. Die
    // Negativproben leben als static_assert im Header selbst (anatomy_stamp_entries.hpp, Praedikat
    // abi::stamp_line_is_parsable) -- hier stehen sie noch einmal an der Test-Naht, damit der Bruch der
    // Zusage im Test-Bericht sichtbar wuerde und nicht nur in einer Uebersetzungs-Fehlermeldung.
    // KERN-Zusage (Owner-Q1): "Ein group ist IMMER ein regulaeres ';'-Geschwister-Segment" -- sonst
    // ergaeben zwei byte-VERSCHIEDENE Zeilen dasselbe Entry-Array.
    static_assert(abi::stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0]">, "kanonische Geschwister-Form.");
    static_assert(!abi::stamp_line_is_parsable<"a=b@1.0.0[c=d@1.0.0]">, "GEKLEBTE Gruppe bricht hart.");
    static_assert(!abi::stamp_line_is_parsable<"[a=b@1.0.0][c=d@1.0.0]">, "Gruppen ohne ';' bricht hart.");
    static_assert(!abi::stamp_line_is_parsable<"[a=b@1.0.0]c=d@1.0.0">, "Entry hinter ']' bricht hart.");
    static_assert(!abi::stamp_line_is_parsable<"[]">, "leere Gruppe bricht hart.");
    // A13-M3/C2b (Befund Z-02): dieselbe Zusage an der GRUPPEN-GRENZE. "[c];[e]" trug bis C2b exakt dasselbe
    // (Text, Ebene)-Entry-Array wie die kanonische Ein-Gruppen-Form "[c;e]" -- wieder zwei byte-verschiedene
    // Zeilen mit einem POD. Der Renderer erzeugt ohnehin genau EINE Gruppe je Anhang-Position.
    static_assert(abi::stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0;e=f@1.0.0]">, "kanonische EINE Gruppe.");
    static_assert(!abi::stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0];[e=f@1.0.0]">,
                  "zwei DIREKT aufeinander folgende Geschwister-Gruppen brechen hart (F6).");
    static_assert(abi::stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0];x=y@1.0.0;[e=f@1.0.0]">,
                  "... mit entry dazwischen bleiben sie zulaessig: die Reihenfolge macht sie eindeutig.");
    EXPECT_TRUE((abi::stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0]">));
    EXPECT_FALSE((abi::stamp_line_is_parsable<"a=b@1.0.0[c=d@1.0.0]">));
    EXPECT_FALSE((abi::stamp_line_is_parsable<"[]">));
    EXPECT_FALSE((abi::stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0];[e=f@1.0.0]">));
    EXPECT_TRUE((abi::stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0;e=f@1.0.0]">));
    // Und die REALEN Zeilen bleiben selbstverstaendlich parsbar -- die Haertung darf den Bestand nie treffen.
    EXPECT_TRUE((abi::stamp_line_is_parsable<
                 "target_isa=code@1.0.0c;operating_system=code@1.0.0c;external_utils=code@1.0.0c;[simd=code@1.0.0c]">));
    EXPECT_TRUE((abi::stamp_line_is_parsable<"measurement_tooling=wallclock@1.0.0c;[load_framework=ycsb@1.0.0c]">));
}

TEST(MW12StampBausteine, MeasurementStampLineCarriesToolingMainThenLoadFrameworkKlammer) {
    // W12-A3 (Section 43, Section 47): der Mess-Stempel traegt die gewaehlte Mess-Tooling-HAUPT-Wahl als
    // Eintrag "measurement_tooling=<tooling>@X.Y.Z" -- Voll-Form, SEPARATE Welt zur .algos-Sig.
    // A13-M2 (OP-3-RUECKBAU, Owner-Entscheid E2 vom 02.08.2026): load_framework stand seit O-8 Schritt 9 als
    // ERSTES Segment davor (OP-3, Manager-Entscheid 27.07.). Der Owner-Wortlaut verdraengt ihn -- Meta-Metas
    // werden "einfach dynamisch ans Ende der Kette in den bestehenden Zeilen angehaengt", und zwar in der
    // Klammer-Form (Owner-Q1). load_framework ist die Meta-Meta-HAUPT-Achse des Mess-Realms und steht deshalb
    // jetzt AM ENDE, geklammert. Neu geankert aus dem Ist-Output des A13-M2-Laufs.
    std::string const line = ::comdare::cache_engine::abi::measurement_stamp_line("wallclock");
    EXPECT_EQ(line, std::string{"measurement_tooling=wallclock@1.0.0c;[load_framework=ycsb@1.0.0c]"});
    // EINE Tooling-Haupt-Achse + der Klammer-Anhang -> genau EIN ';'-Trenner (Ablaufmethodik/Workloads sind
    // UNTER-Achsen -> nie Bestandteil).
    EXPECT_EQ(std::count(line.begin(), line.end(), ';'), 1);
    EXPECT_EQ(line.rfind("measurement_tooling=wallclock@1.0.0c", 0), 0u); // ERSTES Segment ist jetzt das Tooling
    EXPECT_TRUE(line.ends_with(";[load_framework=ycsb@1.0.0c]")) << "line=" << line;
    EXPECT_EQ(line.find("@v1"), std::string::npos); // separate Welt zur .algos-Sig (X.Y.Z, nicht roh)
    // Andere Tooling-Haupt-Wahlen materialisieren analog -- der load_framework-Anhang bleibt konstant dahinter.
    EXPECT_EQ(::comdare::cache_engine::abi::measurement_stamp_line("macro"),
              std::string{"measurement_tooling=macro@1.0.0c;[load_framework=ycsb@1.0.0c]"});
    EXPECT_EQ(::comdare::cache_engine::abi::measurement_stamp_line("micro"),
              std::string{"measurement_tooling=micro@1.0.0c;[load_framework=ycsb@1.0.0c]"});
    // Leere Wahl -> leere Zeile (ehrlich: kein Mess-Tooling einkompiliert). AUCH der Klammer-Anhang entfaellt
    // dann: eine Mess-Zeile ohne jedes Tooling ist ehrlich leer und nie ein einsamer Rahmen-Anhang.
    EXPECT_TRUE(::comdare::cache_engine::abi::measurement_stamp_line("").empty());
}

TEST(MW12StampBausteine, MeasurementStampLineSetFormCarriesToolingMenge) {
    // K7b-2 (Section 64-D1-B, 2026-07-22): die MENGEN-Form -- N Tools -> N ';'-getrennte
    // measurement_tooling=<t>@1.0.0c-Eintraege (Eingabe-Reihenfolge, Section-64-Vollmengen-Provenienz). Additive
    // span-Ueberladung; die Einzel-Form oben bleibt unveraendert (der [all]/from_env-LIVE-Pfad routet ueber die Menge).
    // A13-M2 (OP-3-Rueckbau, Owner-E2/Q1): auch die MENGEN-Form fuehrt den load_framework-Anhang EINMAL --
    // aber jetzt geklammert AM ENDE (nicht je Tool und nicht vorne). Er ist eine Eigenschaft der Mess-ZEILE,
    // nicht des einzelnen Tooling-Eintrags. Neu geankert aus dem Ist-Output des A13-M2-Laufs.
    namespace abi                            = ::comdare::cache_engine::abi;
    std::array<std::string_view, 2> const tw = {"wallclock", "macro"};
    EXPECT_EQ(abi::measurement_stamp_line(std::span<std::string_view const>{tw}),
              std::string{"measurement_tooling=wallclock@1.0.0c;measurement_tooling=macro@1.0.0c;"
                          "[load_framework=ycsb@1.0.0c]"});
    // Leere Tokens werden uebersprungen (ehrlich: kein Tool an der Stelle).
    std::array<std::string_view, 3> const gappy = {"wallclock", "", "micro"};
    EXPECT_EQ(abi::measurement_stamp_line(std::span<std::string_view const>{gappy}),
              std::string{"measurement_tooling=wallclock@1.0.0c;measurement_tooling=micro@1.0.0c;"
                          "[load_framework=ycsb@1.0.0c]"});
    // Leere Menge -> leere Zeile. AUCH der load_framework-Anhang entfaellt dann: eine Mess-Zeile ohne jedes
    // Tooling ist ehrlich leer und nicht ein einsamer Rahmen-Anhang.
    EXPECT_TRUE(abi::measurement_stamp_line(std::span<std::string_view const>{}).empty());
    // Die Vollmenge = das volle Registry-Angebot {wallclock,macro,micro} in Registry-Reihenfolge (Single-Source).
    EXPECT_EQ(abi::measurement_stamp_line_full_set(),
              std::string{"measurement_tooling=wallclock@1.0.0c;measurement_tooling=macro@1.0.0c;"
                          "measurement_tooling=micro@1.0.0c;[load_framework=ycsb@1.0.0c]"});
    // SEPARATE Welt zur .algos-Sig: X.Y.Z-Voll-Form, NICHT die rohe "@v1".
    EXPECT_EQ(abi::measurement_stamp_line_full_set().find("@v1"), std::string::npos);
}

TEST(MW12StampBausteine, AnatomyVersionLinesPodLayoutIsStableAt120) {
    // A13-M3 (Owner-E2 02.08.2026): merge_line/merge_len sind ERSATZLOS ENTFERNT -- der ERSTE Feld-ENTFALL
    // dieses POD (bis Layout 5 war alles append-only). Layout-Bump 5 -> 6, sizeof 136 -> 120 (-16 = 1 Zeiger +
    // 1 uint64). Die Offsets von sha512_line und den drei {ptr,count}-Paaren verschieben sich damit um -16 --
    // genau darum ist stamp_pod_has_entries auf die GLEICHHEITS-Wache (== 6) gezogen (K-4). Der
    // sizeof-static_assert lebt in anatomy_module_abi_v1_decl.hpp und haelt build-weit -- hier zusaetzlich als
    // literaler ctest-Beweis gespiegelt. binary_id/CRC UNBERUEHRT (POD-Layout != binary_id).
    using ::comdare::cache_engine::abi::AnatomyVersionLines;
    static_assert(sizeof(AnatomyVersionLines) == 120, "POD-Layout-Wache: 16 Felder, 8-aligned -> 120 Byte (x86_64).");
    static_assert(alignof(AnatomyVersionLines) == 8);
    EXPECT_EQ(sizeof(AnatomyVersionLines), 120u);
    EXPECT_EQ(alignof(AnatomyVersionLines), 8u);
    EXPECT_EQ(::comdare::cache_engine::abi::kAnatomyVersionLinesLayout, 6u);
    // Die drei Zeilen-Paare bis measurement_len liegen unveraendert (der Entfall sitzt DAHINTER).
    EXPECT_EQ(offsetof(AnatomyVersionLines, organ_line), 8u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, system_line), 24u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_line), 40u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_len), 48u);
    // Ab hier hat der merge-Entfall alles um -16 gezogen (vorher sha512_line @72, organ_entries @88 ...).
    EXPECT_EQ(offsetof(AnatomyVersionLines, sha512_line), 56u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, sha512_len), 64u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, organ_entries), 72u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, organ_entry_count), 80u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, system_entries), 88u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, system_entry_count), 96u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_entries), 104u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_entry_count), 112u);
}

TEST(MW12StampBausteine, K4StampPodHasEntriesIsEqualityNotOrder) {
    // A13-M3/K-4: der Feld-ENTFALL verschiebt die Offsets -- ein `>= 5`-Praedikat wuerde ein v6-POD mit
    // v5-Offsets lesen (und umgekehrt) und stillen Zeiger-Muell liefern. Die CT-Negativ-Probe steht im
    // Header selbst (static_assert); hier ist sie als literaler ctest-Beweis gespiegelt.
    namespace abi = ::comdare::cache_engine::abi;
    EXPECT_TRUE(abi::stamp_pod_has_entries(abi::detail::stamp_pod_layout_probe(6u)));
    EXPECT_FALSE(abi::stamp_pod_has_entries(abi::detail::stamp_pod_layout_probe(5u)));
    EXPECT_FALSE(abi::stamp_pod_has_entries(abi::detail::stamp_pod_layout_probe(7u)));
}

TEST(MW12StampBausteine, AnatomyFingerprintHexIsSha512OfSeparatedGlieder) {
    // K7b-3 (Section 62-B) + A13-M3 (OF-M3-1 = Option A, F7): der POD-Stempel == SHA-512 ueber die
    // '\n'-GETRENNTE Glied-Folge als 128-hex, nullterminiert. Selbst-konsistent gegen die K7b-1-Primitive
    // geprueft (kein externer Vektor): das Referenz-Preimage wird hier von Hand zusammengesetzt, damit der
    // Test die Ordnung UND den Trenner beweist und nicht bloss die Funktion gegen sich selbst.
    namespace abi     = ::comdare::cache_engine::abi;
    namespace s5      = ::comdare::cache_engine::sha512;
    constexpr auto fp = abi::anatomy_fingerprint_hex("a", "b", "c");
    static_assert(fp[128] == '\0', "Fingerprint-Zeile nullterminiert");
    // O-2/C-2 (Format 3): das Referenz-Preimage traegt jetzt ACHT Glieder -- Toolchain [5] und bvset [6]
    // liegen zwischen Werteset und Overlay. Sie stehen hier BEWUSST als Konstanten und nicht als leere
    // Literale: waeren sie hart als "" eingesetzt, wuerde dieser Test die Nicht-Injektion zementieren und
    // ausgerechnet dann gruen bleiben, wenn die C-3-Verdrahtung sie fuellt.
    std::string ref_pre;
    ref_pre += abi::kAnatomyFingerprintFormat;
    ref_pre += "\na\nb\nc\n";
    ref_pre += abi::kSubAxisValuesetSegment;
    ref_pre += '\n';
    ref_pre += abi::kToolchainStampGlied;
    ref_pre += '\n';
    ref_pre += abi::kBuildVariantSetSignatureGlied;
    ref_pre += '\n';
    ref_pre += abi::kOverlaySourceHash;
    auto const ref = s5::to_hex(s5::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(ref_pre.data()), ref_pre.size()}));
    for (std::size_t i = 0; i < 128; ++i) EXPECT_EQ(fp[i], ref[i]) << "hex-Stelle " << i;

    // Die Format-Kennung ist das ERSTE Glied (F7: Layout-Evolution mismatcht deterministisch statt still
    // zu kollidieren) und das Werteset-Segment ein EIGENES Glied (F7-VERIFY, "schwerster Befund": sonst
    // wuerde ein Werteset-Bump unter dem SHA512-only-Skip-Gate STILL reused).
    constexpr auto glieder = abi::anatomy_fingerprint_glieder("a", "b", "c");
    static_assert(glieder.size() == 8u); // O-2/C-2: 6 -> 8 (Toolchain + bvset)
    static_assert(glieder[0] == abi::kAnatomyFingerprintFormat);
    static_assert(glieder[4] == abi::kSubAxisValuesetSegment);
    // O-2/C-2: die drei Schwanz-Glieder ueber ihre BENANNTEN Positionen adressiert -- nicht ueber nackte
    // Zahlen. Eine Umsortierung ohne Nachzug der Konstanten bricht damit hier, statt still zu wandern.
    static_assert(glieder[abi::kAnatomyFingerprintToolchainGlied] == abi::kToolchainStampGlied);
    static_assert(glieder[abi::kAnatomyFingerprintBvsetGlied] == abi::kBuildVariantSetSignatureGlied);
    static_assert(glieder[abi::kAnatomyFingerprintOverlayGlied] == abi::kOverlaySourceHash);
    static_assert(abi::kAnatomyFingerprintFormat == std::string_view{"fingerprint_format=3"},
                  "O-2/C-2: der Format-Bump 2 -> 3 ist der Anker dieses Fensters -- er trennt den "
                  "Alt-Bestand deterministisch vom 8-Glieder-Layout.");
    // Dass diese consteval-Quelle byte-gleich zur .algos-Laufzeit-Quelle ist, prueft die schwere TU
    // test_reflect_versions_all17 (dort liegt build_axis_variant_version_table; diese TU bleibt leicht).
}

TEST(MW12StampBausteine, GA01FingerprintPreimageIsInjective) {
    // A13-M3 / OF-M3-1 = Option A (Owner-Entscheid 03.08.2026) -- die NEUE Pflicht-Probe zu Befund GA-01
    // [BLOCK]. VOR M3 entstand das Preimage als reine Byte-Konkatenation OHNE Trenner; die drei folgenden
    // Aufrufe lieferten damit BEWEISBAR DENSELBEN Fingerprint, obwohl die Zeilen-SAETZE verschieden sind.
    // Genau darauf ruht aber das SHA512-only-Skip-Gate (F7: "der deckt die anderen Stempel allein").
    // Es gab im ganzen Baum keine einzige Injektivitaets-Probe -- nur Positiv-Belege.
    namespace abi                 = ::comdare::cache_engine::abi;
    constexpr std::string_view kX = "search_algo=k_ary@1.0.0c";

    // (1) Die drei Feldgrenzen-Verschiebungen der GA-01-Demo: gleiches Zeichenmaterial, andere Zuordnung.
    constexpr auto a = abi::anatomy_fingerprint_hex("", "", kX);
    constexpr auto b = abi::anatomy_fingerprint_hex(kX, "", "");
    constexpr auto c = abi::anatomy_fingerprint_hex("", kX, "");
    static_assert(a != b, "GA-01: Mess-Zeile X vs. Organ-Zeile X muessen verschiedene Fingerprints ergeben.");
    static_assert(a != c, "GA-01: Mess-Zeile X vs. System-Zeile X muessen verschiedene Fingerprints ergeben.");
    static_assert(b != c, "GA-01: Organ-Zeile X vs. System-Zeile X muessen verschiedene Fingerprints ergeben.");
    EXPECT_NE(std::string_view{a.data()}, std::string_view{b.data()});
    EXPECT_NE(std::string_view{a.data()}, std::string_view{c.data()});
    EXPECT_NE(std::string_view{b.data()}, std::string_view{c.data()});

    // (2) Die EIN-ZEICHEN-Grenzverschiebung zwischen Organ- und System-Zeile (das ';' wandert ueber die
    //     Feldgrenze). Ohne Trenner ist das Preimage identisch -- mit Trenner nicht.
    constexpr auto d = abi::anatomy_fingerprint_hex("achse=algo@1.0.0c;", "target_isa=code@1.0.0c", "");
    constexpr auto e = abi::anatomy_fingerprint_hex("achse=algo@1.0.0c", ";target_isa=code@1.0.0c", "");
    static_assert(d != e, "GA-01: die Ein-Zeichen-Grenzverschiebung darf nicht kollabieren.");
    EXPECT_NE(std::string_view{d.data()}, std::string_view{e.data()});

    // (3) Der Trenner liegt BEWEISBAR ausserhalb des Stempel-Zeichenvorrats -- kein Glied kann ihn tragen,
    //     also ist die Zerlegung bei fester Glied-Anzahl eindeutig.
    static_assert(abi::kAnatomyFingerprintSeparator == '\n');
    EXPECT_EQ(abi::kAnatomyFingerprintFormat.find(abi::kAnatomyFingerprintSeparator), std::string_view::npos);
    EXPECT_EQ(abi::kSubAxisValuesetSegment.find(abi::kAnatomyFingerprintSeparator), std::string_view::npos);
    // O-2/C-2: dieselbe Pflicht fuer die beiden NEUEN Glieder. Ohne sie waere die '\n'-Zerlegung
    // ausgerechnet an den Gliedern mehrdeutig, die kuenftig von aussen befuellt werden.
    EXPECT_EQ(abi::kToolchainStampGlied.find(abi::kAnatomyFingerprintSeparator), std::string_view::npos);
    EXPECT_EQ(abi::kBuildVariantSetSignatureGlied.find(abi::kAnatomyFingerprintSeparator), std::string_view::npos);
    EXPECT_EQ(abi::kOverlaySourceHash.find(abi::kAnatomyFingerprintSeparator), std::string_view::npos);

    // (4) O-2/C-2 -- DIE INJEKTIVITAETS-PROBE UEBER DIE ACHT GLIEDER. Dieselbe Feldgrenzen-Verschiebung
    //     wie in (1)/(2), aber jetzt an den drei injizierbaren Schwanz-Gliedern: dasselbe Zeichenmaterial
    //     in Toolchain-, bvset- bzw. Overlay-Slot MUSS drei verschiedene Fingerprints ergeben. Ohne diese
    //     Probe waere der teuerste Teil des Neuankers -- die Unterscheidbarkeit von Toolchain-Wahl und
    //     Enable-Menge -- unbewiesen, und ein spaeterer Slot-Dreher faellt niemandem auf.
    constexpr std::string_view kY = "opt=O3{-O3}@1.0.0c";
    constexpr auto             t =
        abi::anatomy_fingerprint_hex("", "", "", abi::ToolchainGlied{kY}, abi::BvsetGlied{""}, abi::OverlayHash{""});
    constexpr auto v =
        abi::anatomy_fingerprint_hex("", "", "", abi::ToolchainGlied{""}, abi::BvsetGlied{kY}, abi::OverlayHash{""});
    constexpr auto o =
        abi::anatomy_fingerprint_hex("", "", "", abi::ToolchainGlied{""}, abi::BvsetGlied{""}, abi::OverlayHash{kY});
    static_assert(t != v, "O-2/C-2: Toolchain-Glied X vs. bvset-Glied X muessen sich unterscheiden.");
    static_assert(t != o, "O-2/C-2: Toolchain-Glied X vs. Overlay-Glied X muessen sich unterscheiden.");
    static_assert(v != o, "O-2/C-2: bvset-Glied X vs. Overlay-Glied X muessen sich unterscheiden.");
    EXPECT_NE(std::string_view{t.data()}, std::string_view{v.data()});
    EXPECT_NE(std::string_view{t.data()}, std::string_view{o.data()});
    EXPECT_NE(std::string_view{v.data()}, std::string_view{o.data()});

    // (5) WIRKSAMKEIT statt blosser Verschiedenheit: eine BELEGTE Toolchain bzw. bvset-Menge ergibt einen
    //     anderen Fingerprint als der leere Default. Genau das war der C1-/C6-Befund -- vor Format 3
    //     waren diese beiden Groessen im Preimage gar nicht vertreten, zwei Baue mit anderem opt/bt oder
    //     anderer Enable-Menge hatten denselben Digest.
    constexpr auto leer = abi::anatomy_fingerprint_hex("ORGAN", "SYSTEM", "MESS");
    constexpr auto mit_tc =
        abi::anatomy_fingerprint_hex("ORGAN", "SYSTEM", "MESS", abi::ToolchainGlied{"tc=1;opt=O3{-O3}@1.0.0c"});
    constexpr auto mit_bv = abi::anatomy_fingerprint_hex("ORGAN", "SYSTEM", "MESS", abi::ToolchainGlied{""},
                                                         abi::BvsetGlied{"bvset=1;bv=2;page_type[{bplus}]"});
    static_assert(leer != mit_tc, "C1: eine belegte Toolchain MUSS den Fingerprint verschieben.");
    static_assert(leer != mit_bv, "C6: eine belegte Enable-Mengen-Signatur MUSS den Fingerprint verschieben.");
    static_assert(mit_tc != mit_bv);
    EXPECT_NE(std::string_view{leer.data()}, std::string_view{mit_tc.data()});
    EXPECT_NE(std::string_view{leer.data()}, std::string_view{mit_bv.data()});
}

// A1 (G2-4a, 2026-07-23) EINGEFRORENER FINGERPRINT-TESTVEKTOR (Lager-Gate Section 66, Sync-Kante B3). FESTE Stempel-
// Zeilen -> EIN fester 128-hex SHA-512. Zweck: Konsistenz-Anker fuer Lane B (G3-BinaryKeyPolicy, Scheibe B3) --
// Impl-G3-P2 bildet ctsha512 ueber DIESELBE Glied-Folge mit DEMSELBEN Separator und MUSS exakt
// kFrozenFingerprintV1 erhalten. EIN Testvektor, zwei Module. Die Zeilen und der Hex sind EINGEFROREN: NIE
// aendern (bricht die B3-Sync), nur bei bewusstem Fingerprint-Bruch unter Absprache.
//
// O-2/C-2 (05.08.2026, Owner-Entscheid abend-5 = OPTION A) -- DER NEUANKER DIESES FENSTERS: Preimage-Format
// 2 -> 3. Der Vorgaenger-Hex 0fe275bd...9fe36 (A13-M3, 03.08.) ist damit historisch; er steht in der
// git-Historie. URSACHE: zwei zusaetzliche Glieder (Toolchain [5], bvset [6]) und das Overlay-Glied ans Ende
// [7]. Beide neuen Glieder sind HIER NOCH LEER -- die per-Perm-Injektion folgt in Scheibe C-3; den Hex
// verschiebt allein der Format-Bump plus die zwei zusaetzlichen Separatoren. Der Anker faellt damit GENAU
// EINMAL fuer das ganze Buendel, nicht zweimal.
//
// [NEU EINGEFROREN 06.08.2026, NB/CX-4 -- DER ZWEITE UND LETZTE NEUANKER-DREH DIESES BUENDELS. Der
// Vorgaenger-Hex f8f811a9...9137fb0c (O-2/C-2, 05.08.) ist damit historisch; er steht in der git-Historie.
// URSACHE, EINZELN benannt: der Vektor rechnete ueber die LEEREN Default-Glieder [5]/[6] und war damit ein
// Anker fuer einen Zustand, den es produktiv nicht mehr gibt -- seit der Live-Naht tragen beide Glieder in
// jedem realen Bau Werte. Ein Anker, der genau den Teil des Preimage NICHT abdeckt, der neu ist, waere ein
// gruener Test, der die alte Ordnung zementiert. Er ist deshalb in der END-FORM eingefroren: beide Glieder
// mit realistischen, LITERALEN Werten belegt (Fixture-END-Form-Lehre, dieselbe Begruendung wie beim
// A13-M3-Dreh (3)).
// WARUM LITERALE UND NICHT DIE LIVE-WERTE: die Live-Werte haengen an der uebersetzenden Toolchain und an
// der Enable-Menge der Maschine (kDetectedCompilerRealVersion, kDriverBuildVariantSignature). Ein Vektor
// darueber haette in der 8er-Docker-Matrix je Distro einen anderen Wert -- ein EINGEFRORENER Testvektor
// waere dann unmoeglich. Die Literale hier sind bewusst FREMD zur Maschine und damit stabil; die
// WIRKSAMKEIT der Live-Werte beweist stattdessen NbCx4LiveGliederStehenImPreimage (unten).]
//
// HISTORIE A13-M3 (Owner-E2/OF-M3-1, 02./03.08.2026) -- der VORIGE Neuanker, bewusst genau EINER.
// Sein Vorgaenger-Hex 0f0c0eb4...c31b93 (A1, 23.07.) steht ebenfalls in der git-Historie.
// DREI Ursachen fallen in DIESEN einen Commit, weil jede fuer sich einen eigenen Neuanker gekostet haette:
//   (1) OWNER-E2: die merge-ZEILE entfaellt ersatzlos ("Merge Zeile kann daher nicht existieren") -> das
//       frueher hier stehende kMerge-Literal faellt aus dem Preimage;
//   (2) OF-M3-1 = OPTION A (Befund GA-01 [BLOCK]): die Glieder sind ab jetzt '\n'-GETRENNT, tragen die
//       fingerprint_format-Kennung als erstes Glied und das Sub-Achsen-Werteset-Segment als eigenes Glied;
//   (3) FIXTURE-END-FORM (Fixture-Zementierungs-Lehre): die Literale waren inhaltlich VERALTET -- kSystem trug
//       "compiler=code@1.0.0", eine seit O-8 Schritt 4 abgeschaffte System-Haupt-Achse, kMeasure trug
//       "wallclock@1.0.0" ohne Achsen-Praefix, und beide standen in der flaglosen Vor-Q3-Form. Als reiner
//       Hash-Konsistenz-Anker war das gleichgueltig; als REFERENZ-BEISPIEL las es sich falsch. Sie sind hier
//       gleich in der END-Form eingefroren (heutige Achsen + Owner-Q3-Flag "@1.0.0c"), damit die
//       Literal-Migration in C4 KEINEN zweiten Neuanker im selben Fenster erzeugt.
// Der neue Hex wurde NICHT vorausberechnet, sondern aus dem literalen Compiler-/Testlauf uebernommen.
TEST(MW12StampBausteine, FrozenFingerprintTestVectorForLagerGateB3) {
    namespace abi                       = ::comdare::cache_engine::abi;
    constexpr std::string_view kOrgan   = "search_algo=k_ary@1.0.0c;path_compression=path_compression_none@1.0.0c";
    constexpr std::string_view kSystem  = "target_isa=code@1.0.0c;operating_system=code@1.0.0c;"
                                          "external_utils=code@1.0.0c;[simd=code@1.0.0c]";
    constexpr std::string_view kMeasure = "measurement_tooling=wallclock@1.0.0c;[load_framework=ycsb@1.0.0c]";
    // NB/CX-4 END-FORM: die beiden injizierten Glieder sind BELEGT (Literale, s. Kopf). Die Werte sind in
    // der realen Renderer-Form gehalten -- Toolchain-Glied wie render_toolchain_stamp_glied es baut,
    // bvset-Glied wie variant_set_signature es baut -- damit der Anker den ECHTEN Preimage-Bau abdeckt.
    constexpr std::string_view kFrozenToolchain =
        "tc=1;cxx=gcc-16.2.0@1.0.0c;opt=O3{-O3}@1.0.0c;ext=avx512;ceb=8.0;gate=avx512;atomic128=cx16{-mcx16}@1.0.0c";
    constexpr std::string_view kFrozenBvset = "bvset=1;bv=2;page_type[{bplus;hw_cache_line=64;hw_numa_capable=0}];"
                                              "simd_extension[{avx512}];"
                                              "general_hardware[{x86_64;hw_cache_line=64;hw_numa_capable=0}]";
    // EINGEFROREN (Sync mit Lane-B B3): 128-hex SHA-512 ueber die '\n'-getrennte Glied-Folge. NIE aendern.
    constexpr std::string_view kFrozenFingerprintV1 =
        "17148e5a4d0f4a2d96e1f5ad97dc4c727b99fce6e38bd6e337fb6dbf0e4461f9"
        "b7fd37fbba76414be4718ad2180deecbb14387293935a8eff1469cef8ce89374";
    constexpr auto fp = abi::anatomy_fingerprint_hex(kOrgan, kSystem, kMeasure, abi::ToolchainGlied{kFrozenToolchain},
                                                     abi::BvsetGlied{kFrozenBvset});
    static_assert(fp[128] == '\0', "Fingerprint-Zeile nullterminiert");
    static_assert(std::string_view{fp.data()} == kFrozenFingerprintV1,
                  "EINGEFRORENER Fingerprint (B3-Sync): die Zeilen ODER der Hash haben sich geaendert -- unter "
                  "Absprache neu einfrieren, sonst bricht die Lane-B-Konsistenz");
    EXPECT_EQ(std::string_view{fp.data()}, kFrozenFingerprintV1)
        << "eingefrorener Fingerprint-Testvektor (Lager-Gate §66, Sync mit Lane-B Scheibe B3)";
}

// -- O-2/C-2: DAS TOOLCHAIN-GLIED [5] -----------------------------------------------------------------
// Owner-KERN abend-5 (verbatim): "die Flags des Compilers werden in der Tier-Binary statisch verbaut, sind
// also in der Compiler Haupt-Achse ein Teil der Haupt-Achsen Definition selbst". Dieser Test prueft genau
// die drei Zusagen, die daraus folgen: (a) die Flags stehen IM Glied, (b) jede Toolchain-Achse traegt ihre
// VERSION in Q3-Grammatik, (c) das leere Parts-Set rendert die IDENTITAET (leeres Glied).
TEST(MW12StampBausteine, O2ToolchainStampGliedRendersAxesWithFlagsAndVersions) {
    namespace abi = ::comdare::cache_engine::abi;

    // (c) zuerst: ALLES leer => "" (kein Byte im Preimage). Das ist die Zusage, auf der die
    //     Nicht-Injektions-Identitaet dieses Commits ruht -- ohne sie waere der Frozen-Vektor unten
    //     compiler-/umgebungsabhaengig.
    EXPECT_EQ(abi::render_toolchain_stamp_glied(abi::ToolchainStampParts{}), std::string{});

    abi::ToolchainStampParts p{};
    p.cxx_dialect       = "gcc";
    p.cxx_realversion   = "16.1.0"; // G-C4: die REAL erkannte Version, nicht der Treiber-Tag "g++-16"
    p.cxx_driver        = "g++-16"; // NB2-1 (R1): der Tier-Treiber-Tag, Pflicht neben dem Dialekt
    p.opt               = "O3";
    p.opt_flags         = "-O3";
    p.simd              = "avx512";
    p.ceb               = "8.0";
    p.build_type        = "Debug";
    p.gate_contribution = "avx512";
    p.atomic128         = "cx16";
    p.atomic128_flags   = "-mcx16";
    std::string const g = abi::render_toolchain_stamp_glied(p);

    // (a) Flags als Teil der Achsen-DEFINITION -- in der Klammer hinter der id.
    EXPECT_NE(g.find("opt=O3{-O3}@1.0.0c"), std::string::npos) << "glied='" << g << "'";
    EXPECT_NE(g.find("atomic128=cx16{-mcx16}@1.0.0c"), std::string::npos) << "glied='" << g << "'";
    // (b) REAL erkannte Compiler-Version am Dialekt, mit Achsen-Version -- und NB2-1: dahinter, durch ':'
    //     getrennt, der TIER-TREIBER-TAG. Er ERSETZT die Version nicht (das war G-C4s Punkt), er tritt
    //     NEBEN sie: ohne ihn kollabierten g++-17 und g++-18 auf dasselbe Glied.
    EXPECT_NE(g.find("cxx=gcc-16.1.0:g++-16@1.0.0c"), std::string::npos) << "glied='" << g << "'";
    // Der Tag ist NICHT der Versions-Traeger: zwischen Dialekt und Version steht weiter die Realversion,
    // nie der Tag. Genau ein ':' im Feld -- daran haengt die Zerlegbarkeit (NB2-1-Klebepunkt-Regel).
    EXPECT_EQ(std::count(g.begin(), g.end(), ':'), 1) << "glied='" << g << "'";
    // Der Kopf traegt die Glied-Format-Version; die uebrigen Felder stehen als schlichte Paare.
    EXPECT_TRUE(g.starts_with("tc=1;")) << "glied='" << g << "'";
    EXPECT_NE(g.find(";ext=avx512"), std::string::npos);
    EXPECT_NE(g.find(";ceb=8.0"), std::string::npos);
    EXPECT_NE(g.find(";bt=Debug"), std::string::npos);
    EXPECT_NE(g.find(";gate=avx512"), std::string::npos);
    // Leeres Feld => KEIN Segment (dieselbe Regel wie im build_version-Suffix).
    EXPECT_EQ(g.find("target="), std::string::npos);
    EXPECT_EQ(g.find("tel="), std::string::npos);
    // Injektivitaets-Pflicht des Preimage-Glieds.
    EXPECT_EQ(g.find(abi::kAnatomyFingerprintSeparator), std::string::npos);
    EXPECT_LE(g.size(), abi::kAnatomyFingerprintToolchainMax);

    // Die Versions-Tabelle: DREI Toolchain-Achsen, alle in Q3-Grammatik (v + 3-stellig + CPU-Flag).
    EXPECT_EQ(abi::kToolchainAxisCount, std::size_t{3});
    for (auto const& e : abi::kToolchainAxisVersions) {
        EXPECT_FALSE(e.axis.empty());
        EXPECT_EQ(e.version, std::string_view{"v1.0.0c"});
    }
}

// -- O-2/C-2: DIE DOPPEL-WAHRHEITS-WACHE (Suffix vs. Glied) --------------------------------------------
// Der static_assert in system_version_suffix.hpp beweist die ORDNUNG. Dieser Test beweist die WERTE: aus
// EINEM SystemVersionSuffixParts entstehen Suffix und Glied, und jedes Segment, das es in beiden Welten
// gibt, traegt byte-gleich denselben Wert. Ohne diese Haelfte koennte die Ordnung stimmen und der Inhalt
// trotzdem auseinanderlaufen -- eine Binary waere dann anders gestempelt als gekeyt.
TEST(MW12StampBausteine, O2ToolchainGliedAndBuildVersionSuffixShareOneSource) {
    namespace abi = ::comdare::cache_engine::abi;
    namespace pf  = ::comdare::cache_engine::profile_facade;

    pf::SystemVersionSuffixParts sp{};
    sp.cxx               = "g++-16"; // Treiber-Tag: NUR Transport (bewusst asymmetrisch, s. Konverter-Doku)
    sp.opt               = "O3";
    sp.simd              = "avx512";
    sp.ceb               = "8.0";
    sp.target_isa        = "aarch64";
    sp.telemetry         = "silent";
    sp.build_type        = "Debug";
    sp.gate_contribution = "avx512";

    std::string const suffix = pf::compose_system_version_suffix(sp);
    std::string const glied =
        abi::render_toolchain_stamp_glied(pf::toolchain_stamp_parts_from_suffix_parts(sp, "gcc", "16.1.0", "-O3"));

    // Die sieben geteilten Felder: was im Suffix als "+k=v" steht, steht im Glied als "k=v".
    struct Paar {
        char const* suffix_segment;
        char const* glied_segment;
    };
    constexpr Paar kGeteilt[] = {{"+opt=O3", ";opt=O3"},          {"+ext=avx512", ";ext=avx512"},
                                 {"+ceb=8.0", ";ceb=8.0"},        {"+target=aarch64", ";target=aarch64"},
                                 {"+tel=silent", ";tel=silent"},  {"+bt=Debug", ";bt=Debug"},
                                 {"+gate=avx512", ";gate=avx512"}};
    for (auto const& [suf, gl] : kGeteilt) {
        EXPECT_NE(suffix.find(suf), std::string::npos) << "suffix='" << suffix << "'";
        EXPECT_NE(glied.find(gl), std::string::npos) << "glied='" << glied << "'";
    }
    // opt traegt im Glied ZUSAETZLICH seine Flags und seine Achsen-Version -- das ist der Mehrwert der
    // Identitaets-Seite, kein Widerspruch: die id ist dieselbe.
    EXPECT_NE(glied.find(";opt=O3{-O3}@1.0.0c"), std::string::npos) << "glied='" << glied << "'";
    // NB2-1: der TREIBER-TAG steht jetzt in BEIDEN Welten und kommt aus DERSELBEN Quelle (sp.cxx) -- das
    // Glied kann also gar keinen anderen Treiber nennen als den, unter dem gebaut wird. Die verbleibende
    // Asymmetrie ist nur noch die Realversion: sie steht im Glied und hat im Suffix kein Gegenstueck.
    EXPECT_NE(suffix.find("+cxx=g++-16"), std::string::npos);
    EXPECT_NE(glied.find("cxx=gcc-16.1.0:g++-16@"), std::string::npos) << "glied='" << glied << "'";
}

TEST(MW12StampBausteine, PlannerVersionStampCarriesSelfVersionAndIsaOs) {
    // CX-W5 (Codex-Doppelreview 02.08.2026): das ROH-Literal traegt jetzt das 'v' (Owner-Q10) -- frueher stand
    // hier "1.0.0" und ZEMENTIERTE die alte, Q10-widrige Form. Die GERENDERTE Zeile bleibt praefixfrei
    // "planner@1.0.0c" (A13-M3/C4: das 'v' faellt beim Rendern weg, das HW-Flag bleibt).
    EXPECT_EQ(pl::kPlannerVersion, std::string_view{"v1.0.0c"}); // Roh-Literal mit 'v' (Owner-Q10)
    EXPECT_EQ(pl::planner_target_isa(), std::string_view{"x86_64"});
    std::string const stamp = pl::planner_version_stamp();
    EXPECT_NE(stamp.find("planner@1.0.0c"), std::string::npos) << "stamp='" << stamp << "'"; // render byte-identisch
    EXPECT_EQ(stamp.find("planner@v"), std::string::npos) << "gerenderte Form traegt KEIN 'v' (Owner-Q10)";
    EXPECT_NE(stamp.find("isa=x86_64"), std::string::npos) << "stamp='" << stamp << "'";
    EXPECT_NE(stamp.find("os="), std::string::npos) << "stamp='" << stamp << "'";
}

// A2 (G2-4 Schritt 3+4): System-Achsen-Code-Versionen + Mess-Tooling-Version aus Single-Sources statt Hartkodierung.
// Bei Anlage render-neutral; seit A13-M3/C4 tragen die gueltigen ids/Achsen das CPU-Flag ("1.0.0c",
// deklariertes Byte-Ereignis). "v0.0.0"-Sentinel weiterhin nur fuer ungueltige Tooling-ids (A13-M1b).
TEST(MW12StampBausteine, A2SystemAndToolingCodeVersionsSingleSource) {
    namespace abi = ::comdare::cache_engine::abi;
    // (a) System-Achsen-Single-Source: DREI Achsen (O-8 Schritt 4, A3-Kern). A13-M3/C4: alle drei tragen
    // seit dem Migrations-Commit das CPU-Flag ("v1.0.0c", Owner-Q3) -- die Version selbst ist NICHT gebumpt.
    EXPECT_EQ(abi::kSystemAxisCodeCount, std::size_t{3});
    for (auto const& e : abi::kSystemAxisCodeVersions) {
        EXPECT_FALSE(e.axis.empty());
        EXPECT_EQ(e.version, std::string_view{"v1.0.0c"});
    }
    // Der frueher hier stehende "Neutralitaets-Anker" (byte-identisch zur alten v1-Hartkodierung) ist mit dem
    // A3-Rueckbau gegenstandslos: die Zeile SOLL sich geaendert haben. Sie ist aus dem Werkzeug-Output des
    // Fenster-Laufs neu geankert und bindet jetzt die Drei-Achsen-Ordnung.
    // A13-M2 (Owner-E2 + Q1): NEU GEANKERT -- hinter den drei Haupt-Achsen steht jetzt der Meta-Meta-
    // KLAMMER-ANHANG "[simd=code@1.0.0c]". Das ist der beabsichtigte Byte-Wechsel der System-Zeile (und damit
    // des Fingerprints ALLER kuenftigen Binaries), nicht eine Drift: vor Voll-Bau-4 existiert kein
    // schuetzenswerter Bestand, das eine Neuanker-Fenster ist genau hier.
    EXPECT_EQ(abi::system_stamp_line(), std::string{"target_isa=code@1.0.0c;operating_system=code@1.0.0c;"
                                                    "external_utils=code@1.0.0c;[simd=code@1.0.0c]"});

    // (b) Mess-Tooling-Version-Feld + id-Lookup.
    for (auto const& t : m::kMeasurementToolingRegistry) EXPECT_EQ(t.version, std::string_view{"v1.0.0c"});
    EXPECT_EQ(m::tooling_version_for_id("wallclock"), std::string_view{"v1.0.0c"});
    EXPECT_EQ(m::tooling_version_for_id("macro"), std::string_view{"v1.0.0c"});
    EXPECT_EQ(m::tooling_version_for_id("micro"), std::string_view{"v1.0.0c"});
    // A13-M1b (Owner-Q3, dreistellig): der Sentinel-Rueckgabewert ist "v0.0.0" statt der Kurzform "v0" --
    // byte-neutral, beide rendern "0.0.0" (Beleg im Render-Block (c) unten: "@0.0.0" unveraendert).
    EXPECT_EQ(m::tooling_version_for_id("bogus"), std::string_view{"v0.0.0"}); // unbekannt -> Sentinel

    // (c) Sentinel-Render: ungueltige Tooling-id -> @0.0.0 (flaglos, der Sentinel traegt nie ein Flag);
    //     gueltige rendern seit C4 @1.0.0c.
    // A13-M2: der load_framework-Anhang steht geklammert AM ENDE -- der Sentinel betrifft nur das Tooling-Glied.
    EXPECT_EQ(abi::measurement_stamp_line("bogus"),
              std::string{"measurement_tooling=bogus@0.0.0;[load_framework=ycsb@1.0.0c]"});
    EXPECT_EQ(abi::measurement_stamp_line("wallclock"),
              std::string{"measurement_tooling=wallclock@1.0.0c;[load_framework=ycsb@1.0.0c]"});
}

// A3 (G2-1a): Entry-POD AnatomyStampEntryV1 (48B-Pin) + consteval count/parse_stamp_entries + parse_dotted_semver.
// Reine Parser-/POD-Vorstufe (POD waechst erst in A4); tokenisiert die gerenderten "achse=algo@X.Y.Z"-Zeilen.
TEST(MW12StampBausteine, A3AnatomyStampEntryPodAndConstevalParser) {
    namespace abi = ::comdare::cache_engine::abi;
    // (a) Entry-POD sizeof/align-Pin.
    EXPECT_EQ(sizeof(abi::AnatomyStampEntryV1), std::size_t{48});
    EXPECT_EQ(alignof(abi::AnatomyStampEntryV1), std::size_t{8});
    static_assert(sizeof(abi::AnatomyStampEntryV1) == 48);

    // (b) parse_dotted_semver = Umkehrung von algo_semver_string (dotted "X.Y.Z", OHNE 'v').
    EXPECT_EQ(m::parse_dotted_semver("1.0.0"), (m::AlgoSemVer{1, 0, 0}));
    EXPECT_EQ(m::parse_dotted_semver("2.3.4"), (m::AlgoSemVer{2, 3, 4}));
    EXPECT_EQ(m::parse_dotted_semver("v1.0.0"), (m::AlgoSemVer{0, 0, 0})); // rohe Form -> Sentinel
    EXPECT_EQ(m::parse_dotted_semver("1.0"), (m::AlgoSemVer{0, 0, 0}));    // Kurzform -> Sentinel

    // (c) count_stamp_entries: leer -> 0; N Segmente -> N.
    EXPECT_EQ(abi::count_stamp_entries(""), std::size_t{0});
    EXPECT_EQ(abi::count_stamp_entries("measurement_tooling=wallclock@1.0.0"), std::size_t{1});
    EXPECT_EQ(abi::count_stamp_entries("a=b@1.0.0;c=d@2.3.4;e=f@0.0.0"), std::size_t{3});

    // (d) parse_stamp_entries: Tokenisierung + {ptr,len}-Rekonstruktion == exakter Teilstring, X.Y.Z korrekt.
    static constexpr char kLit[]  = "search_algo=k_ary@1.0.0;filter=bloom@2.3.4";
    constexpr auto        entries = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kLit})>(kLit);
    static_assert(entries.size() == 2);
    EXPECT_EQ(std::string_view(entries[0].axis, entries[0].axis_len), std::string_view{"search_algo"});
    EXPECT_EQ(std::string_view(entries[0].algorithm, entries[0].algo_len), std::string_view{"k_ary"});
    EXPECT_EQ(entries[0].x, 1u);
    EXPECT_EQ(entries[0].y, 0u);
    EXPECT_EQ(entries[0].z, 0u);
    EXPECT_EQ(std::string_view(entries[1].axis, entries[1].axis_len), std::string_view{"filter"});
    EXPECT_EQ(std::string_view(entries[1].algorithm, entries[1].algo_len), std::string_view{"bloom"});
    EXPECT_EQ(entries[1].x, 2u);
    EXPECT_EQ(entries[1].y, 3u);
    EXPECT_EQ(entries[1].z, 4u);
    // consteval-Beweis: die Rekonstruktion haelt schon compile-time.
    static_assert(std::string_view(entries[0].axis, entries[0].axis_len) == "search_algo");
    static_assert(entries[1].z == 4u);

    // (e) Sentinel: nie nullptr, leere Felder (""-Doktrin).
    EXPECT_NE(abi::kAnatomyStampNoEntries[0].axis, nullptr);
    EXPECT_EQ(abi::kAnatomyStampNoEntries[0].axis_len, std::uint64_t{0});
    // A13-M1: der Sentinel-Eintrag traegt KEIN Flag (reserved == 0).
    EXPECT_EQ(abi::kAnatomyStampNoEntries[0].reserved, std::uint32_t{0});
}

// A13-M1/M1b (Owner-E2 + Nachtraege Q2/Q3 vom 02.08.2026): das 'e' reist als Bit 0, das Hardware-Flag als
// Bits 1-2 im reserved-Feld des 48-Byte-Entry-PODs (kein sizeof-/Layout-Bruch); hierarchische Namen
// "prt-art.memory.abc@1.0.0c" bleiben reiner NAMENS-Anteil.
TEST(MW12StampBausteine, A13M1bStampEntryCarriesFlagBitsAndTolerantNames) {
    namespace abi = ::comdare::cache_engine::abi;
    // (a) Entry-POD-Groesse UNVERAENDERT -- beide Flag-Gruppen nutzen das vorgesehene reserved-Feld.
    static_assert(sizeof(abi::AnatomyStampEntryV1) == 48);
    EXPECT_EQ(abi::kStampEntryFlagExperimental, std::uint32_t{1});
    EXPECT_EQ(abi::kStampEntryHwFlagShift, std::uint32_t{1});
    EXPECT_EQ(abi::kStampEntryHwFlagMask, std::uint32_t{6}); // Bits 1-2
    // Die Bit-Gruppen ueberlappen NICHT (sonst wuerde ein Hardware-Flag das 'e' ueberschreiben).
    EXPECT_EQ(abi::kStampEntryFlagExperimental & abi::kStampEntryHwFlagMask, std::uint32_t{0});

    // (b) Owner-Q2-Namens-Toleranz: Punkte im Namens-Anteil VOR dem '@' sind Namens-Bestandteil (Achse UND
    //     Algorithmus), Punkte NACH dem '@' bleiben reine Zahlen-Trenner. Owner-Q3-Voll-Form "ce".
    static constexpr char kLit[] = "prt-art.memory.abc=prt_patricia.simd@2.3.4ce;filter=bloom@1.0.0";
    constexpr auto        e      = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kLit})>(kLit);
    static_assert(e.size() == 2);
    EXPECT_EQ(std::string_view(e[0].axis, e[0].axis_len), std::string_view{"prt-art.memory.abc"});
    EXPECT_EQ(std::string_view(e[0].algorithm, e[0].algo_len), std::string_view{"prt_patricia.simd"});
    EXPECT_EQ(e[0].x, 2u);
    EXPECT_EQ(e[0].y, 3u);
    EXPECT_EQ(e[0].z, 4u);
    static_assert(std::string_view(e[0].axis, e[0].axis_len) == "prt-art.memory.abc");

    // (c) 'ce' -> Bit 0 gesetzt, Hardware-Bits 0 (c IST der Default); ohne Flags bleibt reserved sauber 0.
    EXPECT_TRUE(abi::stamp_entry_is_experimental(e[0]));
    EXPECT_EQ(abi::stamp_entry_hardware_flag(e[0]), abi::StampEntryHardwareFlag::cpu);
    EXPECT_EQ(e[0].reserved, abi::kStampEntryFlagExperimental);
    EXPECT_FALSE(abi::stamp_entry_is_experimental(e[1]));
    EXPECT_EQ(abi::stamp_entry_hardware_flag(e[1]), abi::StampEntryHardwareFlag::cpu); // Default-Lesart
    EXPECT_EQ(e[1].reserved, std::uint32_t{0});
    static_assert(abi::stamp_entry_is_experimental(e[0]));
    static_assert(!abi::stamp_entry_is_experimental(e[1]));

    // (c2) A13-M1b: die NICHT-CPU-Flags belegen die Bits 1-2 PAARWEISE VERSCHIEDEN -- sonst waeren zwei
    //      Hardware-Staende im POD ununterscheidbar (Lager-Key-Kollision). 'ne' setzt beide Gruppen zugleich.
    static constexpr char kHw[] = "a=x@1.0.0g;b=y@1.0.0f;c=z@1.0.0n;d=w@1.0.0ne";
    constexpr auto        h     = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kHw})>(kHw);
    static_assert(h.size() == 4);
    EXPECT_EQ(abi::stamp_entry_hardware_flag(h[0]), abi::StampEntryHardwareFlag::gpu);
    EXPECT_EQ(abi::stamp_entry_hardware_flag(h[1]), abi::StampEntryHardwareFlag::fpga);
    EXPECT_EQ(abi::stamp_entry_hardware_flag(h[2]), abi::StampEntryHardwareFlag::npu);
    EXPECT_EQ(abi::stamp_entry_hardware_flag(h[3]), abi::StampEntryHardwareFlag::npu);
    EXPECT_TRUE(abi::stamp_entry_is_experimental(h[3]));
    EXPECT_FALSE(abi::stamp_entry_is_experimental(h[2]));
    EXPECT_NE(h[0].reserved, h[1].reserved);
    EXPECT_NE(h[1].reserved, h[2].reserved);
    EXPECT_NE(h[2].reserved, h[3].reserved);
    EXPECT_NE(h[0].reserved, e[1].reserved); // g != flaglose Form
    static_assert(abi::stamp_entry_hardware_flag(h[1]) == abi::StampEntryHardwareFlag::fpga);

    // (c3) A13-M3/C2b (Befund Z-09) -- GEDREHT. Bis C2b hielt hier fest, dass ein ZWEITES Hardware-Flag
    //      "grammatisch Sentinel" sei und still auf @0.0.0/reserved==0 falle. Genau das war das fail-open:
    //      zwei byte-VERSCHIEDENE Defekt-Formen ergaben denselben POD, obwohl der Stempel die Lager-
    //      Identitaet ist. Ab C2b bricht die Form im consteval-Pfad HART -- die Aussage steht deshalb als
    //      Negativ-Beweis (das Praedikat dreht die Nicht-Parsbarkeit in eine positive, beweisbare Aussage).
    static_assert(!abi::stamp_line_is_parsable<"a=x@1.0.0cg">, "zweites Hardware-Flag bricht hart (Z-09).");
    EXPECT_FALSE((abi::stamp_line_is_parsable<"a=x@1.0.0cg">));
    // ... und die POSITIVE Restaussage bleibt: der Parser raet NIE ein Flag herbei -- das dokumentierte
    //     Sentinel-Rendering "@0.0.0" ist zulaessig UND flaglos.
    static constexpr char kSentinel[] = "a=x@0.0.0";
    constexpr auto sen = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kSentinel})>(kSentinel);
    EXPECT_EQ(sen[0].x, 0u);
    EXPECT_EQ(sen[0].reserved, std::uint32_t{0});

    // (d) GOLDEN-NEUTRALITAET: eine Zeile OHNE Flags setzt in KEINEM Eintrag ein Bit. Das Literal ist
    //     bewusst flaglos -- genau das ist die Aussage; die reale Bestands-Zeile traegt seit A13-M3/C4
    //     ueberall 'c' und faellt (c == Code 0) auf dasselbe reserved == 0.
    static constexpr char kPlain[] =
        "search_algo=k_ary@1.0.0;filter=bloom@2.3.4;target_isa=code@1.0.0"; // FLAGLOSE Probe, kein Bestand
    constexpr auto p = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kPlain})>(kPlain);
    for (auto const& x : p) EXPECT_EQ(x.reserved, std::uint32_t{0});

    // (e) A13-M3/C2b (Z-09) -- ebenfalls GEDREHT: ein 'e' im VERSIONS-Anteil an falscher Stelle ("1.0e",
    //     Kurzform mit Flag-Schwanz) fiel bis C2b still auf den Sentinel. Ab jetzt bricht er benannt; ebenso
    //     die beiden Struktur-Fehlformen ohne '=' bzw. ohne '@'.
    static_assert(!abi::stamp_line_is_parsable<"achse=algo@1.0e">, "Kurzform mit Flag-Schwanz bricht hart.");
    static_assert(!abi::stamp_line_is_parsable<"achse=algo">, "Segment ohne '@' bricht hart.");
    static_assert(!abi::stamp_line_is_parsable<"nur_ein_name">, "Segment ohne '=' bricht hart.");
    EXPECT_FALSE((abi::stamp_line_is_parsable<"achse=algo@1.0e">));
    EXPECT_FALSE((abi::stamp_line_is_parsable<"achse=algo">));
    EXPECT_FALSE((abi::stamp_line_is_parsable<"nur_ein_name">));
}

// A4 (G2-1b): die Array-Form reist durch das AnatomyVersionLines-POD. Der POD wird hier MANUELL exakt wie im
// COMDARE_ANATOMY_VERSION_STAMP_M-Makro konstruiert (dieselbe Feld-Reihenfolge; die Aggregat-Init ist positions-
// UND typgeprueft -> eine Feld-Vertauschung Zeiger<->uint64 waere ein Compile-Fehler). Der REALE Makro-POD wird
// zusaetzlich vom Struktur-Smoke ueber echte DLL-Builds kompiliert. Beweis: entry_counts {18,3,4} + join(entries)==Zeile.
//
// O-8 Schritt 12: die drei Fixture-Zeilen sind SYNTHETISCH (Kurz-Algo-Namen "t"/"m"/"p") -- der Test prueft die
// POD-Mechanik, nicht die Welt. Ihre ACHSEN-NAMEN und -ZAHLEN sind trotzdem auf den Fenster-Stand nachgezogen
// (18 Organ-Slots, 3 System-Achsen in der A2/A3-Ordnung, load_framework als erstes Mess-Segment): der Test war
// gruen und ist es geblieben, aber ein Fixture, der die abgeschaffte 5-Achsen-Ordnung mit compiler/
// extension_hardware/scheduling stehen laesst, liest sich fuer den Naechsten wie eine gueltige Referenz.
TEST(MW12StampBausteine, A4AnatomyStampArraysRoundtripThroughPod) {
    namespace abi = ::comdare::cache_engine::abi;
    static constexpr char kOrgan[] =
        "search_algo=k_ary@1.0.0c;cache_traversal=t@1.0.0c;mapping=m@1.0.0c;path_compression=p@2.3.4c;"
        "node_type=n@1.0.0c;memory_layout=l@1.0.0c;allocator=a@1.0.0c;prefetch=pf@1.0.0c;concurrency=c@1.0.0c;"
        "serialization=s@1.0.0c;value_handle=v@1.0.0c;index_organization=i@1.0.0c;io_dispatch=io@1.0.0c;"
        "migration_policy=mp@1.0.0c;filter=f@1.0.0c;queuing_q1=q1@1.0.0c;queuing_q2=q2@1.0.0c;"
        "persistence_target=pt@1.0.0c"; // 18 Haupt-Achsen (A8.2), A13-M3/C4-Flag-Form
    // A13-M2: BEIDE Nicht-Organ-Fixtures sind auf die Klammer-Welt nachgezogen (Owner-E2/Q1) -- System-Zeile
    // 3 -> 4 Eintraege (Meta-Meta-Anhang), Mess-Zeile mit load_framework GEKLAMMERT AM ENDE statt vorne. Ein
    // Fixture, der die alte Ordnung stehen laesst, liest sich fuer den Naechsten wie eine gueltige Referenz.
    static constexpr char kSystem[]  = "target_isa=code@1.0.0c;operating_system=code@1.0.0c;"
                                       "external_utils=code@1.0.0c;[simd=code@1.0.0c]"; // 3 + 1 Meta-Meta (A13-M2)
    static constexpr char kMeasure[] = "measurement_tooling=wallclock@1.0.0c;measurement_tooling=macro@1.0.0c;"
                                       "measurement_tooling=micro@1.0.0c;[load_framework=ycsb@1.0.0c]"; // 3 + 1

    static constexpr auto kOE = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kOrgan})>(kOrgan);
    static constexpr auto kSE = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kSystem})>(kSystem);
    static constexpr auto kME =
        abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kMeasure})>(kMeasure);

    abi::AnatomyVersionLines const v{abi::kAnatomyVersionLinesLayout, 0u, kOrgan, sizeof(kOrgan) - 1, kSystem,
                                     sizeof(kSystem) - 1, kMeasure, sizeof(kMeasure) - 1,
                                     // A13-M3: die merge-Slots ("" / 0u) sind hier ERSATZLOS entfallen (18 -> 16
                                     // Initialisierer) -- sha512_line folgt jetzt unmittelbar auf measurement_len.
                                     "deadbeef", 8u, abi::stamp_entries_ptr(kOE), kOE.size(),
                                     abi::stamp_entries_ptr(kSE), kSE.size(), abi::stamp_entries_ptr(kME), kME.size()};

    EXPECT_TRUE(abi::stamp_pod_has_entries(v));
    EXPECT_EQ(v.stamp_layout_version, 6u);
    EXPECT_EQ(v.organ_entry_count, 18u);
    EXPECT_EQ(v.system_entry_count, 4u); // A13-M2: 3 Haupt-Achsen + 1 geklammerte Meta-Meta
    EXPECT_EQ(v.measurement_entry_count, 4u);
    // A13-M2: die Meta-Meta-Eintraege reisen mit ihrer EBENE durch das POD -- ein Konsument unterscheidet
    // Haupt-Achse und Meta-Meta ohne die Zeile erneut zu tokenisieren.
    EXPECT_EQ(abi::stamp_entry_meta_level(v.system_entries[2]), 0u);
    EXPECT_EQ(abi::stamp_entry_meta_level(v.system_entries[3]), 1u);
    EXPECT_EQ(abi::stamp_entry_meta_level(v.measurement_entries[2]), 0u);
    EXPECT_EQ(abi::stamp_entry_meta_level(v.measurement_entries[3]), 1u);
    for (std::uint64_t i = 0; i < v.organ_entry_count; ++i)
        EXPECT_EQ(abi::stamp_entry_meta_level(v.organ_entries[i]), 0u)
            << "die Organ-Zeile traegt heute KEINE Meta-Meta (leere Typliste, no-op). i=" << i;

    // A13-M2: die Rekonstruktion faehrt die KLAMMER-Grammatik mit -- aus (Text, EBENE) je Eintrag entsteht
    // die Zeile VERLUSTFREI zurueck. Das ist der eigentliche Beweis der Grammatik: haette der Renderer eine
    // andere Trenner-Regel als der Parser, faellt genau hier die Gleichheit.
    auto const join = [](abi::AnatomyStampEntryV1 const* e, std::uint64_t n) {
        std::string   out;
        std::uint32_t prev = 0;
        for (std::uint64_t i = 0; i < n; ++i) {
            std::uint32_t const lvl = abi::stamp_entry_meta_level(e[i]);
            if (i == 0) {
                for (std::uint32_t k = 0; k < lvl; ++k) out += '[';
            } else if (lvl > prev) {
                for (std::uint32_t k = prev; k < lvl; ++k) out += ";[";
            } else {
                for (std::uint32_t k = lvl; k < prev; ++k) out += ']';
                out += ';';
            }
            out += std::string(e[i].axis, e[i].axis_len);
            out += '=';
            out += std::string(e[i].algorithm, e[i].algo_len);
            out += '@';
            out += std::to_string(e[i].x) + '.' + std::to_string(e[i].y) + '.' + std::to_string(e[i].z);
            // A13-M3/C4: der FLAG-SCHWANZ gehoert zur gerenderten Version und muss mit zurueckgeschrieben
            // werden -- ohne ihn waere die Rekonstruktion seit der Migration verlustbehaftet. EHRLICHE
            // GRENZE (Belegung kStampEntryHwCodeCpu == 0, Owner-Q3 "c ist Default"): der POD unterscheidet
            // "kein Flag" NICHT von "c". Die Rekonstruktion einer FLAGLOSEN Zeile traegt daher ein 'c' --
            // fuer den migrierten Bestand ist das exakt richtig, fuer eine Alt-Zeile waere es die bewusst
            // in Kauf genommene Normalisierung.
            if (char const hw =
                    ::comdare::cache_engine::measurement::hardware_flag_char(abi::stamp_entry_hardware_flag(e[i]));
                hw != '\0')
                out += hw;
            if (abi::stamp_entry_is_experimental(e[i])) out += 'e';
            prev = lvl;
        }
        for (std::uint32_t k = 0; k < prev; ++k) out += ']';
        return out;
    };
    EXPECT_EQ(join(v.organ_entries, v.organ_entry_count), std::string(v.organ_line, v.organ_len));
    EXPECT_EQ(join(v.system_entries, v.system_entry_count), std::string(v.system_line, v.system_len));
    EXPECT_EQ(join(v.measurement_entries, v.measurement_entry_count),
              std::string(v.measurement_line, v.measurement_len));

    // Leeres Mess-Array (kein Tooling) -> Sentinel-Zeiger (nie nullptr), count 0.
    static constexpr auto kEmpty = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{""})>("");
    EXPECT_EQ(abi::stamp_entries_ptr(kEmpty), abi::kAnatomyStampNoEntries);
    EXPECT_EQ(kEmpty.size(), std::size_t{0});
}

// A5 (G2-1, A8): der CEB-Selbst-Stempel -- consteval Mess-Array-Zeile aus der Registry + eigene SHA-512-Provenienz.
// Kern-Beweis: die consteval-CEB-Zeile ist DRIFT-FREI zur Runtime-Tier-Binary-Mengen-Form (eine Wahrheit).
TEST(MW12StampBausteine, A5CebVersionStampComposesMeasurementArrayAndSha512) {
    namespace ceb = ::comdare::cache_engine::builder;
    namespace abi = ::comdare::cache_engine::abi;
    // Mess-Array-Zeile aus der Registry, X.Y.Z-gerendert (nicht die rohe v-Form).
    // A13-M2 (OP-3-Rueckbau, Owner-E2/Q1): der ZWILLING ist symmetrisch nachgezogen -- load_framework steht
    // als geklammerter Meta-Meta-Anhang AM ENDE. Genau dieser Header war der von O-8 Schritt 12 dokumentierte
    // DRITTE Ableitungsweg; wer ihn beim Umbau vergisst, bekommt dieselbe Drift zurueck.
    EXPECT_EQ(ceb::kCebMeasurementStamp, std::string_view{"measurement_tooling=wallclock@1.0.0c;"
                                                          "measurement_tooling=macro@1.0.0c;"
                                                          "measurement_tooling=micro@1.0.0c;"
                                                          "[load_framework=ycsb@1.0.0c]"});
    // DRIFT-GUARD: die consteval-CEB-Zeile deckt sich EXAKT mit der Runtime-Tier-Binary-Mengen-Form -> EINE Wahrheit,
    // keine Parallel-Ableitung (Section-64-Vollmengen-Provenienz teilt sich die Quelle).
    EXPECT_EQ(std::string{ceb::kCebMeasurementStamp}, abi::measurement_stamp_line_full_set());
    // SHA-512-Provenienz: 128 hex, == Host-Nachrechnung via anatomy_fingerprint_hex ("","",mess).
    // A13-M3/K-1: hier stand die 4-arg-Form mit dem merge-"" -- genau der Alt-Aufruf, den die Sperre faengt.
    // Sie hat literal gefeuert ("die merge-ZEILE existiert nicht mehr ... das 4. Argument ist der
    // OverlayHash-TYP"); der Aufruf ist auf die 3-arg-Form gezogen.
    static_assert(ceb::kCebFingerprint.size() == 128);
    EXPECT_EQ(ceb::kCebFingerprint.size(), std::size_t{128});
    constexpr auto host = abi::anatomy_fingerprint_hex("", "", ceb::kCebMeasurementStamp);
    EXPECT_EQ(ceb::kCebFingerprint, std::string_view(host.data(), 128));
    // ceb_version_stamp() traegt beide Teile + die X.Y.Z-Form (keine rohe @v1).
    std::string const stamp = ceb::ceb_version_stamp();
    EXPECT_NE(stamp.find("ceb-measurement=measurement_tooling=wallclock@1.0.0c"), std::string::npos);
    EXPECT_NE(stamp.find(";[load_framework=ycsb@1.0.0c];sha512="), std::string::npos) << "stamp=" << stamp;
    EXPECT_NE(stamp.find(";sha512="), std::string::npos);
    EXPECT_EQ(stamp.find("@v1"), std::string::npos);
}

// -- NB/CX-3: DIE COMPILER-REAL-VERSIONS-ERHEBUNG (compile-time) ---------------------------------------
// G-C4/OE-C verlangt die REAL ERKANNTE Version statt des Treiber-Namens. Bis zum NB-Fenster war
// ToolchainStampParts.cxx_realversion ein Slot OHNE Erhebung -- dieser Test prueft, dass er jetzt gefuellt
// werden KANN und dass die Erhebung die Form haelt, an der die Injektivitaet des Glieds haengt.
TEST(MW12StampBausteine, NbCx3CompilerRealVersionIsDetectedAtCompileTime) {
    namespace abi = ::comdare::cache_engine::abi;
    namespace cm  = ::comdare::cache_engine::measurement;

    // (a) Auf jeder Toolchain, mit der dieses Projekt uebersetzt wird (gcc oder clang), ist die Erhebung
    //     ERFOLGREICH. Ein leeres Ergebnis waere kein Stil-Mangel, sondern der fail-closed-Fall.
    static_assert(abi::kDetectedCompilerIsKnown,
                  "NB/CX-3: weder __clang__ noch __GNUC__ -- diese Toolchain kann keine Realversion stempeln.");
    EXPECT_TRUE(abi::kDetectedCompilerIsKnown);

    // (b) Der DIALEKT kommt aus der Compiler-System-Achse, nicht aus einem Literal.
    EXPECT_TRUE(abi::kDetectedCompilerDialect == cm::GccCompilerAxis::compiler_id() ||
                abi::kDetectedCompilerDialect == cm::ClangCompilerAxis::compiler_id())
        << "dialekt='" << abi::kDetectedCompilerDialect << "'";

    // (c) Die FORM ist <major>.<minor>.<patch> -- drei Zahlen, zwei Punkte, sonst nichts. Genau daran haengt,
    //     dass der Wert kein Struktur-Zeichen des Glieds traegt.
    std::size_t punkte = 0;
    for (char const c : abi::kDetectedCompilerRealVersion) {
        if (c == '.') {
            ++punkte;
            continue;
        }
        EXPECT_TRUE(c >= '0' && c <= '9') << "unerwartetes Zeichen in der Realversion: '" << c << "'";
    }
    EXPECT_EQ(punkte, std::size_t{2}) << "realversion='" << abi::kDetectedCompilerRealVersion << "'";
    EXPECT_TRUE(abi::toolchain_wert_ist_wohlgeformt(abi::kDetectedCompilerRealVersion));
    EXPECT_TRUE(abi::toolchain_dialekt_ist_wohlgeformt(abi::kDetectedCompilerDialect));

    // (d) __clang__-VORRANG: clang definiert __GNUC__ MIT und meldet dort die emulierte GCC-Version. Wird
    //     der Vorrang je gedreht, stempelt ein clang-Bau als "gcc-4.2.1". Die Zusicherung ist deshalb
    //     mechanisch und nicht nur ein Kommentar im Header.
#if defined(__clang__)
    EXPECT_EQ(abi::kDetectedCompilerDialect, cm::ClangCompilerAxis::compiler_id())
        << "__clang__ ist gesetzt -- der Dialekt MUSS clang sein, nicht die emulierte GCC-Identitaet";
#elif defined(__GNUC__)
    EXPECT_EQ(abi::kDetectedCompilerDialect, cm::GccCompilerAxis::compiler_id());
#endif

    // (e) WIRKSAMKEIT: die Erhebung laesst sich in ein Glied rendern, und dort steht der DIALEKT-VERSION-
    //     Verbund -- nie der Treiber-Tag.
    abi::ToolchainStampParts p{};
    p.cxx_dialect     = abi::kDetectedCompilerDialect;
    p.cxx_realversion = abi::kDetectedCompilerRealVersion;
    // NB2-1: der Tag ist Pflicht, sobald der Dialekt gesetzt ist -- hier ein bewusst FREMDER Tag, damit
    // sichtbar bleibt, dass die Realversion NICHT aus ihm abgeleitet wird (G-C4).
    p.cxx_driver        = "hausgemachter-treiber";
    std::string const g = abi::render_toolchain_stamp_glied(p);
    std::string const erwartet = "cxx=" + std::string{abi::kDetectedCompilerDialect} + "-" +
                                 std::string{abi::kDetectedCompilerRealVersion} + ":hausgemachter-treiber@1.0.0c";
    EXPECT_NE(g.find(erwartet), std::string::npos) << "glied='" << g << "' erwartet-Segment='" << erwartet << "'";
    // Die Realversion bleibt der Versions-Traeger; der Tag steht hinter dem ':' und wird nie zur Version.
    EXPECT_NE(g.find("-" + std::string{abi::kDetectedCompilerRealVersion} + ":"), std::string::npos) << g;
}

// -- NB/CX-2: DER RENDERER IST INTERN INJEKTIV ---------------------------------------------------------
// Die drei Kollisionen sind KEINE Erfindung dieses Tests: sie stehen so im Codex-Nachreview [MAJOR] und
// waren am Vor-Stand real. Der Test friert sie als NEGATIV-Probe ein -- wer die Wache entschaerft, bekommt
// sie zurueck und sieht es hier sofort.
TEST(MW12StampBausteine, NbCx2ToolchainRendererIstInjektiv) {
    namespace abi = ::comdare::cache_engine::abi;

    // (1) ';'/'=' im Wert: {simd="avx2;ceb=8.0", ceb=""} rendert am Vor-Stand wie {simd="avx2", ceb="8.0"}.
    abi::ToolchainStampParts a{};
    a.simd = "avx2;ceb=8.0";
    abi::ToolchainStampParts b{};
    b.simd = "avx2";
    b.ceb  = "8.0";
    EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(a), std::string_view{"simd"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(a), std::invalid_argument);
    EXPECT_NO_THROW((void)abi::render_toolchain_stamp_glied(b));

    // (2) Die Flag-KLAMMER im Wert: {opt="O3{-funroll}"} vs. {opt="O3", opt_flags="-funroll"}.
    abi::ToolchainStampParts c{};
    c.opt = "O3{-funroll}";
    abi::ToolchainStampParts d{};
    d.opt       = "O3";
    d.opt_flags = "-funroll";
    EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(c), std::string_view{"opt"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(c), std::invalid_argument);
    EXPECT_NO_THROW((void)abi::render_toolchain_stamp_glied(d));

    // (3) Der DIALEKT-VERSION-Klebepunkt: {"gcc-13.2.0", ""} vs. {"gcc", "13.2.0"} -- beide ergaeben
    //     "cxx=gcc-13.2.0@1.0.0c". Deshalb traegt der Dialekt zusaetzlich kein '-'.
    abi::ToolchainStampParts e{};
    e.cxx_dialect = "gcc-13.2.0";
    e.cxx_driver  = "g++-13";
    abi::ToolchainStampParts f{};
    f.cxx_dialect     = "gcc";
    f.cxx_realversion = "13.2.0";
    f.cxx_driver      = "g++-13";
    EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(e), std::string_view{"cxx_dialect"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(e), std::invalid_argument);
    EXPECT_EQ(abi::render_toolchain_stamp_glied(f), std::string{"tc=1;cxx=gcc-13.2.0:g++-13@1.0.0c"});

    // (4) '@' und '\n' sind ebenso STRUKTUR bzw. Domain-Separator.
    abi::ToolchainStampParts g{};
    g.build_type = "Debug@2";
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(g), std::invalid_argument);
    abi::ToolchainStampParts h{};
    h.gate_contribution = "avx512\nfoo";
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(h), std::invalid_argument);

    // (5) Der gutartige Fall bleibt unberuehrt: alles leer => die IDENTITAET, kein Wurf.
    EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(abi::ToolchainStampParts{}), std::string_view{});
    EXPECT_EQ(abi::render_toolchain_stamp_glied(abi::ToolchainStampParts{}), std::string{});
}

// -- NB2-1: DAS cxx-FELD IST INJEKTIV JE TREIBER-TAG ---------------------------------------------------
// Codex-Zweitreview [KRITISCH], verbatim: "Tag ohne Endziffern oder Major-Mismatch laesst die Version ganz
// weg -> g++-17 und g++-18 kollabieren auf cxx=gcc ohne Version." Der Test friert die Heilung als
// POSITIV- UND NEGATIV-Probe ein: verschiedene Tags MUESSEN verschiedene Glieder ergeben, und zwar auch
// (gerade) dann, wenn ueber die Version nichts bekannt ist.
TEST(MW12StampBausteine, Nb21CxxFeldIstInjektivJeTreiberTag) {
    namespace abi = ::comdare::cache_engine::abi;

    auto glied_fuer = [](std::string_view treiber, std::string_view realversion) {
        abi::ToolchainStampParts p{};
        p.cxx_dialect     = "gcc";
        p.cxx_realversion = realversion;
        p.cxx_driver      = treiber;
        return abi::render_toolchain_stamp_glied(p);
    };

    // (1) DER BEFUND SELBST: zwei Treiber, ueber deren Version nichts gedeckt ist. Am Vor-Stand ergaben
    //     beide "tc=1;cxx=gcc@1.0.0c" -- also denselben Fingerprint fuer zwei verschiedene Compiler.
    std::string const g17 = glied_fuer("g++-17", {});
    std::string const g18 = glied_fuer("g++-18", {});
    EXPECT_NE(g17, g18) << "g17='" << g17 << "' g18='" << g18 << "'";
    EXPECT_EQ(g17, std::string{"tc=1;cxx=gcc:g++-17@1.0.0c"});
    EXPECT_EQ(g18, std::string{"tc=1;cxx=gcc:g++-18@1.0.0c"});

    // (2) Ein Tag OHNE Endziffern nennt keine Version -- er ist trotzdem unterscheidbar.
    EXPECT_NE(glied_fuer("/usr/bin/c++", {}), glied_fuer("g++", {}));
    EXPECT_NE(glied_fuer("/usr/bin/c++", {}), g17);

    // (3) Zwei Tags mit GLEICHEM Major kollabieren nicht mehr, obwohl beide dieselbe (gedeckte) Version
    //     tragen -- das war der zweite Ast des Befunds.
    EXPECT_NE(glied_fuer("g++-16", "16.2.0"), glied_fuer("x86_64-linux-gnu-g++-16", "16.2.0"));

    // (4) Die Klebepunkt-Regel ist FAIL-LOUD, nicht Konvention: ein ':' im Tag machte die Zerlegung
    //     mehrdeutig ("gcc-1:2" liesse sich als (gcc,1,2) oder (gcc,,1:2) lesen).
    abi::ToolchainStampParts k{};
    k.cxx_dialect = "gcc";
    k.cxx_driver  = "weird:tag";
    EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(k), std::string_view{"cxx_driver"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(k), std::invalid_argument);
    // Whitespace/Backslash/Anfuehrungszeichen wuerden das Define in der @rsp-Datei zerlegen.
    for (std::string_view const boese : {"g++ -16", "g++\\16", "g++\"16"}) {
        abi::ToolchainStampParts b{};
        b.cxx_dialect = "gcc";
        b.cxx_driver  = boese;
        EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(b), std::string_view{"cxx_driver"}) << boese;
        EXPECT_THROW((void)abi::render_toolchain_stamp_glied(b), std::invalid_argument) << boese;
    }
    // Und die realen Tag-Formen passieren -- die Wache ist scharf, aber nicht im Weg.
    EXPECT_NO_THROW((void)glied_fuer("/usr/bin/c++", {}));
    EXPECT_NO_THROW((void)glied_fuer("clang++-22", {}));

    // (5) Das Glied bleibt ein gueltiger Preimage-Glied-Wert (aeussere Wache aus anatomy_fingerprint.hpp).
    EXPECT_TRUE(abi::injizierter_glied_wert_ist_wohlgeformt(glied_fuer("/usr/bin/c++", {})));
    EXPECT_TRUE(abi::injizierter_glied_wert_ist_wohlgeformt(glied_fuer("g++-16", "16.2.0")));
}

// -- NB2-4: ABHAENGIGE FELDER WERDEN NICHT MEHR STILL VERWORFEN ----------------------------------------
// Codex-Zweitreview [MITTEL], verbatim: "abhaengige Felder still verworfen -- cxx_realversion ohne Dialekt,
// opt_flags ohne opt, atomic128_flags ohne atomic128 passieren die Diagnose, erscheinen aber nicht im
// Ergebnis." Der Test friert alle drei plus die NB2-1-Paarung als Negativ-Probe ein.
TEST(MW12StampBausteine, Nb24AbhaengigeFelderSindFailLoud) {
    namespace abi = ::comdare::cache_engine::abi;

    // (1) Realversion ohne Dialekt: am Vor-Stand lief der cxx-Zweig gar nicht erst an.
    abi::ToolchainStampParts a{};
    a.cxx_realversion = "16.2.0";
    EXPECT_EQ(abi::toolchain_stamp_parts_abhaengigkeits_diagnose(a), std::string_view{"cxx_realversion"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(a), std::invalid_argument);

    // (2) opt_flags ohne opt: toolchain_append_axis kehrte bei leerer id sofort um.
    abi::ToolchainStampParts b{};
    b.opt_flags = "-O3";
    EXPECT_EQ(abi::toolchain_stamp_parts_abhaengigkeits_diagnose(b), std::string_view{"opt_flags"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(b), std::invalid_argument);

    // (3) atomic128_flags ohne atomic128: derselbe Weg.
    abi::ToolchainStampParts c{};
    c.atomic128_flags = "-mcx16";
    EXPECT_EQ(abi::toolchain_stamp_parts_abhaengigkeits_diagnose(c), std::string_view{"atomic128_flags"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(c), std::invalid_argument);

    // (4) NB2-1: Dialekt und Treiber-Tag gehoeren zusammen -- in BEIDE Richtungen.
    abi::ToolchainStampParts d{};
    d.cxx_dialect = "gcc";
    EXPECT_EQ(abi::toolchain_stamp_parts_abhaengigkeits_diagnose(d), std::string_view{"cxx_dialect"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(d), std::invalid_argument);
    abi::ToolchainStampParts e{};
    e.cxx_driver = "g++-16";
    EXPECT_EQ(abi::toolchain_stamp_parts_abhaengigkeits_diagnose(e), std::string_view{"cxx_driver"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(e), std::invalid_argument);

    // (5) Die gutartigen Belegungen bleiben unberuehrt: LEER+LEER ist die IDENTITAET, und eine id OHNE
    //     Flags ist zulaessig (die Flags sind der optionale Teil, nicht die id).
    EXPECT_EQ(abi::toolchain_stamp_parts_abhaengigkeits_diagnose(abi::ToolchainStampParts{}), std::string_view{});
    abi::ToolchainStampParts f{};
    f.opt       = "O3";
    f.atomic128 = "cx16";
    EXPECT_EQ(abi::toolchain_stamp_parts_abhaengigkeits_diagnose(f), std::string_view{});
    EXPECT_NO_THROW((void)abi::render_toolchain_stamp_glied(f));
}

// -- NB2-3: DER bvset-NAMENS-ZEICHENVORRAT -------------------------------------------------------------
// Codex-Zweitreview [MITTEL], verbatim: "PT::name() wird unescaped zwischen Strukturzeichen eingesetzt,
// die Aussenwache erlaubt genau diese Zeichen -> ein Typ name='a;page_kind=1}{b' rendert identisch zu zwei
// Typen (a,1)+(b,2)." Die WACHE selbst ist compile-hart (static_assert je emittiertem Wrapper-Typ, s.
// build_variant_set_signature.hpp) -- ein Verstoss kann deshalb gar nicht bis in einen Testlauf kommen.
// Hier steht die Praedikat-Haelfte: dass die Wache genau die Kollisions-Zeichen faengt und die realen
// Registry-Namen passieren laesst (Digest-Neutralitaet).
TEST(MW12StampBausteine, Nb23BvsetNamensWacheFaengtStrukturzeichen) {
    namespace bx = ::comdare::cache_engine::builder::experiment;

    // (1) Der Codex-Name selbst -- die Kollision, um die es geht.
    EXPECT_FALSE(bx::variant_set_signature_name_ist_wohlgeformt("a;page_kind=1}{b"));
    // (2) Jedes Struktur-Zeichen einzeln.
    for (std::string_view const boese : {"a;b", "a=b", "a{b", "a}b", "a[b", "a]b", "a@b", "a\nb", "a\rb"})
        EXPECT_FALSE(bx::variant_set_signature_name_ist_wohlgeformt(boese)) << "name='" << boese << "'";
    // (3) LEER: `{;page_kind=1}` liesse die Grenze zwischen Name und erstem Feld offen.
    EXPECT_FALSE(bx::variant_set_signature_name_ist_wohlgeformt(""));
    // (4) Die realen Namensformen der drei Build-Achsen passieren -- die Wache ist DIGEST-NEUTRAL.
    for (std::string_view const gut : {"bplus", "avx512", "x86_64", "no_extension", "general_x86_64"})
        EXPECT_TRUE(bx::variant_set_signature_name_ist_wohlgeformt(gut)) << "name='" << gut << "'";
    // (5) Und die LEBENDE Signatur dieses Treibers ist ein gueltiger Preimage-Glied-Wert -- der Beweis,
    //     dass die Bestands-Namen die Wache real bestehen (sonst haette der Bau schon gebrochen).
    EXPECT_TRUE(::comdare::cache_engine::abi::injizierter_glied_wert_ist_wohlgeformt(
        bx::kDriverBuildVariantSignature));
}

namespace {
// NB-3/T2-D: SYNTHETISCHE Registry-Wrapper fuer die Negativ-Probe der Paar-Wache. Sie muessen synthetisch
// sein: die realen Registries sind (per compile-harter Wache) eindeutig, an ihnen liesse sich der
// Kollisions-Fall gar nicht mehr herstellen. Nur name() wird gebraucht -- die Paar-Wache liest sonst
// nichts vom Typ.
struct T2dDupA {
    static constexpr std::string_view name() { return "dup"; }
};
struct T2dDupB {
    static constexpr std::string_view name() { return "dup"; }
};
struct T2dUniq {
    static constexpr std::string_view name() { return "uniq"; }
};
} // namespace

// -- NB-3/T2-D (1): DIE PAARWEISE NAMENS-EINDEUTIGKEIT DES bvset-GLIEDS -------------------------------
// NB2-3 hat geprueft, dass ein Name die Signatur nicht ZERLEGT. Die andere Haelfte derselben Zusage --
// dass der Name ueberhaupt DISKRIMINIERT -- war offen: zwei verschiedene Wrapper mit gleichem Namen und
// gleichen Feldern lassen {A} und {B} byte-identisch rendern. Seit Format 3 ist das ein gleicher
// Fingerprint fuer zwei verschieden gebaute Treiber (falscher Skip). Die Wache selbst ist compile-hart
// (static_assert je emittierter Liste UND je voller Registry-Liste); hier steht das Praedikat.
TEST(MW12StampBausteine, Nb3T2dBvsetNamenSindPaarweiseEindeutig) {
    namespace bx = ::comdare::cache_engine::builder::experiment;
    namespace mp = ::boost::mp11;

    // (1) Der Kollisions-Fall, um den es geht: zwei VERSCHIEDENE Typen, EIN Name.
    EXPECT_FALSE((bx::variant_set_signature_namen_paarweise_eindeutig<mp::mp_list<T2dDupA, T2dDupB>>()));
    // (2) Er faellt auch auf, wenn er in einer laengeren Liste versteckt sitzt (die Pruefung ist paarweise,
    //     nicht bloss "Nachbarn vergleichen").
    EXPECT_FALSE((bx::variant_set_signature_namen_paarweise_eindeutig<mp::mp_list<T2dDupA, T2dUniq, T2dDupB>>()));
    // (3) Verschiedene Namen passieren -- die Wache ist DIGEST-NEUTRAL.
    EXPECT_TRUE((bx::variant_set_signature_namen_paarweise_eindeutig<mp::mp_list<T2dDupA, T2dUniq>>()));
    // (4) Die Rand-Kardinalitaeten: leer und einelementig sind trivial eindeutig (kein Paar existiert).
    EXPECT_TRUE((bx::variant_set_signature_namen_paarweise_eindeutig<mp::mp_list<>>()));
    EXPECT_TRUE((bx::variant_set_signature_namen_paarweise_eindeutig<mp::mp_list<T2dUniq>>()));
    // (5) Die REALEN Registries dieses Treibers -- volle Listen, nicht bloss die enabled Menge. Diese
    //     Zeilen sind zugleich der Beweis, dass die compile-harte Wache in
    //     driver_build_variant_signature.hpp nicht auf einer leeren Menge trivial gruen ist.
    EXPECT_TRUE((bx::variant_set_signature_namen_paarweise_eindeutig<
                 ::comdare::cache_engine::nodes::axis_01_page_type::AllPageTypes>()));
    EXPECT_TRUE((bx::variant_set_signature_namen_paarweise_eindeutig<
                 ::comdare::cache_engine::hardware::axis_09b_simd_extension::AllExtensions>()));
    EXPECT_TRUE((bx::variant_set_signature_namen_paarweise_eindeutig<
                 ::comdare::cache_engine::hardware::axis_12_general_hardware::AllPlatforms>()));
    EXPECT_GT((mp::mp_size<::comdare::cache_engine::nodes::axis_01_page_type::AllPageTypes>::value), 1u);
}

// -- NB-3/T2-D (2): DER TRANSPORT-ZEICHENVORRAT GILT FUER JEDES FELD ----------------------------------
// Vor T2-D garantierte die Wache die @rsp-Transportfaehigkeit nur fuer den TREIBER-TAG. Mit den
// per-Perm-Feldern (opt_flags aus der optxsimd-Schleife) wird ein mehrteiliger Flag-String realistisch --
// und der haette das Define-Argument in der Response-Datei in ZWEI Optionen zerlegt.
TEST(MW12StampBausteine, Nb3T2dTransportZeichenvorratGiltFuerJedesFeld) {
    namespace abi = ::comdare::cache_engine::abi;

    // (1) Das Praedikat selbst: jedes Transport-Zeichen einzeln, inkl. der unsichtbaren.
    for (std::string_view const boese : {"a b", "a\tb", "a\vb", "a\fb", "a\\b", "a\"b", "a'b"})
        EXPECT_FALSE(abi::toolchain_wert_ist_rsp_transportfaehig(boese)) << "wert='" << boese << "'";
    for (std::string_view const gut : {"-O3", "-mcx16", "-march=x86-64-v3", "g++-16", "8.0"})
        EXPECT_TRUE(abi::toolchain_wert_ist_rsp_transportfaehig(gut)) << "wert='" << gut << "'";

    // (2) Die Verengung wirkt auf ALLE Felder, nicht nur auf den Tag -- der Renderer nennt das Feld.
    abi::ToolchainStampParts p{};
    p.cxx_dialect = "gcc";
    p.cxx_driver  = "g++-16";
    p.opt         = "O3";
    p.opt_flags   = "-O3 -funroll-loops"; // genau der Fall, den die Teil-2-Welle heraufbeschwoert
    EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(p), std::string_view{"opt_flags"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(p), std::invalid_argument);

    // (3) Whitespace-frei gerendert passiert derselbe Inhalt -- die gewollte Ausweichform.
    p.opt_flags = "-O3,-funroll-loops";
    EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(p), std::string_view{});
    EXPECT_NO_THROW((void)abi::render_toolchain_stamp_glied(p));

    // (4) DIGEST-NEUTRALITAET: die heutige einteilige Flag-Form bleibt unveraendert zulaessig.
    p.opt_flags = "-O3";
    EXPECT_NE(abi::render_toolchain_stamp_glied(p).find("opt=O3{-O3}"), std::string::npos);
}

// -- NB-3/T2-D (3): DIE TRANSPORT-NAHT PRUEFT, STATT ZU BEHAUPTEN -------------------------------------
// Der Bau-Kanal (perm_stamp_glied_defines) reicht den frisch GERENDERTEN String direkt an die
// Define-Naht -- er laeuft also gar nicht durch einen Traeger. Auf genau diesem Weg war die zugesagte
// Transport-Eigenschaft unbewiesen.
TEST(MW12StampBausteine, Nb3T2dDefineNahtIstFailLoud) {
    namespace pfn = ::comdare::cache_engine::profile_facade;
    namespace bx  = ::comdare::cache_engine::builder::experiment;

    // (1) LEER bleibt LEER -- kein Define, also die Identitaet (unveraendert).
    EXPECT_EQ(pfn::toolchain_stamp_glied_define_arg(""), std::string{});
    EXPECT_EQ(pfn::build_variant_set_signature_define_arg(""), std::string{});

    // (2) Ein Wert mit Whitespace wird ABGELEHNT statt still zerlegt zu werden -- beide Nahtstellen.
    EXPECT_THROW((void)pfn::toolchain_stamp_glied_define_arg("tc=1;opt=O3{-O3 -x}"), std::invalid_argument);
    EXPECT_THROW((void)pfn::build_variant_set_signature_define_arg("bvset=1;page_type[{a b}]"),
                 std::invalid_argument);

    // (3) Die LEBENDEN Werte passieren -- die Naht ist digest-neutral fuer den realen Bau.
    EXPECT_NO_THROW((void)pfn::toolchain_stamp_glied_define_arg(pfn::compose_live_toolchain_stamp_glied()));
    EXPECT_NO_THROW((void)pfn::build_variant_set_signature_define_arg(bx::kDriverBuildVariantSignature));
}

// -- NB-3/T2-D (4): TRAEGER-LEBENSDAUER UND VOLL-WACHE BEIM GEBRAUCH ----------------------------------
// Die Traeger halten eine SICHT. Die Konstruktor-Wache beweist etwas ueber den Wert ZUM ZEITPUNKT DER
// KONSTRUKTION -- gehasht wird er spaeter. Beide Loecher werden getrennt geschlossen.
TEST(MW12StampBausteine, Nb3T2dTraegerLebensdauerUndVollWache) {
    namespace abi = ::comdare::cache_engine::abi;

    // (a) DANGLING: ein Temporary kann sich nicht mehr an einen Traeger binden (compile-hart). Lvalues,
    //     also der Normalfall einer benannten Variable, die den Aufruf ueberlebt, bleiben zulaessig.
    static_assert(!std::is_constructible_v<abi::ToolchainGlied, std::string&&>);
    static_assert(!std::is_constructible_v<abi::BvsetGlied, std::string&&>);
    static_assert(!std::is_constructible_v<abi::OverlayHash, std::string&&>);
    static_assert(std::is_constructible_v<abi::ToolchainGlied, std::string&>);
    static_assert(std::is_constructible_v<abi::ToolchainGlied, std::string_view>);

    // (b) SPAETE MUTATION: der Puffer HINTER der Sicht aendert sich nach der Pruefung. Die Voll-Wache in
    //     anatomy_fingerprint_glieder() steht unmittelbar vor dem Preimage -- zwischen ihr und dem Hash
    //     liegt kein Aufrufer mehr.
    std::string wert = "tc=1";
    abi::ToolchainGlied const tc{wert}; // Konstruktor-Wache: wohlgeformt
    EXPECT_NO_THROW((void)abi::anatomy_fingerprint_glieder("O", "S", "M", tc));
    wert[2] = '\n'; // der Domain-Separator, nachtraeglich eingeschleust
    EXPECT_THROW((void)abi::anatomy_fingerprint_glieder("O", "S", "M", tc), std::invalid_argument);
}

// -- NB-3/T2-D (5): DIE FELD-ORDNUNG VON ToolchainStampParts IST BEWIESEN -----------------------------
// NB2-1 hatte cxx_driver als DRITTES Feld EINGESCHOBEN. Ein Aggregat ist positionell initialisierbar --
// ein Bestands-Aufruf haette danach still um eins verschoben belegt. Die Ordnungs-Wache steht als
// static_assert am Struct; hier steht die lesbare Haelfte: cxx_driver ist das LETZTE Feld.
TEST(MW12StampBausteine, Nb3T2dFeldOrdnungCxxDriverStehtAmEnde) {
    namespace abi = ::comdare::cache_engine::abi;

    // Positionelle Belegung der ersten vier Felder -- die Ordnung, nach der Bestands-Aufrufer lesen.
    abi::ToolchainStampParts const p{"gcc", "15.3.0", "O3", "-O3"};
    EXPECT_EQ(p.cxx_dialect, std::string_view{"gcc"});
    EXPECT_EQ(p.cxx_realversion, std::string_view{"15.3.0"});
    EXPECT_EQ(p.opt, std::string_view{"O3"});
    EXPECT_EQ(p.opt_flags, std::string_view{"-O3"});
    EXPECT_TRUE(p.cxx_driver.empty()) << "cxx_driver muss das LETZTE Feld sein -- neue Felder werden "
                                         "ANGEHAENGT, nie eingeschoben.";
    // Und die Abhaengigkeits-Diagnose sieht genau das: ein Dialekt ohne Treiber-Tag ist unvollstaendig.
    EXPECT_EQ(abi::toolchain_stamp_parts_abhaengigkeits_diagnose(p), std::string_view{"cxx_dialect"});
}

// -- NB/CX-1: DIE RT-INJEKTIVITAETS-WACHE DER INJIZIERTEN GLIEDER --------------------------------------
// Der Codex-Blocker war konkret: A={Toolchain="TC\nX", bvset="BV"} und B={Toolchain="TC", bvset="X\nBV"}
// erzeugen BYTE-IDENTISCHE Preimages. Der Test beweist, dass beide Belegungen jetzt gar nicht mehr
// entstehen koennen -- die Wache sitzt im Traeger-Typ, also auf JEDEM Weg ins Preimage.
TEST(MW12StampBausteine, NbCx1RtInjektivitaetsWacheIstFailLoud) {
    namespace abi = ::comdare::cache_engine::abi;

    // (1) Der Domain-Separator in einem injizierten Wert -- beide Haelften der Codex-Kollision.
    EXPECT_THROW((void)abi::ToolchainGlied{std::string_view{"TC\nX"}}, std::invalid_argument);
    EXPECT_THROW((void)abi::BvsetGlied{std::string_view{"X\nBV"}}, std::invalid_argument);
    EXPECT_THROW((void)abi::OverlayHash{std::string_view{"a\nb"}}, std::invalid_argument);

    // (2) Leerer SCHLUESSEL -- am Anfang und im Segment. Ohne diese Regel liesse sich die Grenze zwischen
    //     Schluessel und Wert verschieben, ohne dass sich ein Byte aendert.
    EXPECT_THROW((void)abi::ToolchainGlied{std::string_view{"=1;cxx=gcc"}}, std::invalid_argument);
    EXPECT_THROW((void)abi::BvsetGlied{std::string_view{"bvset=1;=x"}}, std::invalid_argument);
    EXPECT_THROW((void)abi::BvsetGlied{std::string_view{"bvset=1;page_type[=x]"}}, std::invalid_argument);

    // (3) ZEICHENVORRAT -- Whitespace und Anfuehrungszeichen sind das typische Zeichen dafuer, dass ein
    //     fremder String (Pfad, Fehlertext) in den Slot geraten ist.
    EXPECT_THROW((void)abi::ToolchainGlied{std::string_view{"tc=1; cxx=gcc"}}, std::invalid_argument);
    EXPECT_THROW((void)abi::ToolchainGlied{std::string_view{"tc=\"1\""}}, std::invalid_argument);

    // (4) Die REALEN Werte passieren -- die Wache ist scharf, aber nicht im Weg.
    EXPECT_NO_THROW((void)abi::ToolchainGlied{std::string_view{"tc=1;cxx=gcc-16.0.1@1.0.0c;opt=O3{-O3}@1.0.0c"}});
    EXPECT_NO_THROW(
        (void)abi::BvsetGlied{std::string_view{"bvset=1;bv=2;page_type[{bplus;hw_cache_line=64}];simd_extension[]"}});
    EXPECT_NO_THROW((void)abi::ToolchainGlied{std::string_view{}});
    EXPECT_TRUE(abi::injizierter_glied_wert_ist_wohlgeformt(abi::kToolchainStampGlied));
    EXPECT_TRUE(abi::injizierter_glied_wert_ist_wohlgeformt(abi::kBuildVariantSetSignatureGlied));

    // (5) Die LAUFZEIT-Preimage-Bildung faengt den Separator auch in den Stempel-ZEILEN [1]-[3], die nicht
    //     durch einen Traeger-Typ laufen. Ohne diesen Zweig bliebe genau dort eine Luecke.
    std::array<std::string_view, abi::kAnatomyFingerprintGliedCount> const kaputt{
        abi::kAnatomyFingerprintFormat, "organ\nzeile", "", "", "", "", "", ""};
    EXPECT_THROW((void)abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{kaputt.data(),
                                                                                           kaputt.size()}),
                 std::invalid_argument);
    auto const heil = abi::anatomy_fingerprint_glieder("ORGAN", "SYSTEM", "MESS");
    EXPECT_NO_THROW(
        (void)abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{heil.data(), heil.size()}));
}

// -- NB/CX-4: DIE WIRKSAMKEITS-PROBE DER LIVE-NAHT -----------------------------------------------------
// Die Frozen-Vektoren oben rechnen ueber LITERALE (sie muessen maschinen-unabhaengig bleiben). Diese Probe
// prueft die andere Haelfte: dass die LIVE komponierten Glieder [5]/[6] wirklich Werte tragen und wirklich
// im Preimage landen. Ohne sie waere die Zusage "der produktive Fingerprint traegt die Glieder LIVE" genau
// die Sorte Behauptung, die das ganze Nachbesserungs-Fenster beseitigen soll.
TEST(MW12StampBausteine, NbCx4LiveGliederStehenImPreimage) {
    namespace abi = ::comdare::cache_engine::abi;
    namespace pf  = ::comdare::cache_engine::profile_facade;

    // (1) Das LIVE-Toolchain-Glied ist NICHT leer und traegt den Kopf + das cxx-Feld.
    std::string const tc = pf::compose_live_toolchain_stamp_glied();
    ASSERT_FALSE(tc.empty()) << "das Toolchain-Glied [5] ist live LEER -- die Naht ist wirkungslos";
    EXPECT_TRUE(tc.starts_with("tc=")) << "glied='" << tc << "'";
    EXPECT_NE(tc.find("cxx="), std::string::npos) << "glied='" << tc << "'";
    EXPECT_NE(tc.find(";ceb="), std::string::npos) << "der Contract-Wert gehoert ins Glied (G-C2, Fall C)";

    // (2) Das LIVE-bvset-Glied ist die Treiber-Signatur -- unveraendert durchgereicht, nichts abgeleitet.
    std::string const bv = pf::live_build_variant_set_signature_glied();
    ASSERT_FALSE(bv.empty());
    EXPECT_TRUE(bv.starts_with("bvset=")) << "glied='" << bv << "'";
    EXPECT_EQ(bv, std::string{::comdare::cache_engine::builder::experiment::kDriverBuildVariantSignature});

    // (3) BEIDE stehen LITERAL im Preimage, an ihren benannten Positionen.
    auto const glieder = abi::anatomy_fingerprint_glieder("ORGAN", "SYSTEM", "MESS", abi::ToolchainGlied{tc},
                                                          abi::BvsetGlied{bv});
    EXPECT_EQ(glieder[abi::kAnatomyFingerprintToolchainGlied], tc);
    EXPECT_EQ(glieder[abi::kAnatomyFingerprintBvsetGlied], bv);
    std::string const preimage =
        abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{glieder.data(), glieder.size()});
    EXPECT_NE(preimage.find("cxx="), std::string::npos) << "preimage='" << preimage << "'";
    EXPECT_NE(preimage.find("bvset="), std::string::npos) << "preimage='" << preimage << "'";

    // (4) WIRKSAMKEIT: mit den Live-Gliedern ergibt sich ein ANDERER Digest als mit den leeren Defaults.
    //     Das ist die eigentliche Aussage -- vorher waren beide Wege byte-gleich.
    auto const leer = abi::anatomy_fingerprint_glieder("ORGAN", "SYSTEM", "MESS");
    EXPECT_NE(abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{leer.data(), leer.size()}),
              preimage);

    // (5) DIE DRIFT-FREIHEIT DER NAHT: derselbe argumentlose Aufruf liefert denselben String. Genau darauf
    //     ruht, dass Bau-Kanal (-D an perm_compile) und Laufzeit-Zwilling ueber DASSELBE Glied rechnen.
    EXPECT_EQ(pf::compose_live_toolchain_stamp_glied(), tc);
    EXPECT_EQ(pf::live_build_variant_set_signature_glied(), bv);

    // (6) Die Define-ARGUMENTE tragen die Wertform als C-String-Literal (rsp-Escaping wie die Zellwert-Naht)
    //     und enthalten keinen Whitespace -- sonst zerfielen sie in der Response-Datei in mehrere Optionen.
    std::string const arg_tc = pf::toolchain_stamp_glied_define_arg(tc);
    std::string const arg_bv = pf::build_variant_set_signature_define_arg(bv);
    EXPECT_TRUE(arg_tc.starts_with("-DCOMDARE_TOOLCHAIN_STAMP_GLIED=")) << arg_tc;
    EXPECT_TRUE(arg_bv.starts_with("-DCOMDARE_BUILD_VARIANT_SET_SIGNATURE=")) << arg_bv;
    EXPECT_EQ(arg_tc.find(' '), std::string::npos) << arg_tc;
    EXPECT_EQ(arg_bv.find(' '), std::string::npos) << arg_bv;
    EXPECT_EQ(pf::toolchain_stamp_glied_define_arg(""), std::string{}) << "leer => kein Define => Identitaet";

    // (7) DIE EHRLICHKEITS-WACHE der Realversion. NB2-1 hat sie von fail-open auf fail-closed gezogen:
    //     gedeckt ist die CEB-Realversion nur, wenn der Treiber-Tag sie VOLLSTAENDIG selbst nennt --
    //     ein Tag, der nur den MAJOR nennt, ist mit jeder x.y vertraeglich und pinnt gar nichts.
    EXPECT_FALSE(pf::ct_realversion_deckt_treiber("/usr/bin/c++")) << "ein Tag ohne Endziffern nennt keine Version";
    EXPECT_FALSE(pf::ct_realversion_deckt_treiber("g++")) << "dito";
    std::string const ct_voll{abi::kDetectedCompilerRealVersion};
    std::string const ct_major{abi::kDetectedCompilerRealVersion.substr(0, abi::kDetectedCompilerRealVersion.find('.'))};
    std::string const praefix = (abi::kDetectedCompilerDialect == std::string_view{"clang"} ? "clang++-" : "g++-");
    EXPECT_TRUE(pf::ct_realversion_deckt_treiber(praefix + ct_voll)) << "treiber='" << praefix + ct_voll << "'";
    // DER CODEX-FALL, jetzt Negativ-Probe: "g++-16" deckt 16.1.0 und 16.3.0 gleichermassen -- also keines.
    EXPECT_FALSE(pf::ct_realversion_deckt_treiber(praefix + ct_major))
        << "ein Tag, der nur den MAJOR nennt, darf die CEB-Version NICHT decken: " << praefix + ct_major;
    EXPECT_FALSE(pf::ct_realversion_deckt_treiber(praefix + ct_voll + "0")) << "andere Version => KEINE Deckung";
    // Die VOLLE Kennung wird gelesen, nicht bloss die letzte Ziffernfolge (sonst waere "g++-16.1" == "1").
    EXPECT_EQ(pf::cxx_driver_version_kennung("g++-16.1"), std::string_view{"16.1"});
    EXPECT_EQ(pf::cxx_driver_version_kennung("g++-16"), std::string_view{"16"});
    EXPECT_EQ(pf::cxx_driver_version_kennung("/usr/bin/c++"), std::string_view{});
    // Ist die Deckung da, steht die Realversion im Glied; ist sie es nicht, steht sie NICHT drin.
    if (pf::ct_realversion_deckt_treiber(pf::active_cxx_driver_tag()))
        EXPECT_NE(tc.find("-" + ct_voll + ":"), std::string::npos) << tc;
    else
        EXPECT_EQ(tc.find("-" + ct_voll + ":"), std::string::npos)
            << "eine UNGEDECKTE Realversion darf nicht im Glied stehen: " << tc;

    // (8) NB2-1 (R1): der TIER-TREIBER-TAG steht IMMER im Glied -- er ist der Diskriminator, der
    //     verhindert, dass zwei verschiedene Treiber dasselbe Glied ergeben. Er ist KEINE Ableitung: es
    //     ist byte-genau derselbe Tag, den active_cxx_driver_tag() liefert und unter dem gebaut wird.
    std::string const treiber = pf::active_cxx_driver_tag();
    ASSERT_FALSE(treiber.empty());
    EXPECT_NE(tc.find(":" + treiber + "@"), std::string::npos)
        << "das cxx-Feld muss den Tier-Treiber-Tag tragen: tc='" << tc << "' treiber='" << treiber << "'";
}

// -- NB2-2: EINE PREIMAGE-QUELLE FUER BEIDE WEGE -------------------------------------------------------
// Codex-Zweitreview [KRITISCH], verbatim: "consteval anatomy_fingerprint_hex baut das Preimage in eigener
// ungeprueft er Schleife (nicht ueber anatomy_fingerprint_preimage) -> organ='A\nB',system='C' kollidiert
// byte-identisch mit organ='A',system='B\nC'". Beide Belegungen sind ab jetzt UNBAUBAR (der consteval-Weg
// laeuft durch dieselbe Wache wie der Laufzeit-Weg; ein Wurf ist kein konstanter Ausdruck) -- ein
// Positiv-Test dafuer kann es per Konstruktion nicht geben. Was hier steht, ist die andere Haelfte: der
// BEWEIS, dass beide Wege dasselbe Preimage bilden, und die Laufzeit-Haelfte der Wache.
TEST(MW12StampBausteine, Nb22ConstevalUndLaufzeitTeilenEinePreimageQuelle) {
    namespace abi = ::comdare::cache_engine::abi;
    namespace s5  = ::comdare::cache_engine::sha512;

    constexpr std::string_view kOrgan   = "search_algo=k_ary@1.0.0c";
    constexpr std::string_view kSystem  = "target_isa=code@1.0.0c";
    constexpr std::string_view kMeasure = "measurement_tooling=wallclock@1.0.0c";

    // (1) consteval-Hex == SHA-512 ueber das LAUFZEIT-gebildete Preimage derselben Glieder. Vor NB2-2
    //     waren das zwei getrennte Schleifen, die nur per Sichtnaehe uebereinstimmten.
    constexpr auto    fp      = abi::anatomy_fingerprint_hex(kOrgan, kSystem, kMeasure);
    auto const        glieder = abi::anatomy_fingerprint_glieder(kOrgan, kSystem, kMeasure);
    std::string const pre =
        abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{glieder.data(), glieder.size()});
    auto const rt = s5::to_hex(
        s5::sha512(std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(pre.data()), pre.size()}));
    EXPECT_EQ(std::string_view(fp.data(), 128), std::string_view(rt.data(), rt.size()));

    // (2) DIE KOLLISION, um die es geht: das GLEICHE Zeichenmaterial, eine verschobene Feldgrenze. Beide
    //     Belegungen tragen den Separator und werden jetzt auf BEIDEN Wegen abgelehnt -- hier literal auf
    //     dem Laufzeit-Weg (der consteval-Weg lehnt sie compile-hart ab, s. Kopf).
    std::array<std::string_view, abi::kAnatomyFingerprintGliedCount> const a{
        abi::kAnatomyFingerprintFormat, "A\nB", "C", "", "", "", "", ""};
    std::array<std::string_view, abi::kAnatomyFingerprintGliedCount> const b{
        abi::kAnatomyFingerprintFormat, "A", "B\nC", "", "", "", "", ""};
    EXPECT_THROW((void)abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{a.data(), a.size()}),
                 std::invalid_argument);
    EXPECT_THROW((void)abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{b.data(), b.size()}),
                 std::invalid_argument);

    // (3) DIE KAPSELUNG der Traeger: der geprueffte Wert kann nach der Konstruktion nicht mehr ersetzt
    //     werden. Das ist keine Stil-Frage -- am Vor-Stand war `.value` ein public Feld und die
    //     Konstruktor-Wache damit umgehbar (`ToolchainGlied t{"ok"}; t.value = "TC\nX";`). Den BEWEIS
    //     traegt der Compiler: die Zeile oben ist heute schlicht nicht mehr uebersetzbar, es gibt kein
    //     zuweisbares Feld. Hier steht die Lese-Haelfte -- der Wert kommt unveraendert wieder heraus.
    constexpr std::string_view kTc = "tc=1;cxx=gcc:g++-16@1.0.0c";
    EXPECT_EQ(abi::ToolchainGlied{kTc}.wert(), kTc);
    EXPECT_EQ(abi::BvsetGlied{std::string_view{"bvset=1"}}.wert(), std::string_view{"bvset=1"});
    EXPECT_EQ(abi::OverlayHash{std::string_view{}}.wert(), std::string_view{});
}
