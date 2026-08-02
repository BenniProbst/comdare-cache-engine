// Querschnitt M -- W12-A (Section43): X.Y.Z-Parse-Helfer (Inkrement 2) + Planer-Selbst-Stempel (Inkrement 3).
// Leichte TU (keine Registries): verifiziert die isolierten Stempel-Bausteine + ihre Byte-Trennung zur .algos-Welt.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // W12-A3: AnatomyVersionLines-POD-Layout-Wache
#include <cache_engine/abi/anatomy_fingerprint.hpp>   // K7b-3: anatomy_fingerprint_hex (SHA-512 der 4 Stempel-Zeilen)
#include <cache_engine/abi/anatomy_stamp_entries.hpp> // A3: count/parse_stamp_entries + AnatomyStampEntryV1
#include <cache_engine/abi/anatomy_version_stamp.hpp>
#include <cache_engine/abi/system_axis_code_versions.hpp>            // A2: kSystemAxisCodeVersions (Single-Source)
#include <cache_engine/measurement/measurement_tooling_registry.hpp> // A2: version-Feld + tooling_version_for_id
#include <sha512/ctsha512.hpp> // K7b-3: Referenz-SHA-512 fuer den Fingerprint-Korrektheitstest
#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/axis_version_stamp.hpp>
#include "builder/ceb_version_stamp.hpp" // A5: CEB-Selbst-Stempel (consteval Mess-Array + SHA-512)
#include <profile_facade/planner/planner_version.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint> // A13-M1: uint32_t/uint64_t explizit (reserved-Flag-Wachen)
#include <span>
#include <string>
#include <string_view>
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
// Ersetzt die A13-M1-Erwartungen ("vNe"-Kurzform). Hier: Uebergangs-Toleranz, volle Flag-Familie, die
// FIXE Reihenfolge (HW-Flag, dann 'e') und die Review-Auflage K-5 (Sentinel-Batterie mit Flags).
TEST(MW12StampBausteine, A13M1bFlagGrammarParsesInRawAndDottedForm) {
    // (a) UEBERGANGS-TOLERANZ: der flaglose dreistellige Bestand parst weiter und traegt KEIN Flag.
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
    //     nur "...c"/"...ce" erfuellen den CPU-only-Scope. Der HEUTIGE Bestand erfuellt sie bewusst NICHT --
    //     genau deshalb steht das Define auf OFF, bis der M2/M3-Migrations-Commit die Literale zieht.
    EXPECT_TRUE(m::version_satisfies_cpu_only_policy("v1.0.0c"));
    EXPECT_TRUE(m::version_satisfies_cpu_only_policy("v2.3.4ce"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1.0.0")); // der heutige 122x-Bestand
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1.0.0g"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1.0.0f"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1.0.0n"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v0.0.0c")); // Sentinel erfuellt nie eine Politik
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1c"));     // Kurzform erfuellt nie eine Politik
    // Der Bestand ist heute (noch) flaglos -- das ist der dokumentierte, befristete Zustand:
    for (auto const& t : m::kMeasurementToolingRegistry)
        EXPECT_FALSE(m::version_satisfies_cpu_only_policy(t.version))
            << "Bestands-Version '" << t.version << "' traegt schon ein 'c'? Dann ist die M2/M3-Migration "
            << "gelaufen und COMDARE_VERSION_HW_FLAG_ENFORCE gehoert auf ON.";
}

TEST(MW12StampBausteine, A13M1bFlagsRenderAndStayGoldenNeutral) {
    // Render: erst das Hardware-Flag, dann das 'e'; der Sentinel rendert IMMER nackt "0.0.0".
    EXPECT_EQ(m::algo_semver_string("v1.0.0c"), std::string{"1.0.0c"});
    EXPECT_EQ(m::algo_semver_string("v2.3.4ce"), std::string{"2.3.4ce"});
    EXPECT_EQ(m::algo_semver_string("v2.3.4g"), std::string{"2.3.4g"});
    EXPECT_EQ(m::algo_semver_string("v2.3.4ne"), std::string{"2.3.4ne"});
    EXPECT_EQ(m::algo_semver_string("v0.0.0c"), std::string{"0.0.0"});
    EXPECT_EQ(m::algo_semver_string("v0.0.0ce"), std::string{"0.0.0"});
    // GOLDEN-NEUTRALITAET: der flaglose Bestand ("v1.0.0"/"v2.3.4") rendert byte-identisch wie vor A13-M1b.
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
    std::string const line = ::comdare::cache_engine::abi::system_stamp_line();
    EXPECT_EQ(std::count(line.begin(), line.end(), ';'), 2); // 3 System-Haupt-Achsen -> 2 Trenner
    EXPECT_EQ(line.rfind("target_isa=code@1.0.0", 0), 0u);
    EXPECT_NE(line.find(";operating_system=code@1.0.0"), std::string::npos);
    EXPECT_NE(line.find(";external_utils=code@1.0.0"), std::string::npos);
    // Die drei abgewanderten Namen duerfen hier NICHT mehr auftauchen -- sonst waere der Umbau nur halb.
    EXPECT_EQ(line.find("compiler="), std::string::npos);
    EXPECT_EQ(line.find("scheduling="), std::string::npos);
    EXPECT_EQ(line.find("extension_hardware="), std::string::npos);
    EXPECT_EQ(line.find("load_framework="), std::string::npos);
    EXPECT_EQ(line.find("@v1"), std::string::npos); // separate Welt zur .algos-Sig
}

TEST(MW12StampBausteine, MeasurementStampLineCarriesLoadFrameworkThenToolingMain) {
    // W12-A3 (Section 43, Section 47): der Mess-Stempel traegt die gewaehlte Mess-Tooling-HAUPT-Wahl als
    // Eintrag "measurement_tooling=<tooling>@X.Y.Z" -- Voll-Form, SEPARATE Welt zur .algos-Sig.
    // O-8 Schritt 9 (K1-Anschluss, Scharfschaltung): DAVOR steht jetzt IMMER das load_framework-Segment.
    // load_framework war bis Schritt 4 eine System-Haupt-Achse; mit dem Umzug in den Mess-Realm ist es das
    // ERSTE Segment dieser Zeile geworden. Alle Erwartungen unten sind aus dem Werkzeug-Output des
    // Fenster-Laufs geankert (Ist "load_framework=ycsb@1.0.0;measurement_tooling=wallclock@1.0.0").
    std::string const line = ::comdare::cache_engine::abi::measurement_stamp_line("wallclock");
    EXPECT_EQ(line, std::string{"load_framework=ycsb@1.0.0;measurement_tooling=wallclock@1.0.0"});
    // load_framework + EINE Tooling-Haupt-Achse -> genau EIN ';'-Trenner (Ablaufmethodik/Workloads sind
    // UNTER-Achsen -> nie Bestandteil; die Zahl steigt also von 0 auf 1, nicht weiter).
    EXPECT_EQ(std::count(line.begin(), line.end(), ';'), 1);
    EXPECT_EQ(line.rfind("load_framework=ycsb@1.0.0", 0), 0u); // ERSTES Segment, nicht irgendwo
    EXPECT_EQ(line.find("@v1"), std::string::npos);            // separate Welt zur .algos-Sig (X.Y.Z, nicht roh)
    // Andere Tooling-Haupt-Wahlen materialisieren analog -- das load_framework-Segment bleibt konstant davor.
    EXPECT_EQ(::comdare::cache_engine::abi::measurement_stamp_line("macro"),
              std::string{"load_framework=ycsb@1.0.0;measurement_tooling=macro@1.0.0"});
    EXPECT_EQ(::comdare::cache_engine::abi::measurement_stamp_line("micro"),
              std::string{"load_framework=ycsb@1.0.0;measurement_tooling=micro@1.0.0"});
    // Leere Wahl -> leere Zeile (ehrlich: kein Mess-Tooling einkompiliert).
    EXPECT_TRUE(::comdare::cache_engine::abi::measurement_stamp_line("").empty());
}

TEST(MW12StampBausteine, MeasurementStampLineSetFormCarriesToolingMenge) {
    // K7b-2 (Section 64-D1-B, 2026-07-22): die MENGEN-Form -- N Tools -> N ';'-getrennte
    // measurement_tooling=<t>@1.0.0-Eintraege (Eingabe-Reihenfolge, Section-64-Vollmengen-Provenienz). Additive
    // span-Ueberladung; die Einzel-Form oben bleibt unveraendert (der [all]/from_env-LIVE-Pfad routet ueber die Menge).
    // O-8 Schritt 9: auch die MENGEN-Form fuehrt das load_framework-Segment EINMAL vorne (nicht je Tool) --
    // es ist eine Eigenschaft der Mess-Zeile, nicht des einzelnen Tooling-Eintrags. Aus Werkzeug-Output geankert.
    namespace abi                            = ::comdare::cache_engine::abi;
    std::array<std::string_view, 2> const tw = {"wallclock", "macro"};
    EXPECT_EQ(abi::measurement_stamp_line(std::span<std::string_view const>{tw}),
              std::string{"load_framework=ycsb@1.0.0;measurement_tooling=wallclock@1.0.0;"
                          "measurement_tooling=macro@1.0.0"});
    // Leere Tokens werden uebersprungen (ehrlich: kein Tool an der Stelle).
    std::array<std::string_view, 3> const gappy = {"wallclock", "", "micro"};
    EXPECT_EQ(abi::measurement_stamp_line(std::span<std::string_view const>{gappy}),
              std::string{"load_framework=ycsb@1.0.0;measurement_tooling=wallclock@1.0.0;"
                          "measurement_tooling=micro@1.0.0"});
    // Leere Menge -> leere Zeile. AUCH das load_framework-Segment entfaellt dann: eine Mess-Zeile ohne jedes
    // Tooling ist ehrlich leer und nicht ein einsames Rahmen-Segment.
    EXPECT_TRUE(abi::measurement_stamp_line(std::span<std::string_view const>{}).empty());
    // Die Vollmenge = das volle Registry-Angebot {wallclock,macro,micro} in Registry-Reihenfolge (Single-Source).
    EXPECT_EQ(abi::measurement_stamp_line_full_set(),
              std::string{"load_framework=ycsb@1.0.0;measurement_tooling=wallclock@1.0.0;"
                          "measurement_tooling=macro@1.0.0;measurement_tooling=micro@1.0.0"});
    // SEPARATE Welt zur .algos-Sig: X.Y.Z-Voll-Form, NICHT die rohe "@v1".
    EXPECT_EQ(abi::measurement_stamp_line_full_set().find("@v1"), std::string::npos);
}

TEST(MW12StampBausteine, AnatomyVersionLinesPodLayoutIsStableAt136) {
    // K7a/K7b-3: merge_line/sha512_line append-only (Layout bis 4). G2-1b (A4, Section 58-V/Section 66): die drei
    // {ptr,count}-Array-Paare (organ_/system_/measurement_entries) ans POD-Ende angehaengt = die Array-Form der
    // Stempel-Zeilen. Layout-Bump 4 -> 5, sizeof 88 -> 136 (der EINE intendierte Pin-Nachzug im A4-Fenster). Die
    // Offsets ALLER bisherigen Felder (bis sha512_len @80) bleiben stabil (append-only). Der sizeof-static_assert lebt
    // in anatomy_module_abi_v1_decl.hpp und haelt build-weit -- hier zusaetzlich als literaler ctest-Beweis gespiegelt.
    // binary_id/CRC UNBERUEHRT (POD-Layout != binary_id); die Byte-Wache bleibt gruen (emittierte Quelle unveraendert).
    using ::comdare::cache_engine::abi::AnatomyVersionLines;
    static_assert(sizeof(AnatomyVersionLines) == 136, "POD-Layout-Wache: 18 Felder, 8-aligned -> 136 Byte (x86_64).");
    static_assert(alignof(AnatomyVersionLines) == 8);
    EXPECT_EQ(sizeof(AnatomyVersionLines), 136u);
    EXPECT_EQ(alignof(AnatomyVersionLines), 8u);
    EXPECT_EQ(::comdare::cache_engine::abi::kAnatomyVersionLinesLayout, 5u);
    // Offset-Stabilitaet der 12 Alt-Felder (append-only): bis sha512_len unveraendert.
    EXPECT_EQ(offsetof(AnatomyVersionLines, organ_line), 8u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, system_line), 24u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_line), 40u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_len), 48u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, merge_line), 56u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, merge_len), 64u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, sha512_line), 72u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, sha512_len), 80u);
    // A4: die drei neuen {ptr,count}-Paare folgen dahinter (@88..@128).
    EXPECT_EQ(offsetof(AnatomyVersionLines, organ_entries), 88u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, organ_entry_count), 96u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, system_entries), 104u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, system_entry_count), 112u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_entries), 120u);
    EXPECT_EQ(offsetof(AnatomyVersionLines, measurement_entry_count), 128u);
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
// Render-neutral fuer die gueltigen ids/Achsen; "v0.0.0"-Sentinel nur fuer ungueltige Tooling-ids (A13-M1b).
TEST(MW12StampBausteine, A2SystemAndToolingCodeVersionsSingleSource) {
    namespace abi = ::comdare::cache_engine::abi;
    // (a) System-Achsen-Single-Source: DREI Achsen (O-8 Schritt 4, A3-Kern), alle Init-Version "v1.0.0".
    EXPECT_EQ(abi::kSystemAxisCodeCount, std::size_t{3});
    for (auto const& e : abi::kSystemAxisCodeVersions) {
        EXPECT_FALSE(e.axis.empty());
        EXPECT_EQ(e.version, std::string_view{"v1.0.0"});
    }
    // Der frueher hier stehende "Neutralitaets-Anker" (byte-identisch zur alten v1-Hartkodierung) ist mit dem
    // A3-Rueckbau gegenstandslos: die Zeile SOLL sich geaendert haben. Sie ist aus dem Werkzeug-Output des
    // Fenster-Laufs neu geankert und bindet jetzt die Drei-Achsen-Ordnung.
    EXPECT_EQ(abi::system_stamp_line(),
              std::string{"target_isa=code@1.0.0;operating_system=code@1.0.0;external_utils=code@1.0.0"});

    // (b) Mess-Tooling-Version-Feld + id-Lookup.
    for (auto const& t : m::kMeasurementToolingRegistry) EXPECT_EQ(t.version, std::string_view{"v1.0.0"});
    EXPECT_EQ(m::tooling_version_for_id("wallclock"), std::string_view{"v1.0.0"});
    EXPECT_EQ(m::tooling_version_for_id("macro"), std::string_view{"v1.0.0"});
    EXPECT_EQ(m::tooling_version_for_id("micro"), std::string_view{"v1.0.0"});
    // A13-M1b (Owner-Q3, dreistellig): der Sentinel-Rueckgabewert ist "v0.0.0" statt der Kurzform "v0" --
    // byte-neutral, beide rendern "0.0.0" (Beleg im Render-Block (c) unten: "@0.0.0" unveraendert).
    EXPECT_EQ(m::tooling_version_for_id("bogus"), std::string_view{"v0.0.0"}); // unbekannt -> Sentinel

    // (c) Sentinel-Render: ungueltige Tooling-id -> @0.0.0; gueltige bleiben @1.0.0 (render-neutral).
    // Das load_framework-Segment (Schritt 9) steht auch hier vorne -- der Sentinel betrifft nur das Tooling-Glied.
    EXPECT_EQ(abi::measurement_stamp_line("bogus"),
              std::string{"load_framework=ycsb@1.0.0;measurement_tooling=bogus@0.0.0"});
    EXPECT_EQ(abi::measurement_stamp_line("wallclock"),
              std::string{"load_framework=ycsb@1.0.0;measurement_tooling=wallclock@1.0.0"});
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
    EXPECT_NE(h[0].reserved, e[1].reserved); // g != flagloser Bestand
    static_assert(abi::stamp_entry_hardware_flag(h[1]) == abi::StampEntryHardwareFlag::fpga);

    // (c3) Fehlform: ein ZWEITES Hardware-Flag ist grammatisch Sentinel -> KEIN Bit wandert ins reserved-Feld.
    static constexpr char kTwoFlags[] = "a=x@1.0.0cg";
    constexpr auto tf = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kTwoFlags})>(kTwoFlags);
    EXPECT_EQ(tf[0].x, 0u);
    EXPECT_EQ(tf[0].reserved, std::uint32_t{0});

    // (d) GOLDEN-NEUTRALITAET: eine Bestands-Zeile ohne Flags setzt in KEINEM Eintrag ein Bit.
    static constexpr char kPlain[] =
        "search_algo=k_ary@1.0.0;filter=bloom@2.3.4;target_isa=code@1.0.0"; // heutige Bestands-Form
    constexpr auto p = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kPlain})>(kPlain);
    for (auto const& x : p) EXPECT_EQ(x.reserved, std::uint32_t{0});

    // (e) Ein 'e' im VERSIONS-Anteil an falscher Stelle bleibt Sentinel (Parser raet nie).
    static constexpr char kBad[] = "achse=algo@1.0e";
    constexpr auto        b      = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kBad})>(kBad);
    EXPECT_EQ(b[0].x, 0u);
    EXPECT_EQ(b[0].y, 0u);
    EXPECT_EQ(b[0].z, 0u);
    EXPECT_EQ(b[0].reserved, std::uint32_t{0}); // Sentinel traegt weder Experimental- noch Hardware-Bit (K-5)
}

// A4 (G2-1b): die Array-Form reist durch das AnatomyVersionLines-POD. Der POD wird hier MANUELL exakt wie im
// COMDARE_ANATOMY_VERSION_STAMP_MERGE-Makro konstruiert (dieselbe Feld-Reihenfolge; die Aggregat-Init ist positions-
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
        "search_algo=k_ary@1.0.0;cache_traversal=t@1.0.0;mapping=m@1.0.0;path_compression=p@2.3.4;node_type=n@1.0.0;"
        "memory_layout=l@1.0.0;allocator=a@1.0.0;prefetch=pf@1.0.0;concurrency=c@1.0.0;serialization=s@1.0.0;"
        "value_handle=v@1.0.0;index_organization=i@1.0.0;io_dispatch=io@1.0.0;migration_policy=mp@1.0.0;filter=f@1.0.0;"
        "queuing_q1=q1@1.0.0;queuing_q2=q2@1.0.0;persistence_target=pt@1.0.0"; // 18 Haupt-Achsen (A8.2)
    static constexpr char kSystem[] =
        "target_isa=code@1.0.0;operating_system=code@1.0.0;external_utils=code@1.0.0"; // 3 (A3-Kern + A2-Rename)
    static constexpr char kMeasure[] = "load_framework=ycsb@1.0.0;measurement_tooling=wallclock@1.0.0;"
                                       "measurement_tooling=macro@1.0.0;measurement_tooling=micro@1.0.0"; // 4 (K1/S9)

    static constexpr auto kOE = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kOrgan})>(kOrgan);
    static constexpr auto kSE = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kSystem})>(kSystem);
    static constexpr auto kME =
        abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kMeasure})>(kMeasure);

    abi::AnatomyVersionLines const v{abi::kAnatomyVersionLinesLayout,
                                     0u,
                                     kOrgan,
                                     sizeof(kOrgan) - 1,
                                     kSystem,
                                     sizeof(kSystem) - 1,
                                     kMeasure,
                                     sizeof(kMeasure) - 1,
                                     "",
                                     0u,
                                     "deadbeef",
                                     8u,
                                     abi::stamp_entries_ptr(kOE),
                                     kOE.size(),
                                     abi::stamp_entries_ptr(kSE),
                                     kSE.size(),
                                     abi::stamp_entries_ptr(kME),
                                     kME.size()};

    EXPECT_TRUE(abi::stamp_pod_has_entries(v));
    EXPECT_EQ(v.stamp_layout_version, 5u);
    EXPECT_EQ(v.organ_entry_count, 18u);
    EXPECT_EQ(v.system_entry_count, 3u);
    EXPECT_EQ(v.measurement_entry_count, 4u);

    auto const join = [](abi::AnatomyStampEntryV1 const* e, std::uint64_t n) {
        std::string out;
        for (std::uint64_t i = 0; i < n; ++i) {
            if (i != 0) out += ';';
            out += std::string(e[i].axis, e[i].axis_len);
            out += '=';
            out += std::string(e[i].algorithm, e[i].algo_len);
            out += '@';
            out += std::to_string(e[i].x) + '.' + std::to_string(e[i].y) + '.' + std::to_string(e[i].z);
        }
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
    // O-8 Schritt 12: mit fuehrendem load_framework-Segment -- die consteval-CEB-Fassung ist dem
    // Schritt-9-Stand nachgezogen worden, damit der Drift-Guard darunter wieder Gleichheit sehen kann.
    EXPECT_EQ(ceb::kCebMeasurementStamp, std::string_view{"load_framework=ycsb@1.0.0;"
                                                          "measurement_tooling=wallclock@1.0.0;"
                                                          "measurement_tooling=macro@1.0.0;"
                                                          "measurement_tooling=micro@1.0.0"});
    // DRIFT-GUARD: die consteval-CEB-Zeile deckt sich EXAKT mit der Runtime-Tier-Binary-Mengen-Form -> EINE Wahrheit,
    // keine Parallel-Ableitung (Section-64-Vollmengen-Provenienz teilt sich die Quelle).
    EXPECT_EQ(std::string{ceb::kCebMeasurementStamp}, abi::measurement_stamp_line_full_set());
    // SHA-512-Provenienz: 128 hex, == Host-Nachrechnung via anatomy_fingerprint_hex ("","",mess,"").
    static_assert(ceb::kCebFingerprint.size() == 128);
    EXPECT_EQ(ceb::kCebFingerprint.size(), std::size_t{128});
    constexpr auto host = abi::anatomy_fingerprint_hex("", "", ceb::kCebMeasurementStamp, "");
    EXPECT_EQ(ceb::kCebFingerprint, std::string_view(host.data(), 128));
    // ceb_version_stamp() traegt beide Teile + die X.Y.Z-Form (keine rohe @v1).
    std::string const stamp = ceb::ceb_version_stamp();
    EXPECT_NE(stamp.find("ceb-measurement=load_framework=ycsb@1.0.0;measurement_tooling=wallclock@1.0.0"),
              std::string::npos);
    EXPECT_NE(stamp.find(";sha512="), std::string::npos);
    EXPECT_EQ(stamp.find("@v1"), std::string::npos);
}
