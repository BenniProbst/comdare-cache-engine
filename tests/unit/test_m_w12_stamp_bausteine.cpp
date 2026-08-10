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
#include <cache_engine/measurement/measurement_framework_registry.hpp> // S2: Bestands-Probe der Katalog-Wache
#include <cache_engine/measurement/pmc_vendor_registry.hpp> // I-PMC-2 (10.08.2026): die ZWEI PMC-Hardware-Komponenten
#include <sha512/ctsha512.hpp> // K7b-3: Referenz-SHA-512 fuer den Fingerprint-Korrektheitstest
#include <cache_engine/measurement/algo_semver.hpp>
#include <cache_engine/measurement/axis_version_stamp.hpp>
#include "builder/ceb_version_stamp.hpp"              // A5: CEB-Selbst-Stempel (consteval Mess-Array + SHA-512)
#include "pmc_ceb_asymmetrie.hpp"                     // I-PMC-2: DIE EINE Stelle der deklarierten CEB/TIER-Asymmetrie
#include <cache_engine/abi/toolchain_stamp_glied.hpp> // O-2/C-2: Renderer + kToolchainAxisVersions
#include <profile_facade/planner/planner_version.hpp>
#include <profile_facade/system_version_suffix.hpp>    // O-2/C-2: Doppel-Wahrheits-Wache gegen den Suffix
#include <profile_facade/overlay_source_hash_naht.hpp> // E-E: die LIVE-Naht des Glieds [7]
#include <profile_facade/toolchain_stamp_naht.hpp>     // NB/CX-4: die LIVE-Naht der Glieder [5]/[6]

#include "builder/build_variant_set_signature.hpp"    // NB-3/T2-D: die Paar-Wache (Praedikat-Haelfte)
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
#include <utility>     // die Fehlform-Tabelle der Form-Wache (Paar aus Beschreibung und Literal)
#include <vector>      // A4: die Flag-Schwanz-Gegenprobe der POD-Rekonstruktion
#include <axes/persistence_target/axis_persistence_target_memory_only.hpp> // STRUKT-R ORG-18

namespace m  = ::comdare::cache_engine::measurement;
namespace pl = ::comdare::cache_engine::planner;

namespace {
// Mock-Achsen (name() + algo_version) + Mock-Composition (17 benannte Aliase) fuer organ_stamp_line<Comp>().
// A13-M1b (Owner-Q3 02.08.2026, Kurzform-Rueckbau): die Mock-Version stand als Kurzform "v1" da und ist auf die
// dreistellige Form gezogen. RENDER-NEUTRAL -- "v1" und "1.0.0" rendern beide "1.0.0", die Golden-Strings der
// Organ-/Stempel-Zeilen unten bleiben damit byte-identisch (kein Anker wurde nachgezogen).
struct MockAxisV1 {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "algoA"; }
    static constexpr std::string_view               algo_version = "1.0.0";
};
struct MockAxisV234 {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "algoB"; }
    static constexpr std::string_view               algo_version = "2.3.4";
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

TEST(MW12StampBausteine, SemVerTripelUndKurzformRueckbau) {
    // Owner-Q3 (02.08.2026): "Die Kurzform ist verboten, Versionierungen sind einheitlich und immer
    // 3-Stellig." In der FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026) gilt das unveraendert -- geaendert hat
    // sich nur, dass das 'v'-Praefix ERSATZLOS entfallen ist (Regel R1).
    EXPECT_EQ(m::parse_algo_semver("1.0.0"), (m::AlgoSemVer{1, 0, 0}));
    EXPECT_EQ(m::parse_algo_semver("2.3.4"), (m::AlgoSemVer{2, 3, 4}));
    EXPECT_EQ(m::parse_algo_semver("0.0.0"), (m::AlgoSemVer{0, 0, 0}));
    EXPECT_EQ(m::parse_algo_semver("1"), (m::AlgoSemVer{}));   // Kurzform verboten
    EXPECT_EQ(m::parse_algo_semver("12"), (m::AlgoSemVer{}));  // auch mehrstellig
    EXPECT_EQ(m::parse_algo_semver("1.2"), (m::AlgoSemVer{})); // zweistellig verboten
    EXPECT_EQ(m::parse_algo_semver(""), (m::AlgoSemVer{}));
    // R1: die ALTE Q3-Schreibweise hat KEINE Uebergangs-Toleranz (Owner: "die alten Wege komplett
    // ersetzen"). Sie ist nicht "auch erlaubt", sondern unparsbar.
    EXPECT_EQ(m::parse_algo_semver("v1.0.0"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("v1.0.0c"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("v1.0.0ce"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("v0.0.0"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("v1.0.0.c"), (m::AlgoSemVer{})); // 'v' + neue Notation ebenso
    // ALIAS-WACHE der Token-Regel: eine VIERSTELLIGE Version ist NICHT zugleich eine dreistellige mit
    // einem Ziffern-Flag. Ohne sie waere "1.2.3.4" eine gueltige, aber ANDERE Identitaet als gemeint.
    EXPECT_EQ(m::parse_algo_semver("1.2.3.4"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("1.0.0.512"), (m::AlgoSemVer{}));
    EXPECT_FALSE(m::parse_algo_semver("1.0.0.x512").is_sentinel()); // mit Buchstabe darin sehr wohl
}

TEST(MW12StampBausteine, AlgoSemVerFullFormForStampsOnly) {
    // Die kanonische Form fuer Stempel/Registry.
    EXPECT_EQ(m::algo_semver_string("1.0.0"), std::string{"1.0.0"});
    EXPECT_EQ(m::algo_semver_string("2.3.4"), std::string{"2.3.4"});
    EXPECT_EQ(m::algo_semver_string("0.0.0"), std::string{"0.0.0"});
    // Die verbotene Kurzform ist Sentinel und rendert deshalb "0.0.0".
    EXPECT_EQ(m::algo_semver_string("1"), std::string{"0.0.0"});
    // FLAG-GRAMMATIK v2: rohe und gerenderte Form sind DIESELBE Zeichenfolge. Der Renderer ist damit fuer
    // ein wohlgeformtes Literal die IDENTITAET -- das ist keine Redundanz, sondern genau die Aussage von
    // Regel R1. Hier stand bis zur v2 das Gegenteil ("die Voll-Form ist NICHT der rohe String"), weil das
    // 'v' die beiden Welten trennte.
    EXPECT_EQ(m::algo_semver_string("1.0.0.c"), std::string{"1.0.0.c"});
    EXPECT_EQ(m::algo_semver_string("1.0.0.c{p.e}"), std::string{"1.0.0.c{p.e}"});
    // ... und die ALTE Form rendert den Sentinel, statt zu raten.
    EXPECT_EQ(m::algo_semver_string("v1.0.0c"), std::string{"0.0.0"});
}

// FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026, Design super docs/plaene/20260807-DESIGN-flag-grammatik-v2-*).
// Die Batterie prueft die ACHT Regeln je EINZELN. Sie ersetzt die A13-M1b-Batterie VOLLSTAENDIG: deren
// Negativ-Literale ("v1.0.0cc" = zweites Flag, "v1.0.0ec" = Reihenfolge verdreht, "v1.0.0x" = unbekanntes
// Flag-Zeichen) testeten Regeln, die es NICHT MEHR GIBT. Haette man sie nur auf die neue Schreibweise
// gezogen, waeren sie gruen geblieben -- aber aus einem anderen Grund als dem, fuer den sie standen
// ("1.0.0.cc" ist heute ein Token wie jedes andere und faellt nur noch am fehlenden Punkt durch).
TEST(MW12StampBausteine, FlagGrammatikV2RegelnEinzeln) {
    // R2: PUNKT VOR JEDEM FLAG, auch dem ersten.
    EXPECT_FALSE(m::parse_algo_semver("1.0.0.c").is_sentinel());
    EXPECT_EQ(m::parse_algo_semver("1.0.0c"), (m::AlgoSemVer{})); // die Q3-Schreibweise ohne Punkt
    EXPECT_EQ(m::parse_algo_semver("1.0.0..c"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("1.0.0."), (m::AlgoSemVer{}));

    // R3: BASIS DIREKT AN IHRER KLAMMER, ohne Punkt dazwischen.
    EXPECT_FALSE(m::parse_algo_semver("1.0.0.c{p.e}").is_sentinel());
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c.{p.e}"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("1.0.0.{p.e}"), (m::AlgoSemVer{})); // Klammer ohne Basis

    // R4: HINTER '{' NIE EIN FUEHRENDER PUNKT -- und nie eine leere Gruppe.
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{.p}"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{}"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{p..e}"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{p.}"), (m::AlgoSemVer{}));
    // unbalancierte Klammern, in beide Richtungen
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{p"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c}"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{p}}"), (m::AlgoSemVer{}));

    // R5: NUR DER PUNKT TRENNT -- der Unterstrich der Katalog-Token faellt weg (Owner-F-7).
    EXPECT_EQ(m::parse_algo_semver("1.0.0.avx512_vbmi2"), (m::AlgoSemVer{}));
    EXPECT_FALSE(m::parse_algo_semver("1.0.0.x512{vbmi2}").is_sentinel());
    EXPECT_EQ(m::parse_algo_semver("1.0.0.C"), (m::AlgoSemVer{}));    // Grossbuchstabe
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{P}"), (m::AlgoSemVer{})); // Grossbuchstabe im Sub

    // R6: KARDINALITAET 1 -> n, Reihenfolge formal erhalten (der Parser sortiert NICHT).
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c.g.f.n").flags.count, 4u);
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c.g").flags.nodes[0].view(), std::string_view{"c"});
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c.g").flags.nodes[1].view(), std::string_view{"g"});
    EXPECT_NE(m::parse_algo_semver("1.0.0.c.g"), m::parse_algo_semver("1.0.0.g.c"));
    EXPECT_TRUE(m::parse_algo_semver("1.0.0.c.g").has_top_level_flag("c"));
    EXPECT_FALSE(m::parse_algo_semver("1.0.0.c.g").has_top_level_flag("f"));

    // R7: 'e' IST EFFICIENCY CORE -- ein Flag wie jedes andere. Es gibt kein experimental-Konzept mehr;
    //     der Struct hat kein Feld dafuer, und die Politik-Wachen fragen nicht danach.
    EXPECT_FALSE(m::parse_algo_semver("1.0.0.c{e}").is_sentinel());
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{e}").flags.nodes[1].view(), std::string_view{"e"});
    EXPECT_TRUE(m::ce_owned_version_is_wellformed("1.0.0.c{e}"));
    EXPECT_TRUE(m::ce_owned_version_satisfies_cpu_enforce("1.0.0.c{e}"));

    // R8: 'p' und 'e' sind SUB-Flags unter 'c'; "{p}" ist der Default -- SEMANTISCH, nicht syntaktisch.
    //     Der Parser normalisiert NICHT: "1.0.0.c" und "1.0.0.c{p}" sind zwei Werte (der Stempel ist
    //     Identitaet, ein Auffuellen von Defaults wuerde die Zeichenfolge veraendern).
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{p.e}").flags.count, 3u);
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{p.e}").flags.nodes[0].depth, 0u);
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{p.e}").flags.nodes[1].depth, 1u);
    EXPECT_NE(m::parse_algo_semver("1.0.0.c"), m::parse_algo_semver("1.0.0.c{p}"));
    EXPECT_FALSE(m::parse_algo_semver("1.0.0.c{p}").has_top_level_flag("p")); // 'p' steht auf Tiefe 1

    // MEHRZEICHIGE BASEN: der Parser raet NICHT zeichenweise. Ohne diese Eigenschaft waere 'gfni' als
    // 'g' (GPU) + Rest 'fni' gelesen worden und 'x512' als 'x' + '512'.
    EXPECT_EQ(m::parse_algo_semver("1.0.0.gfni").flags.count, 1u);
    EXPECT_EQ(m::parse_algo_semver("1.0.0.gfni").flags.nodes[0].view(), std::string_view{"gfni"});
    EXPECT_NE(m::parse_algo_semver("1.0.0.gfni"), m::parse_algo_semver("1.0.0.g"));
    EXPECT_EQ(m::parse_algo_semver("1.0.0.x512{f}").flags.nodes[0].view(), std::string_view{"x512"});
    EXPECT_EQ(m::parse_algo_semver("1.0.0.x256{f}").flags.nodes[0].view(), std::string_view{"x256"});
    EXPECT_EQ(m::parse_algo_semver("1.0.0.x128{f}").flags.nodes[0].view(), std::string_view{"x128"});
    EXPECT_EQ(m::parse_algo_semver("1.0.0.vpclmulqdq").flags.nodes[0].view(), std::string_view{"vpclmulqdq"});

    // DAS OWNER-BEISPIEL, vollstaendig.
    auto const owner = m::parse_algo_semver("1.0.0.c{p.e}.x512{f.vl.bw.dq}.gfni");
    EXPECT_EQ(owner.flags.count, 9u);
    EXPECT_TRUE(owner.has_top_level_flag("c"));
    EXPECT_TRUE(owner.has_top_level_flag("x512"));
    EXPECT_TRUE(owner.has_top_level_flag("gfni"));
    EXPECT_FALSE(owner.has_top_level_flag("vl")); // 'vl' steht auf Tiefe 1

    // REKURSION > 1: die Struktur traegt sie HEUTE, ohne dass ein Bestands-Literal sie ausuebt.
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{p{x}.e}").flags.count, 4u);
    EXPECT_EQ(m::parse_algo_semver("1.0.0.c{p{x}.e}").flags.nodes[2].depth, 2u);
    EXPECT_EQ(m::parse_algo_semver("1.0.0.a{b{c{d{e}}}}").flags.nodes[4].depth, m::kMaxFlagDepth);
    EXPECT_EQ(m::parse_algo_semver("1.0.0.a{b{c{d{e{f}}}}}"), (m::AlgoSemVer{})); // > kMaxFlagDepth

    // K-5-SENTINEL: das Null-Tripel ist der REINE Sentinel -- auch mit Flags. Sonst haette ein "0.0.0.c"
    // die Registry-Wache ausgehebelt.
    EXPECT_EQ(m::parse_algo_semver("0.0.0.c"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("0.0.0.c{p.e}"), (m::AlgoSemVer{}));
    EXPECT_FALSE(m::parse_algo_semver("0.0.0.c").has_flags());
    EXPECT_TRUE(m::parse_algo_semver("0.0.0.c").is_sentinel());
    EXPECT_FALSE(m::parse_algo_semver("1.0.0.c").is_sentinel());
    static_assert(m::parse_algo_semver("0.0.0.c{p.e}") == m::AlgoSemVer{});

    // UNTERSCHEIDBARKEIT (Fingerprint-/Lager-Wirkung): jede Flag-Form ist ein anderes Stempel-Segment.
    EXPECT_NE(m::parse_algo_semver("1.0.0.c"), m::parse_algo_semver("1.0.0"));
    EXPECT_NE(m::parse_algo_semver("1.0.0.c"), m::parse_algo_semver("1.0.0.g"));
    EXPECT_NE(m::parse_algo_semver("1.0.0.c"), m::parse_algo_semver("1.0.0.c{e}"));
    EXPECT_NE(m::parse_algo_semver("1.0.0.x512{f}"), m::parse_algo_semver("1.0.0.x512{f.vl}"));
}

TEST(MW12StampBausteine, FlagGrammatikV2RoundtripIstVerlustfrei) {
    // Parsen -> rendern -> parsen ergibt DASSELBE, und die Zeichenfolge kommt VERBATIM zurueck. Das ist
    // die Zusage, an der die Stempel-Identitaet haengt: der Renderer erfindet nichts und laesst nichts weg.
    for (char const* lit :
         {"1.0.0", "1.0.0.c", "1.0.2.c", "2.3.4.c{p.e}", "1.0.0.x512{f}", "1.0.0.c.g.f.n", "1.0.0.c{p{x}.e}",
          "10.0.1.c", "999999.999999.999999.c", "1.0.0.c{p.e}.x512{f.vl.bw.dq}.gfni", "0.0.0"}) {
        auto const v = m::parse_algo_semver(lit);
        auto const r = m::render_algo_semver(v);
        EXPECT_EQ(r.view(), std::string_view{lit}) << "Render weicht ab fuer '" << lit << "'";
        EXPECT_EQ(m::parse_algo_semver(r.view()), v) << "Roundtrip nicht stabil fuer '" << lit << "'";
        EXPECT_EQ(m::algo_semver_string(lit), std::string{lit}); // die std::string-Bequemlichkeit gleich mit
    }
    // Der Sentinel rendert IMMER nackt -- auch wenn das Quell-Literal Flags trug (K-5).
    EXPECT_EQ(m::render_algo_semver(m::parse_algo_semver("0.0.0.c{p}")).view(), std::string_view{"0.0.0"});
    // Und ein UNPARSBARES Literal rendert den Sentinel, statt zu raten.
    EXPECT_EQ(m::render_algo_semver(m::parse_algo_semver("v1.0.0c")).view(), std::string_view{"0.0.0"});
    EXPECT_EQ(m::render_algo_semver(m::parse_algo_semver("quatsch")).view(), std::string_view{"0.0.0"});
    // Der Flag-Schwanz-Renderer ist derselbe Weg wie die Voll-Form (Grundlage der POD-Kodierung, Owner-F-3).
    EXPECT_EQ(m::render_flag_tail(m::parse_algo_semver("1.0.0.c{p.e}").flags).view(), std::string_view{".c{p.e}"});
    EXPECT_EQ(m::render_flag_tail(m::parse_algo_semver("1.0.0").flags).view(), std::string_view{""});
}

TEST(MW12StampBausteine, FlagGrammatikV2PolitikWachen) {
    // Owner-F-10: "ce-eigene Achsen tragen mindestens 'c'". Diese Wache ERSETZT die alte B12-Regel
    // "ce-Registry traegt NIE 'e'" -- die ist mit der Bedeutungsaenderung von 'e' gegenstandslos, nicht
    // abgeschwaecht (Owner verbatim zum Vorschlag: "Der vorschlag trifft ins Schwarze. Genau so.").
    EXPECT_TRUE(m::version_satisfies_cpu_only_policy("1.0.0.c"));
    EXPECT_TRUE(m::version_satisfies_cpu_only_policy("1.0.0.c{p.e}"));
    EXPECT_TRUE(m::version_satisfies_cpu_only_policy("1.0.0.c{p.e}.x512{f}"));
    EXPECT_TRUE(m::version_satisfies_cpu_only_policy("1.0.0.c.g")); // NEU entscheidbar (Kardinalitaet 1->n)
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("1.0.0"));    // flaglos
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("1.0.0.g"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("1.0.0.f"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("1.0.0.n"));
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("1.0.0.x512{f}")); // SIMD ohne CPU-Basis
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("0.0.0.c"));       // Sentinel erfuellt nie eine Politik
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy("v1.0.0c"));       // die Alt-Form ebenfalls nicht
    EXPECT_FALSE(m::version_satisfies_cpu_only_policy(""));
    // Die PFLICHT ist scharf, und der BESTAND erfuellt sie -- gemessen an der Registry, nicht behauptet.
    static_assert(COMDARE_VERSION_HW_FLAG_ENFORCE == 1, "die Pflicht ist scharf geschaltet.");
    for (auto const& t : m::kMeasurementToolingRegistry)
        EXPECT_TRUE(m::version_satisfies_cpu_only_policy(t.version))
            << "Bestands-Version '" << t.version << "' ohne CPU-Flag -- die Migration hat sie ausgelassen.";
}

// S2 (07.08.2026), DIE KATALOG-WACHE. Die Grammatik prueft die FORM, dieser Test die BEDEUTUNG:
// steht jedes Flag-Token im Katalog (measurement/flag_grammar_catalog.hpp) und unter SEINER Basis?
//
// WARUM ER ALS LAUFZEIT-TEST EXISTIERT, OBWOHL DIE WACHE COMPILE-TIME IST: die static_assert-Batterie in
// algo_semver.hpp ist der eigentliche Beweis -- sie bricht den Bau, bevor dieser Test je laeuft. Sie ist
// aber im ctest-Protokoll UNSICHTBAR: ein gruener Lauf sagt nichts darueber, ob die Wache existiert oder
// ob jemand sie herausgenommen hat. Dieser Test macht sie im Protokoll SICHTBAR und nennt bei einem
// Bruch das verletzende Literal beim Namen.
TEST(MW12StampBausteine, S2KatalogWacheKenntTokenUndSeineBasis) {
    // (1) UNBEKANNTES TOKEN -- auf jeder Tiefe. Der Parser haelt die Form fuer richtig, der Katalog nicht.
    EXPECT_FALSE(m::parse_algo_semver("1.0.0.x512{quatsch}").is_sentinel());                 // formal wohlgeformt ...
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.x512{quatsch}"))); // ... und leer
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.quatsch")));
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.c{quatsch}")));

    // (2) BEKANNTES TOKEN UNTER FALSCHER BASIS -- die eigentliche Leistung der Wache.
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.x512{sse2}"))); // sse2 -> x128
    EXPECT_TRUE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.x128{sse2}")));  // ... dort richtig
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.x128{vl}")));   // vl -> x512
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.x128{avx2}"))); // avx2 -> x256
    // 'p'/'e' NUR unter 'c' (Owner-R8).
    EXPECT_TRUE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.c{p.e}")));
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.p")));
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.g{p}")));

    // (3) COMPANION UND SKALAR gehoeren NEBEN die Basis, nicht IN sie -- aus entgegengesetzten Gruenden
    //     (Companion: die Breite folgt der Basis; Skalar: es gibt keine Breite).
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.x512{gfni}")));
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.x512{popcnt}")));
    EXPECT_TRUE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.x512{f.vl}.gfni.popcnt")));

    // (4) FALL (4) IST ENTSCHIEDEN (07.08.2026): NUR die m64-Basis-Form ist noch katalog-gueltig, die
    //     alte bare-Token-Form bricht jetzt LAUT (compile-time UND hier, zur Laufzeit sichtbar).
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.c.mmx.mmxext.3dnow.3dnowext")));
    EXPECT_TRUE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.c.m64{mmx.mmxext.3dnow.3dnowext}")));
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.c.x64{mmx.mmxext.3dnow.3dnowext}")))
        << "der alte Name 'x64' ist nicht mehr im Katalog";
    EXPECT_FALSE(m::flag_catalog_is_satisfied(m::parse_algo_semver("1.0.0.x128{mmx}"))); // andere Registerdatei
    EXPECT_EQ(m::flag_catalog_offene_entscheide(), 0u) << "der Fall-(4)-Entscheid ist gefallen, sechs Eintraege zu";

    // (5) DIE DREI NAMENSRAEUME: getrennt gefuehrt, nie ineinander umgerechnet.
    EXPECT_EQ(m::flag_token_for_cpuinfo("pni"), std::string_view{"sse3"}); // cpuinfo != Token
    EXPECT_EQ(m::flag_token_for_cpuinfo("sha_ni"), std::string_view{"sha"});
    EXPECT_EQ(m::flag_token_for_cpuinfo("avx512_vbmi2"), std::string_view{"vbmi2"});
    EXPECT_TRUE(m::flag_token_for_cpuinfo("sse3").empty()); // "sse3" ist ein TOKEN, keine cpuinfo-Id
    // Und die beiden fremden Namensraeume sind als Token gar nicht schreibbar (Punkt bzw. Unterstrich).
    EXPECT_EQ(m::parse_algo_semver("1.0.0.x128{sse4.1}"), (m::AlgoSemVer{}));
    EXPECT_EQ(m::parse_algo_semver("1.0.0.avx512_vbmi2"), (m::AlgoSemVer{}));

    // (6) DER VOLLAUSBAU geht vollstaendig durch -- 59 Knoten, jedes ein echtes Token an seiner Stelle.
    //     Ein Katalog, der eine legitime Vollform verwirft, ist ein Defekt und keine Wache.
    constexpr std::string_view kVoll =
        "1.0.0.c{p.e}"
        ".x128{sse.sse2.sse3.ssse3.sse41.sse42.sse4a.aes.pclmulqdq.sha}"
        ".x256{avx.avx2.fma.f16c.vnni.ifma.vnniint8.vnniint16.neconvert.sha512.sm3.sm4}"
        ".x512{f.cd.vl.dq.bw.ifma.vbmi.vbmi2.vnni.bitalg.vpopcntdq.vp2intersect.bf16.fp16}"
        ".gfni.vaes.vpclmulqdq.popcnt.bmi1.bmi2.abm.movbe.adx.rdrand.rdseed"
        ".m64{mmx.mmxext.3dnow.3dnowext}.3dnowprefetch";
    EXPECT_EQ(m::parse_algo_semver(kVoll).flags.count, 59u);
    EXPECT_TRUE(m::ce_owned_version_is_wellformed(kVoll));

    // (7) DER BESTAND BLEIBT GRUEN -- gemessen an den echten Registries, nicht behauptet.
    for (auto const& t : m::kMeasurementToolingRegistry)
        EXPECT_TRUE(m::ce_owned_version_is_wellformed(t.version))
            << "Bestands-Version '" << t.version << "' faellt an der Katalog-Wache.";
    for (auto const& f : m::kMeasurementFrameworkRegistry)
        EXPECT_TRUE(m::ce_owned_version_is_wellformed(f.version))
            << "Bestands-Version '" << f.version << "' faellt an der Katalog-Wache.";

    // (8) DIE DRIFT-BRUECKE: jeder der 23 Eintraege des alten SIMD-Katalogs hat hier ein Token, und die
    //     Compiler-Schalter stimmen ueberein. Wer dort ein Flag ergaenzt, ohne ihm hier eines zu geben,
    //     bricht den Bau -- dieser Test nennt dann das Flag.
    for (auto const& f : m::kSimdFeatureFlagCatalog) {
        std::size_t const i = m::find_flag_catalog_entry_by_cpuinfo(f.cpuinfo);
        ASSERT_NE(i, m::kNoFlagCatalogEntry) << "SIMD-Flag '" << f.cpuinfo << "' hat kein Grammatik-Token.";
        EXPECT_EQ(m::kFlagGrammarCatalog[i].gpp, f.gpp)
            << "Compiler-Schalter driften fuer '" << f.cpuinfo << "' zwischen den beiden Katalogen.";
    }
}

TEST(MW12StampBausteine, FlagGrammatikV2StempelZeileEndeZuEnde) {
    // Die Flags erscheinen im Segment (und damit im SHA-512-Preimage) -- inklusive der Komposit-Klammern.
    std::array<m::AxisVersionEntry, 2> const entries{
        {{"path_compression", "prt_patricia", "2.3.4.c{p.e}"}, {"filter", "bloom", "1.0.0"}}};
    EXPECT_EQ(m::build_axis_version_stamp_line(entries),
              std::string{"path_compression=prt_patricia@2.3.4.c{p.e};filter=bloom@1.0.0"});
    // Jede Flag-Belegung ist gerendert verschieden von der flaglosen und von jeder anderen.
    EXPECT_NE(m::algo_semver_string("2.3.4.c"), m::algo_semver_string("2.3.4"));
    EXPECT_NE(m::algo_semver_string("2.3.4.c"), m::algo_semver_string("2.3.4.g"));
    EXPECT_NE(m::algo_semver_string("2.3.4.c{e}"), m::algo_semver_string("2.3.4.c"));
    // Die '{'/'}' der Versions-Grammatik kollidieren NICHT mit den '['/']' der Zeilen-Grammatik.
    std::string const zeile = m::build_axis_version_stamp_line(entries);
    EXPECT_EQ(zeile.find('['), std::string::npos);
    EXPECT_EQ(std::count(zeile.begin(), zeile.end(), '{'), 1);
    EXPECT_EQ(std::count(zeile.begin(), zeile.end(), '}'), 1);
}

// FLAG-GRAMMATIK v2 / Owner-Direktive 07.08.2026 ("wir WOLLEN den Bestand invalidieren" -- aber LAUT):
// die Form-Wache der Stempel-Eintraege. Sie ist das Praedikat, auf dem die static_assert in
// abi::organ_stamp_line<Comp> und abi::system_stamp_line sitzt.
//
// SIE SCHLIESST EINE GEMESSENE LUECKE. Bissprobe vor ihrem Einbau (07.08.2026, eigene Probe-TU): eine
// Composition mit dem ALT-Literal "v1.0.0c" an allen 18 Organ-Achsen uebersetzte KLAGLOS und lieferte
// "search_algo=algoALT@0.0.0;...;persistence_target=algoALT@0.0.0" -- achtzehn Nicht-Staende, die als
// Stempel in den SHA512-Fingerprint und damit in die Lager-Identitaet gereist waeren. Genau die
// Alias-Identitaet, gegen die die B11-Wachen eine Ebene tiefer gebaut sind.
//
// WARUM DIESER TEST DAS PRAEDIKAT PRUEFT UND NICHT organ_stamp_line SELBST: die Wache dort ist eine
// static_assert im Funktions-Rumpf -- eine Instanziierung mit Alt-Literal ist ein HARTER Compile-Fehler
// und laesst sich nicht in einem laufenden Test ausdruecken. Das Praedikat ist derselbe Zeuge, nur
// aufrufbar. Der Compile-Bruch selbst ist am Objekt belegt (s.o.).
TEST(MW12StampBausteine, FormWacheDerStempelEintraegeBeisstAufAltLiteralen) {
    using m::axis_version_entries_are_wellformed;
    using m::AxisVersionEntry;

    // (1) POSITIV: die kanonischen Formen gehen durch -- inklusive der FLAGLOSEN (Fremd-Pruefling,
    //     Test-Mock) und des dokumentierten Sentinels (die ABSICHT "Version unbekannt").
    static constexpr std::array<AxisVersionEntry, 5> kGut{{
        {"a", "x", "1.0.0.c"},
        {"b", "y", "1.0.2.c"},
        {"c", "z", "1.0.0.c{p.e}.x512{f.vl}.gfni"},
        {"d", "w", "1.0.0"}, // flaglos: hier ausdruecklich zulaessig (nicht jede Zeile ist ce-eigen)
        {"e", "v", "0.0.0"}, // der DOKUMENTIERTE Sentinel -- Absicht, kein Unfall
    }};
    EXPECT_TRUE(axis_version_entries_are_wellformed(kGut));
    static_assert(axis_version_entries_are_wellformed(kGut));

    // (2) NEGATIV: DIE ALT-FORM. Das ist der Fall, der vor dieser Wache still durchlief.
    static constexpr std::array<AxisVersionEntry, 1> kAltform{{{"a", "x", "v1.0.0c"}}};
    EXPECT_FALSE(axis_version_entries_are_wellformed(kAltform));
    static_assert(!axis_version_entries_are_wellformed(kAltform));

    // (3) NEGATIV: die uebrigen Wege, auf denen ein Nicht-Stand still als "@0.0.0" reisen wuerde.
    //     Jeder EINZELN -- ein Sammel-Assert saehe nicht, wenn nur noch einer davon greift.
    for (auto const& [text, fehlform] : std::array<std::pair<char const*, char const*>, 7>{{
             {"v-Praefix mit neuer Notation", "v1.0.0.c"},
             {"Flag ohne fuehrenden Punkt (Q3)", "1.0.0c"},
             {"Kurzform", "1.0"},
             {"fuehrender Punkt hinter '{'", "1.0.0.c{.p}"},
             {"leere Gruppe", "1.0.0.c{}"},
             {"unbalancierte Klammer", "1.0.0.c{p"},
             {"Sentinel-WERT, aber undokumentiertes Literal", "0.0.0.c"},
         }}) {
        std::array<AxisVersionEntry, 1> const eintrag{{{"a", "x", fehlform}}};
        EXPECT_FALSE(axis_version_entries_are_wellformed(eintrag)) << text << " -> '" << fehlform << "'";
    }

    // (4) EINE Fehlform unter vielen guten reicht -- die Wache prueft JEDEN Eintrag, nicht den ersten.
    static constexpr std::array<AxisVersionEntry, 3> kEineFaul{{
        {"a", "x", "1.0.0.c"},
        {"b", "y", "v1.0.0c"}, // die faule in der MITTE
        {"c", "z", "1.0.0.c"},
    }};
    static_assert(!axis_version_entries_are_wellformed(kEineFaul));
}

// FLAG-GRAMMATIK v2: die '{}' der VERSIONS-Grammatik und die '{}' des TOOLCHAIN-Glieds koexistieren ab
// jetzt im selben Stempel-Text -- "opt=O3{-O3}@1.0.0.c" traegt beide Sorten in EINEM Segment. Sie sind
// eindeutig getrennt, weil die einen VOR und die anderen HINTER dem '@' stehen. Das ist keine
// Selbstverstaendlichkeit, sondern eine Zusage, an der ein kuenftiger Leser sich orientieren muss: wer
// nach '{' scannt, ohne vorher am '@' zu trennen, verwechselt sie.
TEST(MW12StampBausteine, VersionsKlammerUndToolchainKlammerKollidierenNicht) {
    namespace abi = ::comdare::cache_engine::abi;
    // Die reale Toolchain-Segment-Form parst als Stempel-Eintrag: Name inkl. Flag-Klammer links vom '@',
    // Versions-Grammatik rechts davon.
    static constexpr char kLit[] = "opt=O3{-O3}@1.0.0.c{p.e}";
    constexpr auto        e      = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kLit})>(kLit);
    static_assert(e.size() == 1);
    EXPECT_EQ(std::string_view(e[0].algorithm, e[0].algo_len), std::string_view{"O3{-O3}"})
        << "die Compiler-Flag-Klammer gehoert zum ALGORITHMUS-Anteil, links vom '@'.";
    EXPECT_EQ(e[0].x, 1u);
    EXPECT_TRUE(abi::stamp_entry_has_flags(e[0])) << "und die Versions-Klammer rechts vom '@' liefert den Flag-Hash.";
    // Der Flag-Hash ist GENAU der von "1.0.0.c{p.e}" -- die linke Klammer faerbt nicht in ihn hinein.
    EXPECT_EQ(abi::stamp_entry_flags_hash(e[0]),
              abi::stamp_entry_flags_hash_of(m::parse_algo_semver("1.0.0.c{p.e}").flags));
    // Gegenprobe: die Compiler-Flag-Schreibweise ist als VERSIONS-Flag unparsbar (sie traegt ein '-').
    EXPECT_EQ(m::parse_algo_semver("1.0.0.O3{-O3}"), (m::AlgoSemVer{}));
}

TEST(MW12StampBausteine, AxisVersionStampLineUsesFullSemverAndCanonicalOrder) {
    // Stempel-Zeile "achse=algorithmus@X.Y.Z;..." in Eingabe- (== compose-) Reihenfolge, Voll-Form via algo_semver.
    // A13-M1b: die Eingabe steht dreistellig ("v1" war Kurzform) -- RENDER-NEUTRAL, der Golden-String unten
    // ist unveraendert.
    std::array<m::AxisVersionEntry, 2> const entries{{{"search_algo", "bst", "1.0.0"}, {"filter", "bloom", "2.3.4"}}};
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
    // System-Meta-Metas -- heute "[simd=code@1.0.0.c]". Der Trenner-Anker steigt deshalb BEWUSST von 2 auf 3
    // (drei Haupt-Achsen + ein Klammer-Anhang == VIER Eintraege). Neu geankert aus dem Ist-Output des
    // A13-M2-Laufs, nicht aus der Zaehlung von Hand.
    std::string const line = ::comdare::cache_engine::abi::system_stamp_line();
    EXPECT_EQ(std::count(line.begin(), line.end(), ';'), 3); // 3 Haupt-Achsen + 1 Klammer-Anhang -> 3 Trenner
    EXPECT_EQ(line.rfind("target_isa=code@1.0.0.c", 0), 0u);
    EXPECT_NE(line.find(";operating_system=code@1.0.0.c"), std::string::npos);
    EXPECT_NE(line.find(";external_utils=code@1.0.0.c"), std::string::npos);
    // Die drei abgewanderten Namen duerfen hier NICHT mehr auftauchen -- sonst waere der Umbau nur halb.
    EXPECT_EQ(line.find("compiler="), std::string::npos);
    EXPECT_EQ(line.find("scheduling="), std::string::npos);
    EXPECT_EQ(line.find("extension_hardware="), std::string::npos);
    EXPECT_EQ(line.find("load_framework="), std::string::npos);
    EXPECT_EQ(line.find("@v1"), std::string::npos); // separate Welt zur .algos-Sig
    // A13-M2: der Anhang steht ANS ENDE der Kette (Owner-E2) und ist geklammert (Owner-Q1) -- NICHT als
    // Punkt-Pfad "external_utils.simd=" (die verworfene Design-Empfehlung) und NICHT vor den Haupt-Achsen.
    EXPECT_TRUE(line.ends_with(";[simd=code@1.0.0.c]")) << "line=" << line;
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
    EXPECT_EQ(abi::meta_meta_stamp_suffix<m::ExternalUtilsHub>(), std::string{"[simd=code@1.0.0.c]"});
    // Leerer Hub (ein Blatt ist kein Hub) -> LEERER Anhang, kein "[]": daran haengt die Byte-Neutralitaet
    // des heute leeren Organ-Realms.
    EXPECT_TRUE(abi::meta_meta_stamp_suffix<m::SimdExternalUtilsFamily>().empty());
    // Die ENTRY-getriebene Form (Mess-Realm: die Wahl kommt aus der Registry, nicht aus dem Typ) schreibt
    // dieselbe Klammer ueber dieselbe Stelle.
    std::array<m::AxisVersionEntry, 1> const lf{{{"load_framework", "ycsb", "1.0.0.c"}}};
    EXPECT_EQ(abi::meta_meta_stamp_suffix_from(std::span<m::AxisVersionEntry const>{lf}),
              std::string{"[load_framework=ycsb@1.0.0.c]"});
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
        "target_isa=code@1.0.0.c;operating_system=code@1.0.0.c;external_utils=code@1.0.0.c;[simd=code@1.0.0.c]";
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
    // Die drei Bit-Gruppen ueberlappen NICHT: Bits 0-2 (frei) / Bits 3-5 (Ebene) / Bits 6-31 (Flag-Hash).
    EXPECT_EQ(abi::kStampEntryReservedFreeMask & abi::kStampEntryMetaLevelMask, std::uint32_t{0});
    EXPECT_EQ(abi::kStampEntryFlagsHashMask & abi::kStampEntryMetaLevelMask, std::uint32_t{0});

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
    EXPECT_TRUE(
        (abi::stamp_line_is_parsable<
            "target_isa=code@1.0.0.c;operating_system=code@1.0.0.c;external_utils=code@1.0.0.c;[simd=code@1.0.0.c]">));
    EXPECT_TRUE((abi::stamp_line_is_parsable<"measurement_tooling=wallclock@1.0.0.c;[load_framework=ycsb@1.0.0.c]">));
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
    EXPECT_EQ(line, std::string{"measurement_tooling=wallclock@1.0.0.c;[load_framework=ycsb@1.0.0.c]"});
    // EINE Tooling-Haupt-Achse + der Klammer-Anhang -> genau EIN ';'-Trenner (Ablaufmethodik/Workloads sind
    // UNTER-Achsen -> nie Bestandteil).
    EXPECT_EQ(std::count(line.begin(), line.end(), ';'), 1);
    EXPECT_EQ(line.rfind("measurement_tooling=wallclock@1.0.0.c", 0), 0u); // ERSTES Segment ist jetzt das Tooling
    EXPECT_TRUE(line.ends_with(";[load_framework=ycsb@1.0.0.c]")) << "line=" << line;
    EXPECT_EQ(line.find("@v1"), std::string::npos); // separate Welt zur .algos-Sig (X.Y.Z, nicht roh)
    // Andere Tooling-Haupt-Wahlen materialisieren analog -- der load_framework-Anhang bleibt konstant dahinter.
    EXPECT_EQ(::comdare::cache_engine::abi::measurement_stamp_line("macro"),
              std::string{"measurement_tooling=macro@1.0.0.c;[load_framework=ycsb@1.0.0.c]"});
    EXPECT_EQ(::comdare::cache_engine::abi::measurement_stamp_line("micro"),
              std::string{"measurement_tooling=micro@1.0.0.c;[load_framework=ycsb@1.0.0.c]"});
    // Leere Wahl -> leere Zeile (ehrlich: kein Mess-Tooling einkompiliert). AUCH der Klammer-Anhang entfaellt
    // dann: eine Mess-Zeile ohne jedes Tooling ist ehrlich leer und nie ein einsamer Rahmen-Anhang.
    EXPECT_TRUE(::comdare::cache_engine::abi::measurement_stamp_line("").empty());
}

TEST(MW12StampBausteine, MeasurementStampLineSetFormCarriesToolingMenge) {
    // K7b-2 (Section 64-D1-B, 2026-07-22): die MENGEN-Form -- N Tools -> N ';'-getrennte
    // measurement_tooling=<t>@1.0.0.c-Eintraege (Eingabe-Reihenfolge, Section-64-Vollmengen-Provenienz). Additive
    // span-Ueberladung; die Einzel-Form oben bleibt unveraendert (der [all]/from_env-LIVE-Pfad routet ueber die Menge).
    // A13-M2 (OP-3-Rueckbau, Owner-E2/Q1): auch die MENGEN-Form fuehrt den load_framework-Anhang EINMAL --
    // aber jetzt geklammert AM ENDE (nicht je Tool und nicht vorne). Er ist eine Eigenschaft der Mess-ZEILE,
    // nicht des einzelnen Tooling-Eintrags. Neu geankert aus dem Ist-Output des A13-M2-Laufs.
    namespace abi                            = ::comdare::cache_engine::abi;
    std::array<std::string_view, 2> const tw = {"wallclock", "macro"};
    EXPECT_EQ(abi::measurement_stamp_line(std::span<std::string_view const>{tw}),
              std::string{"measurement_tooling=wallclock@1.0.0.c;measurement_tooling=macro@1.0.0.c;"
                          "[load_framework=ycsb@1.0.0.c]"});
    // Leere Tokens werden uebersprungen (ehrlich: kein Tool an der Stelle).
    std::array<std::string_view, 3> const gappy = {"wallclock", "", "micro"};
    EXPECT_EQ(abi::measurement_stamp_line(std::span<std::string_view const>{gappy}),
              std::string{"measurement_tooling=wallclock@1.0.0.c;measurement_tooling=micro@1.0.0.c;"
                          "[load_framework=ycsb@1.0.0.c]"});
    // Leere Menge -> leere Zeile. AUCH der load_framework-Anhang entfaellt dann: eine Mess-Zeile ohne jedes
    // Tooling ist ehrlich leer und nicht ein einsamer Rahmen-Anhang.
    EXPECT_TRUE(abi::measurement_stamp_line(std::span<std::string_view const>{}).empty());
    // Die Vollmenge = das volle Registry-Angebot {wallclock,macro,micro} in Registry-Reihenfolge (Single-Source).
    EXPECT_EQ(abi::measurement_stamp_line_full_set(),
              std::string{"measurement_tooling=wallclock@1.0.0.c;measurement_tooling=macro@1.0.0.c;"
                          "measurement_tooling=micro@1.0.0.c;[load_framework=ycsb@1.0.0.c]"});
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
    // R-3 (Format 4): das NEUNTE Glied. anatomy_fingerprint_hex("a","b","c") ruft den DEFAULT, und der
    // ist die LEERE Identitaet (NICHT kMessGatesTuGlied -- s. den ODR-Absatz an anatomy_fingerprint_glieder).
    // Der Separator bleibt trotzdem stehen: genau darauf beruht die Injektivitaet der Zerlegung.
    ref_pre += '\n';
    ref_pre += std::string_view{};
    auto const ref = s5::to_hex(s5::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(ref_pre.data()), ref_pre.size()}));
    for (std::size_t i = 0; i < 128; ++i) EXPECT_EQ(fp[i], ref[i]) << "hex-Stelle " << i;

    // Die Format-Kennung ist das ERSTE Glied (F7: Layout-Evolution mismatcht deterministisch statt still
    // zu kollidieren) und das Werteset-Segment ein EIGENES Glied (F7-VERIFY, "schwerster Befund": sonst
    // wuerde ein Werteset-Bump unter dem SHA512-only-Skip-Gate STILL reused).
    constexpr auto glieder = abi::anatomy_fingerprint_glieder("a", "b", "c");
    static_assert(glieder.size() == 9u); // O-2/C-2: 6 -> 8 (Toolchain + bvset); R-3: 8 -> 9 (Mess-Gates)
    static_assert(glieder[0] == abi::kAnatomyFingerprintFormat);
    static_assert(glieder[4] == abi::kSubAxisValuesetSegment);
    // O-2/C-2: die drei Schwanz-Glieder ueber ihre BENANNTEN Positionen adressiert -- nicht ueber nackte
    // Zahlen. Eine Umsortierung ohne Nachzug der Konstanten bricht damit hier, statt still zu wandern.
    static_assert(glieder[abi::kAnatomyFingerprintToolchainGlied] == abi::kToolchainStampGlied);
    static_assert(glieder[abi::kAnatomyFingerprintBvsetGlied] == abi::kBuildVariantSetSignatureGlied);
    static_assert(glieder[abi::kAnatomyFingerprintOverlayGlied] == abi::kOverlaySourceHash);
    // R-3: das neunte Glied im DEFAULT-Aufruf ist die LEERE Identitaet -- ausdruecklich NICHT
    // kMessGatesTuGlied. Diese Zeile ist die Wache gegen genau den ODR-Fehler, der beim Bau naheliegt.
    static_assert(glieder[abi::kAnatomyFingerprintMessGatesGlied].empty(),
                  "R-3: der Default des Mess-Gates-Glieds MUSS die leere Identitaet sein (ODR: der TU-Wert "
                  "haette in einem Default-Argument einer inline-Funktion externe Bindung).");
    static_assert(abi::kAnatomyFingerprintFormat == std::string_view{"fingerprint_format=4"},
                  "R-3: der Format-Bump 3 -> 4 ist der Anker dieses Fensters -- er trennt den Alt-Bestand "
                  "deterministisch vom 9-Glieder-Layout (O-2/C-2 hat zuvor 2 -> 3 fuer das 8-Glieder-Layout "
                  "getan; die Begruendung ist dieselbe: Layout-Evolution mismatcht, statt still zu kollidieren).");
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
    constexpr std::string_view kX = "search_algo=k_ary@1.0.0.c";

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
    constexpr auto d = abi::anatomy_fingerprint_hex("achse=algo@1.0.0.c;", "target_isa=code@1.0.0.c", "");
    constexpr auto e = abi::anatomy_fingerprint_hex("achse=algo@1.0.0.c", ";target_isa=code@1.0.0.c", "");
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
    constexpr std::string_view kY = "opt=O3{-O3}@1.0.0.c";
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
        abi::anatomy_fingerprint_hex("ORGAN", "SYSTEM", "MESS", abi::ToolchainGlied{"tc=1;opt=O3{-O3}@1.0.0.c"});
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
//       gleich in der END-Form eingefroren (heutige Achsen + Owner-Q3-Flag "@1.0.0.c"), damit die
//       Literal-Migration in C4 KEINEN zweiten Neuanker im selben Fenster erzeugt.
// Der neue Hex wurde NICHT vorausberechnet, sondern aus dem literalen Compiler-/Testlauf uebernommen.
TEST(MW12StampBausteine, FrozenFingerprintTestVectorForLagerGateB3) {
    namespace abi                       = ::comdare::cache_engine::abi;
    constexpr std::string_view kOrgan   = "search_algo=k_ary@1.0.0.c;path_compression=path_compression_none@1.0.0.c";
    constexpr std::string_view kSystem  = "target_isa=code@1.0.0.c;operating_system=code@1.0.0.c;"
                                          "external_utils=code@1.0.0.c;[simd=code@1.0.0.c]";
    constexpr std::string_view kMeasure = "measurement_tooling=wallclock@1.0.0.c;[load_framework=ycsb@1.0.0.c]";
    // NB/CX-4 END-FORM: die beiden injizierten Glieder sind BELEGT (Literale, s. Kopf). Die Werte sind in
    // der realen Renderer-Form gehalten -- Toolchain-Glied wie render_toolchain_stamp_glied es baut,
    // bvset-Glied wie variant_set_signature es baut -- damit der Anker den ECHTEN Preimage-Bau abdeckt.
    constexpr std::string_view kFrozenToolchain =
        "tc=1;cxx=gcc-16.2.0@1.0.0.c;opt=O3{-O3}@1.0.0.c;ext=avx512;ceb=8.0;gate=avx512;atomic128=cx16{-mcx16}@1.0.0.c";
    constexpr std::string_view kFrozenBvset = "bvset=1;bv=2;page_type[{bplus;hw_cache_line=64;hw_numa_capable=0}];"
                                              "simd_extension[{avx512}];"
                                              "general_hardware[{x86_64;hw_cache_line=64;hw_numa_capable=0}]";
    // E-E END-FORM (07.08.2026): das Overlay-Glied [7] wird ab hier EXPLIZIT und als LITERAL gereicht.
    //
    // WARUM ES UEBERHAUPT HIER AUFTAUCHT: bis zur Scharfschaltung war sein Default LEER, der Vektor rechnete
    // also implizit ueber ein leeres Glied. Seit E-E traegt der Default den QUELL-HASH DIESES BAUMS -- und
    // damit haette dieser Anker sich bei JEDER Aenderung an irgendeiner Achsen-Quelle bewegt. Ein
    // eingefrorener Testvektor, der bei jedem Commit wandert, ist kein Anker, sondern ein Dauerrot; er
    // wuerde als erstes weggeworfen. Dieselbe Ueberlegung wie bei NB/CX-4, wo Toolchain- und bvset-Glied aus
    // genau diesem Grund LITERALE statt Live-Werte bekamen ("die Live-Werte haengen an der uebersetzenden
    // Toolchain und an der Enable-Menge der Maschine"). Hier ist die Abhaengigkeit noch schaerfer: sie ist
    // der Quelltext selbst.
    //
    // WARUM EIN BELEGTES LITERAL UND NICHT "": weil ein Anker, der genau den NEUEN Teil des Preimage nicht
    // abdeckt, ein gruener Test waere, der die alte Ordnung zementiert (Fixture-END-Form-Lehre, s. Kopf).
    // Der Wert ist bewusst FREMD zur Maschine und damit stabil; er ist nachrechenbar:
    //   printf 'comdare-overlay-fixture' | sha512sum
    // Die WIRKSAMKEIT des LIVE-Werts beweist stattdessen EeOverlayGliedStehtLiveImPreimage (unten) --
    // dieselbe Arbeitsteilung wie zwischen diesem Vektor und NbCx4LiveGliederStehenImPreimage.
    constexpr std::string_view kFrozenOverlay = "84250c96ec21228119ca6607154fa450e8ffdbc1ae3074be9d8bcf7599198593"
                                                "19f52c64502a78a13da5cb76d05e4988e4c7be0d72fa70935f4a2e5bba5f47ec";
    // EINGEFROREN (Sync mit Lane-B B3): 128-hex SHA-512 ueber die '\n'-getrennte Glied-Folge. NIE aendern.
    //
    // NEU-ANKER ALS DEKLARIERTES BYTE-EREIGNIS, NICHT ALS STILLER FIX.
    // "NIE aendern" heisst nicht "nie", sondern "nie NEBENBEI". Bisher zweimal bewegt:
    //     Format 2 (O-2/C-2): 17148e5a4d0f4a2d... b7fd37fbba76414b... (128 hex)
    //     Format 3 -> 4 (R-3, 07.08.2026, das neunte Preimage-Glied abi/mess_gates_glied.hpp):
    //                        5b18feacb6c7295e... 9cbcfeb89b0fba48... (128 hex)
    //
    // FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026) -- DIE DRITTE, HIER VOLLZOGENE BEWEGUNG. Sie kommt NICHT
    // von einem Format-Bump, sondern von den ZEILEN selbst: die Versions-Schreibweise wandert von "@1.0.0c"
    // (Q3) auf "@1.0.0.c" (v2, Punkt vor jedem Flag). Damit aendert sich das Preimage in den Gliedern [1],
    // [2] und [3] -- und der Fingerprint MUSS wandern. Genau das ist der Zweck: der Owner hat die
    // Cache-Invalidierung ausdruecklich gewollt ("ich moechte die alten Wege komplett ersetzen"), und ein
    // Fingerprint, der ueber eine Grammatik-Aenderung hinweg GLEICH bliebe, waere die Katastrophe -- er
    // wuerde alte Binaries als aktuell ausweisen.
    // Der neue Wert ist NICHT vorausberechnet, sondern aus dem literalen Lauf einer Probe-TU gegen genau
    // diesen Header uebernommen (dieselbe Methode wie bei den beiden vorigen Ankern).
    //
    // E-E (07.08.2026) -- DIE VIERTE BEWEGUNG, und die letzte, die das Overlay-Glied verursacht. Sie kommt
    // NICHT von einem Format-Bump (das Layout bleibt fingerprint_format=4, neun Glieder): sie kommt davon,
    // dass Glied [7] von LEER auf BELEGT wechselt. Ab hier ist der Anker gegen einen LITERALEN Overlay-Wert
    // gepinnt und bewegt sich deshalb NICHT mehr mit dem Quelltext -- diese Bewegung ist einmalig.
    //   Vorgaenger (Flag-Grammatik v2, leeres Overlay-Glied): 88f59b9b0da85e34...96125688
    // WARUM KEIN FORMAT-BUMP NOETIG IST: anders als bei R-3 droht kein STILLER Skip, weil es keinen Bestand
    // gibt -- der .fingerprint-Sidecar-Bestand ist literal 0 (am 07.08. ueber den gesamten Arbeitsbaum inkl.
    // Build-Verzeichnisse nachgezaehlt). Jeder Fingerprint bewegt sich ohnehin einmal, und zwar fail-closed
    // (Neubau), nicht fail-open (falscher Skip).
    constexpr std::string_view kFrozenFingerprintV1 =
        "d53aebdbb22902f3cdbbf5947bc36ea5ba04808248fc23fa99a1b95471edda7c"
        "f0f13be791ced93d2ded7b4906a1c5c4c2123b28322cc38378b06250f20b4d84";
    constexpr auto fp = abi::anatomy_fingerprint_hex(kOrgan, kSystem, kMeasure, abi::ToolchainGlied{kFrozenToolchain},
                                                     abi::BvsetGlied{kFrozenBvset}, abi::OverlayHash{kFrozenOverlay});
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
    EXPECT_NE(g.find("opt=O3{-O3}@1.0.0.c"), std::string::npos) << "glied='" << g << "'";
    EXPECT_NE(g.find("atomic128=cx16{-mcx16}@1.0.0.c"), std::string::npos) << "glied='" << g << "'";
    // (b) REAL erkannte Compiler-Version am Dialekt, mit Achsen-Version -- und NB2-1: dahinter, durch ':'
    //     getrennt, der TIER-TREIBER-TAG. Er ERSETZT die Version nicht (das war G-C4s Punkt), er tritt
    //     NEBEN sie: ohne ihn kollabierten g++-17 und g++-18 auf dasselbe Glied.
    EXPECT_NE(g.find("cxx=gcc-16.1.0:g++-16@1.0.0.c"), std::string::npos) << "glied='" << g << "'";
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
        EXPECT_EQ(e.version, std::string_view{"1.0.0.c"});
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
    EXPECT_NE(glied.find(";opt=O3{-O3}@1.0.0.c"), std::string::npos) << "glied='" << glied << "'";
    // NB2-1: der TREIBER-TAG steht jetzt in BEIDEN Welten und kommt aus DERSELBEN Quelle (sp.cxx) -- das
    // Glied kann also gar keinen anderen Treiber nennen als den, unter dem gebaut wird. Die verbleibende
    // Asymmetrie ist nur noch die Realversion: sie steht im Glied und hat im Suffix kein Gegenstueck.
    EXPECT_NE(suffix.find("+cxx=g++-16"), std::string::npos);
    EXPECT_NE(glied.find("cxx=gcc-16.1.0:g++-16@"), std::string::npos) << "glied='" << glied << "'";
}

TEST(MW12StampBausteine, PlannerVersionStampCarriesSelfVersionAndIsaOs) {
    // CX-W5 (Codex-Doppelreview 02.08.2026): das ROH-Literal traegt jetzt das 'v' (Owner-Q10) -- frueher stand
    // hier "1.0.0" und ZEMENTIERTE die alte, Q10-widrige Form. Die GERENDERTE Zeile bleibt praefixfrei
    // "planner@1.0.0.c" (A13-M3/C4: das 'v' faellt beim Rendern weg, das HW-Flag bleibt).
    EXPECT_EQ(pl::kPlannerVersion, std::string_view{"1.0.0.c"}); // Roh-Literal mit 'v' (Owner-Q10)
    EXPECT_EQ(pl::planner_target_isa(), std::string_view{"x86_64"});
    std::string const stamp = pl::planner_version_stamp();
    EXPECT_NE(stamp.find("planner@1.0.0.c"), std::string::npos) << "stamp='" << stamp << "'"; // render byte-identisch
    EXPECT_EQ(stamp.find("planner@v"), std::string::npos) << "gerenderte Form traegt KEIN 'v' (Owner-Q10)";
    EXPECT_NE(stamp.find("isa=x86_64"), std::string::npos) << "stamp='" << stamp << "'";
    EXPECT_NE(stamp.find("os="), std::string::npos) << "stamp='" << stamp << "'";
}

// A2 (G2-4 Schritt 3+4): System-Achsen-Code-Versionen + Mess-Tooling-Version aus Single-Sources statt Hartkodierung.
// Bei Anlage render-neutral; seit A13-M3/C4 tragen die gueltigen ids/Achsen das CPU-Flag ("1.0.0c",
// deklariertes Byte-Ereignis). "0.0.0"-Sentinel weiterhin nur fuer ungueltige Tooling-ids (A13-M1b).
TEST(MW12StampBausteine, A2SystemAndToolingCodeVersionsSingleSource) {
    namespace abi = ::comdare::cache_engine::abi;
    // (a) System-Achsen-Single-Source: DREI Achsen (O-8 Schritt 4, A3-Kern). A13-M3/C4: alle drei tragen
    // seit dem Migrations-Commit das CPU-Flag ("1.0.0.c", Owner-Q3) -- die Version selbst ist NICHT gebumpt.
    EXPECT_EQ(abi::kSystemAxisCodeCount, std::size_t{3});
    for (auto const& e : abi::kSystemAxisCodeVersions) {
        EXPECT_FALSE(e.axis.empty());
        EXPECT_EQ(e.version, std::string_view{"1.0.0.c"});
    }
    // Der frueher hier stehende "Neutralitaets-Anker" (byte-identisch zur alten v1-Hartkodierung) ist mit dem
    // A3-Rueckbau gegenstandslos: die Zeile SOLL sich geaendert haben. Sie ist aus dem Werkzeug-Output des
    // Fenster-Laufs neu geankert und bindet jetzt die Drei-Achsen-Ordnung.
    // A13-M2 (Owner-E2 + Q1): NEU GEANKERT -- hinter den drei Haupt-Achsen steht jetzt der Meta-Meta-
    // KLAMMER-ANHANG "[simd=code@1.0.0.c]". Das ist der beabsichtigte Byte-Wechsel der System-Zeile (und damit
    // des Fingerprints ALLER kuenftigen Binaries), nicht eine Drift: vor Voll-Bau-4 existiert kein
    // schuetzenswerter Bestand, das eine Neuanker-Fenster ist genau hier.
    EXPECT_EQ(abi::system_stamp_line(), std::string{"target_isa=code@1.0.0.c;operating_system=code@1.0.0.c;"
                                                    "external_utils=code@1.0.0.c;[simd=code@1.0.0.c]"});

    // (b) Mess-Tooling-Version-Feld + id-Lookup.
    for (auto const& t : m::kMeasurementToolingRegistry) EXPECT_EQ(t.version, std::string_view{"1.0.0.c"});
    EXPECT_EQ(m::tooling_version_for_id("wallclock"), std::string_view{"1.0.0.c"});
    EXPECT_EQ(m::tooling_version_for_id("macro"), std::string_view{"1.0.0.c"});
    EXPECT_EQ(m::tooling_version_for_id("micro"), std::string_view{"1.0.0.c"});
    // A13-M1b (Owner-Q3, dreistellig): der Sentinel-Rueckgabewert ist "0.0.0" statt der Kurzform "v0" --
    // byte-neutral, beide rendern "0.0.0" (Beleg im Render-Block (c) unten: "@0.0.0" unveraendert).
    EXPECT_EQ(m::tooling_version_for_id("bogus"), std::string_view{"0.0.0"}); // unbekannt -> Sentinel

    // (c) Sentinel-Render: ungueltige Tooling-id -> @0.0.0 (flaglos, der Sentinel traegt nie ein Flag);
    //     gueltige rendern seit C4 @1.0.0.c.
    // A13-M2: der load_framework-Anhang steht geklammert AM ENDE -- der Sentinel betrifft nur das Tooling-Glied.
    EXPECT_EQ(abi::measurement_stamp_line("bogus"),
              std::string{"measurement_tooling=bogus@0.0.0;[load_framework=ycsb@1.0.0.c]"});
    EXPECT_EQ(abi::measurement_stamp_line("wallclock"),
              std::string{"measurement_tooling=wallclock@1.0.0.c;[load_framework=ycsb@1.0.0.c]"});
}

// A3 (G2-1a): Entry-POD AnatomyStampEntryV1 (48B-Pin) + consteval count/parse_stamp_entries + parse_dotted_semver.
// Reine Parser-/POD-Vorstufe (POD waechst erst in A4); tokenisiert die gerenderten "achse=algo@X.Y.Z"-Zeilen.
TEST(MW12StampBausteine, A3AnatomyStampEntryPodAndConstevalParser) {
    namespace abi = ::comdare::cache_engine::abi;
    // (a) Entry-POD sizeof/align-Pin.
    EXPECT_EQ(sizeof(abi::AnatomyStampEntryV1), std::size_t{48});
    EXPECT_EQ(alignof(abi::AnatomyStampEntryV1), std::size_t{8});
    static_assert(sizeof(abi::AnatomyStampEntryV1) == 48);

    // (b) parse_algo_semver = Umkehrung von algo_semver_string. FLAG-GRAMMATIK v2: hier stand bis zum
    //     Owner-KERN vom 07.08.2026 parse_dotted_semver -- ein ZWEITER Parser fuer die gerenderte Form, die
    //     sich von der rohen nur um das 'v'-Praefix unterschied. Mit dem Wegfall des 'v' sind beide Formen
    //     zeichengleich; der zweite Parser ist ersatzlos entfallen, damit die Grammatik nicht zweimal
    //     existiert (genau die Drift-Quelle, gegen die algo_semver.hpp seit A13-M1b argumentiert).
    EXPECT_EQ(m::parse_algo_semver("1.0.0"), (m::AlgoSemVer{1, 0, 0}));
    EXPECT_EQ(m::parse_algo_semver("2.3.4"), (m::AlgoSemVer{2, 3, 4}));
    EXPECT_EQ(m::parse_algo_semver("v1.0.0"), (m::AlgoSemVer{0, 0, 0})); // die Alt-Form -> Sentinel
    EXPECT_EQ(m::parse_algo_semver("1.0"), (m::AlgoSemVer{0, 0, 0}));    // Kurzform -> Sentinel

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

// FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026): die Flag-LISTE reist als HASH in den reserved-Bits 6-31 des
// 48-Byte-Entry-PODs (Owner-Entscheid F-3: "Hash der normalisierten Flag-Zeichenkette, nicht als Bitmaske
// ueber einen festen Katalog"). Kein sizeof-/Layout-Bruch. Hierarchische Namen "prt-art.memory.abc@..."
// bleiben reiner NAMENS-Anteil -- und das gilt jetzt GEGEN eine Versions-Grammatik, die den Punkt auch
// RECHTS vom '@' als Trenner kennt.
//
// WAS HIER VORHER STAND UND WARUM ES NICHT BLEIBEN KONNTE: die A13-M1b-Batterie pinnte Bit 0 als
// "experimental" und die Bits 1-2 als 4-Werte-Hardware-Code. Beide Konzepte gibt es nicht mehr ('e' ist
// EFFICIENCY CORE, die Hardware-Zielrichtung ist eine Liste). Ein Umschreiben der Literale haette den Test
// gruen gelassen und dabei eine Bit-Belegung bezeugt, die entfallen ist.
TEST(MW12StampBausteine, StampEntryTraegtFlagHashUndToleranteNamen) {
    namespace abi = ::comdare::cache_engine::abi;
    // (a) Entry-POD-Groesse UNVERAENDERT -- die Flag-Liste nutzt das vorgesehene reserved-Feld.
    static_assert(sizeof(abi::AnatomyStampEntryV1) == 48);
    EXPECT_EQ(abi::kStampEntryFlagsHashShift, std::uint32_t{6});
    EXPECT_EQ(abi::kStampEntryFlagsHashBits, std::uint32_t{26});
    EXPECT_EQ(abi::kStampEntryReservedFreeMask, std::uint32_t{0x7});
    // Die drei Bit-Gruppen ueberlappen NICHT und decken zusammen das ganze Wort ab (kein unbenanntes Loch).
    EXPECT_EQ(abi::kStampEntryReservedFreeMask & abi::kStampEntryMetaLevelMask, std::uint32_t{0});
    EXPECT_EQ(abi::kStampEntryReservedFreeMask & abi::kStampEntryFlagsHashMask, std::uint32_t{0});
    EXPECT_EQ(abi::kStampEntryMetaLevelMask & abi::kStampEntryFlagsHashMask, std::uint32_t{0});
    EXPECT_EQ(abi::kStampEntryReservedFreeMask | abi::kStampEntryMetaLevelMask | abi::kStampEntryFlagsHashMask,
              std::uint32_t{0xFFFFFFFF});

    // (b) Owner-Q2-Namens-Toleranz: Punkte im Namens-Anteil VOR dem '@' sind Namens-Bestandteil (Achse UND
    //     Algorithmus), Punkte NACH dem '@' gehoeren der Versions-Grammatik. Der Trenner ist das '@'.
    static constexpr char kLit[] = "prt-art.memory.abc=prt_patricia.simd@2.3.4.c{p.e};filter=bloom@1.0.0";
    constexpr auto        e      = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kLit})>(kLit);
    static_assert(e.size() == 2);
    EXPECT_EQ(std::string_view(e[0].axis, e[0].axis_len), std::string_view{"prt-art.memory.abc"});
    EXPECT_EQ(std::string_view(e[0].algorithm, e[0].algo_len), std::string_view{"prt_patricia.simd"});
    EXPECT_EQ(e[0].x, 2u);
    EXPECT_EQ(e[0].y, 3u);
    EXPECT_EQ(e[0].z, 4u);
    static_assert(std::string_view(e[0].axis, e[0].axis_len) == "prt-art.memory.abc");

    // (c) Der Flag-Hash steht in den Bits 6-31; die flaglose Form laesst reserved exakt 0 (so wie der
    //     gesamte Bestand vor der v2 dastand). "Keine Flags" ist VERLAESSLICH von "irgendwelche Flags" zu
    //     unterscheiden -- der Leer-Hash ist per Konstruktion die 0 und von keiner nicht-leeren Liste
    //     erreichbar (sonst waere die Unterscheidung nur WAHRSCHEINLICH richtig).
    EXPECT_TRUE(abi::stamp_entry_has_flags(e[0]));
    EXPECT_EQ(abi::stamp_entry_flags_hash(e[0]),
              abi::stamp_entry_flags_hash_of(m::parse_algo_semver("2.3.4.c{p.e}").flags));
    EXPECT_EQ(e[0].reserved & abi::kStampEntryReservedFreeMask, std::uint32_t{0});
    EXPECT_FALSE(abi::stamp_entry_has_flags(e[1]));
    EXPECT_EQ(e[1].reserved, std::uint32_t{0});
    static_assert(abi::stamp_entry_has_flags(e[0]));
    static_assert(!abi::stamp_entry_has_flags(e[1]));

    // (c2) VERSCHIEDENE Flag-Listen belegen das reserved-Feld VERSCHIEDEN -- sonst waeren zwei
    //      Hardware-Staende im POD ununterscheidbar (Lager-Key-Kollision). Geprueft werden ALLE Paare, nicht
    //      nur die Nachbarn: die alte Nachbar-Pruefung haette eine Kollision zwischen 'c' und 'gfni' nicht
    //      gesehen.
    static constexpr char kHw[] =
        "a=x@1.0.0.c;b=y@1.0.0.g;c=z@1.0.0.f;d=w@1.0.0.n;e=v@1.0.0.c{p.e};f=u@1.0.0.c{e};g=t@1.0.0.gfni";
    constexpr auto h = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kHw})>(kHw);
    static_assert(h.size() == 7);
    for (std::size_t i = 0; i < h.size(); ++i) {
        EXPECT_TRUE(abi::stamp_entry_has_flags(h[i])) << "Eintrag " << i << " traegt Flags, meldet aber keine.";
        EXPECT_NE(h[i].reserved, e[1].reserved) << "Eintrag " << i << " sieht aus wie die FLAGLOSE Form.";
        for (std::size_t j = i + 1; j < h.size(); ++j)
            EXPECT_NE(h[i].reserved, h[j].reserved) << "Kollision zwischen Eintrag " << i << " und " << j;
    }
    // 'gfni' ist NICHT 'g' -- der mehrzeichige Token bleibt auch im POD ein eigener Stand.
    EXPECT_NE(h[6].reserved, h[1].reserved);

    // (c3) Die ALTE Q3-Schreibweise bricht HART im consteval-Pfad, statt still auf @0.0.0 zu kollabieren.
    //      Hier stand bis zur v2 "a=x@1.0.0cg" (zweites Hardware-Flag) -- das ist heute ein Token wie jedes
    //      andere und faellt nur noch am FEHLENDEN PUNKT durch. Die Probe steht deshalb auf der Regel, die
    //      die v2 wirklich durchsetzt.
    static_assert(!abi::stamp_line_is_parsable<"a=x@1.0.0c">, "R2: Flag ohne fuehrenden Punkt (Alt-Form).");
    static_assert(!abi::stamp_line_is_parsable<"a=x@v1.0.0.c">, "R1: kein 'v'-Praefix mehr.");
    static_assert(!abi::stamp_line_is_parsable<"a=x@1.0.0.c{.p}">, "R4: hinter '{' kein fuehrender Punkt.");
    static_assert(!abi::stamp_line_is_parsable<"a=x@1.0.0.c{}">, "R4: keine leere Gruppe.");
    static_assert(!abi::stamp_line_is_parsable<"a=x@1.0.0.c{p">, "unbalancierte Klammer.");
    static_assert(!abi::stamp_line_is_parsable<"a=x@1.0.0.{p}">, "Klammer ohne Basis.");
    EXPECT_FALSE((abi::stamp_line_is_parsable<"a=x@1.0.0c">));
    EXPECT_FALSE((abi::stamp_line_is_parsable<"a=x@1.0.0.c{.p}">));
    // ... und die POSITIVE Gegenprobe: die kanonischen Formen gehen durch.
    static_assert(abi::stamp_line_is_parsable<"a=x@1.0.0.c">);
    static_assert(abi::stamp_line_is_parsable<"a=x@1.0.0.c{p.e}.x512{f.vl}.gfni">);
    static_assert(abi::stamp_line_is_parsable<"a=x@1.0.0.c;[b=y@1.0.0.c{p}]">,
                  "Komposit-Klammer INNERHALB eines Meta-Meta-Anhangs: beide Klammer-Ebenen zugleich.");

    // ... und die POSITIVE Restaussage bleibt: der Parser raet NIE ein Flag herbei -- das dokumentierte
    //     Sentinel-Rendering "@0.0.0" ist zulaessig UND flaglos.
    static constexpr char kSentinel[] = "a=x@0.0.0";
    constexpr auto sen = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kSentinel})>(kSentinel);
    EXPECT_EQ(sen[0].x, 0u);
    EXPECT_EQ(sen[0].reserved, std::uint32_t{0});
    EXPECT_FALSE(abi::stamp_entry_has_flags(sen[0]));

    // (d) Eine Zeile OHNE Flags setzt in KEINEM Eintrag ein Bit.
    static constexpr char kPlain[] =
        "search_algo=k_ary@1.0.0;filter=bloom@2.3.4;target_isa=code@1.0.0"; // FLAGLOSE Probe, kein Bestand
    constexpr auto p = abi::parse_stamp_entries<abi::count_stamp_entries(std::string_view{kPlain})>(kPlain);
    for (auto const& x : p) EXPECT_EQ(x.reserved, std::uint32_t{0});

    // (e) Die Struktur-Fehlformen ohne '=' bzw. ohne '@' brechen weiterhin benannt, ebenso die Kurzform.
    static_assert(!abi::stamp_line_is_parsable<"achse=algo@1.0.c">, "Kurzform mit Flag-Schwanz bricht hart.");
    static_assert(!abi::stamp_line_is_parsable<"achse=algo">, "Segment ohne '@' bricht hart.");
    static_assert(!abi::stamp_line_is_parsable<"nur_ein_name">, "Segment ohne '=' bricht hart.");
    EXPECT_FALSE((abi::stamp_line_is_parsable<"achse=algo@1.0.c">));
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
        "search_algo=k_ary@1.0.0.c;cache_traversal=t@1.0.0.c;mapping=m@1.0.0.c;path_compression=p@2.3.4.c;"
        "node_type=n@1.0.0.c;memory_layout=l@1.0.0.c;allocator=a@1.0.0.c;prefetch=pf@1.0.0.c;concurrency=c@1.0.0.c;"
        "serialization=s@1.0.0.c;value_handle=v@1.0.0.c;index_organization=i@1.0.0.c;io_dispatch=io@1.0.0.c;"
        "migration_policy=mp@1.0.0.c;filter=f@1.0.0.c;queuing_q1=q1@1.0.0.c;queuing_q2=q2@1.0.0.c;"
        "persistence_target=pt@1.0.0.c"; // 18 Haupt-Achsen (A8.2), A13-M3/C4-Flag-Form
    // A13-M2: BEIDE Nicht-Organ-Fixtures sind auf die Klammer-Welt nachgezogen (Owner-E2/Q1) -- System-Zeile
    // 3 -> 4 Eintraege (Meta-Meta-Anhang), Mess-Zeile mit load_framework GEKLAMMERT AM ENDE statt vorne. Ein
    // Fixture, der die alte Ordnung stehen laesst, liest sich fuer den Naechsten wie eine gueltige Referenz.
    static constexpr char kSystem[]  = "target_isa=code@1.0.0.c;operating_system=code@1.0.0.c;"
                                       "external_utils=code@1.0.0.c;[simd=code@1.0.0.c]"; // 3 + 1 Meta-Meta (A13-M2)
    static constexpr char kMeasure[] = "measurement_tooling=wallclock@1.0.0.c;measurement_tooling=macro@1.0.0.c;"
                                       "measurement_tooling=micro@1.0.0.c;[load_framework=ycsb@1.0.0.c]"; // 3 + 1

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
    // die Zeile zurueck. Das ist der eigentliche Beweis der Grammatik: haette der Renderer eine andere
    // Trenner-Regel als der Parser, faellt genau hier die Gleichheit.
    //
    // EHRLICHE GRENZE DER FLAG-GRAMMATIK v2 -- SIE GEHOERT AN DIESE STELLE, WEIL SIE HIER SICHTBAR WIRD:
    // die Rekonstruktion umfasst den FLAG-SCHWANZ NICHT MEHR. Owner-Entscheid F-3 kodiert die Flag-Liste als
    // HASH, und ein Hash ist einwegig. Der Vergleich unten laeuft deshalb gegen die Zeile OHNE Flag-Schwanz,
    // und die Treue des Schwanzes wird SEPARAT geprueft (hash_stimmt, direkt darunter) -- nicht behauptet.
    // WARUM DAS VERTRETBAR IST: die Rekonstruktions-Zusage aus Owner-Q1 (LEDGER, bindend) gilt der
    // KLAMMER-Grammatik -- sie ist der Grund, warum die geklebten Gruppen-Formen hart brechen. Der
    // Versions-Anteil war schon VOR der v2 nicht verlustfrei: der alte POD kodierte 'c' und "kein Flag" auf
    // denselben Wert 0 (der Kommentar an dieser Stelle sagte das selbst). Neu ist nicht der Verlust, neu ist
    // sein Umfang -- und dass er hier gemessen statt weggeschrieben wird.
    auto const ohne_flag_schwanz = [](std::string_view zeile) {
        std::string out;
        std::size_t i            = 0;
        auto const  ist_struktur = [](char c) { return c == ';' || c == '[' || c == ']'; };
        while (i < zeile.size()) {
            char const c = zeile[i++];
            out += c;
            if (c != '@') continue;
            int punkte = 0; // die DREI Zahlen uebernehmen, ab dem dritten Punkt beginnt der Flag-Schwanz
            while (i < zeile.size() && !ist_struktur(zeile[i])) {
                if (zeile[i] == '.' && punkte == 2) break;
                if (zeile[i] == '.') ++punkte;
                out += zeile[i++];
            }
            while (i < zeile.size() && !ist_struktur(zeile[i])) ++i; // Flag-Schwanz verwerfen
        }
        return out;
    };
    // Die Flag-Schwaenze der Zeile in TEXT-Reihenfolge -- Eingabe der Hash-Gegenprobe.
    auto const flag_schwaenze = [](std::string_view zeile) {
        std::vector<std::string> raus;
        std::size_t              i            = 0;
        auto const               ist_struktur = [](char c) { return c == ';' || c == '[' || c == ']'; };
        while (i < zeile.size()) {
            if (zeile[i++] != '@') continue;
            std::size_t const start = i;
            while (i < zeile.size() && !ist_struktur(zeile[i])) ++i;
            raus.emplace_back(zeile.substr(start, i - start));
        }
        return raus;
    };
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
            // Der FLAG-SCHWANZ steht NICHT hier: er reist als Hash (Owner-F-3) und ist aus dem POD nicht
            // rekonstruierbar. Seine Treue prueft die Hash-Gegenprobe unten.
            prev = lvl;
        }
        for (std::uint32_t k = 0; k < prev; ++k) out += ']';
        return out;
    };
    // (1) STRUKTUR-Rekonstruktion: Namen, Ebenen, Klammer-Grammatik und das X.Y.Z-Tripel kommen verbatim
    //     zurueck -- gemessen gegen die Zeile ohne ihre Flag-Schwaenze.
    EXPECT_EQ(join(v.organ_entries, v.organ_entry_count),
              ohne_flag_schwanz(std::string_view(v.organ_line, v.organ_len)));
    EXPECT_EQ(join(v.system_entries, v.system_entry_count),
              ohne_flag_schwanz(std::string_view(v.system_line, v.system_len)));
    EXPECT_EQ(join(v.measurement_entries, v.measurement_entry_count),
              ohne_flag_schwanz(std::string_view(v.measurement_line, v.measurement_len)));
    // (2) HASH-GEGENPROBE: je Eintrag traegt das reserved-Feld GENAU den Hash des Flag-Schwanzes, der an
    //     dieser Stelle in der Zeile steht. Das ist der Ersatz fuer den Teil der Rekonstruktion, den der
    //     Hash nicht leisten kann -- und er ist eine ECHTE Messung an der Zeile, keine Nachrechnung der
    //     Kodierungs-Formel gegen sich selbst.
    auto const hash_stimmt = [&](abi::AnatomyStampEntryV1 const* e, std::uint64_t n, std::string_view zeile,
                                 char const* wer) {
        auto const schwaenze = flag_schwaenze(zeile);
        ASSERT_EQ(schwaenze.size(), static_cast<std::size_t>(n)) << wer << ": Segment-Zahl weicht ab.";
        for (std::uint64_t i = 0; i < n; ++i) {
            auto const erwartet =
                abi::stamp_entry_flags_hash_of(m::parse_algo_semver(schwaenze[static_cast<std::size_t>(i)]).flags);
            EXPECT_EQ(abi::stamp_entry_flags_hash(e[i]), erwartet)
                << wer << ": Flag-Hash von Eintrag " << i << " passt nicht zum Schwanz \""
                << schwaenze[static_cast<std::size_t>(i)] << "\".";
        }
    };
    hash_stimmt(v.organ_entries, v.organ_entry_count, std::string_view(v.organ_line, v.organ_len), "organ");
    hash_stimmt(v.system_entries, v.system_entry_count, std::string_view(v.system_line, v.system_len), "system");
    hash_stimmt(v.measurement_entries, v.measurement_entry_count,
                std::string_view(v.measurement_line, v.measurement_len), "measurement");

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
    //
    // M-1/D-4 (06.08.2026) -- DIESE ZWEI WACHEN SIND GESCHAERFT, NICHT ABGESCHWAECHT.
    // Bis D-4 rendert der CEB-Stempel das ANGEBOT (die volle Registry) und war damit von der
    // einkompilierten Combo UNABHAENGIG; beide Erwartungen unten durften deshalb die Vollmenge nennen.
    // Seit D-4 rendert er die WAHL. Die Verbatim-Erwartung haengt sich daher an die AUSDRUECKLICHE
    // [all]-Spezialisierung (unbedingt gueltig, egal wie dieses Build-Verzeichnis konfiguriert ist),
    // und der Drift-Guard vergleicht gegen den Runtime-Renderer AN DER EINKOMPILIERTEN LEGENDE.
    // Wuerde man die Erwartungen einfach an kCebMeasurementStamp haengen lassen, waeren sie in einem
    // -DCOMDARE_MEASUREMENT_COMBO=<spezifisch> konfigurierten Verzeichnis rot -- und man haette die
    // Wache dann vermutlich gelockert statt sie richtig zu stellen.
    // I-PMC-2: die Verbatim-Erwartung ist der Vor-10.08.-Text, durch die EINE Asymmetrie-Stelle gereicht.
    // In einem Verzeichnis OHNE Vendor-Makro ist das byte-identisch der alte Text -- das ist der
    // Additivitaets-Beweis an genau der Wache, die ihn tragen muss.
    EXPECT_EQ((std::string{ceb::kCebMeasurementStampFor<ceb::CebComboLegend{"[all]"}>}),
              comdare_test_pmc::ceb_erwartung_aus_tier_zeile("measurement_tooling=wallclock@1.0.0.c;"
                                                             "measurement_tooling=macro@1.0.0.c;"
                                                             "measurement_tooling=micro@1.0.0.c;"
                                                             "[load_framework=ycsb@1.0.0.c]"));
    // DRIFT-GUARD: die consteval-CEB-Zeile deckt sich EXAKT mit der Runtime-Tier-Binary-Mengen-Form -> EINE Wahrheit,
    // keine Parallel-Ableitung. [all] und die Vollmengen-Form sind derselbe Text (Section-64-Vollmengen-Provenienz).
    EXPECT_EQ((std::string{ceb::kCebMeasurementStampFor<ceb::CebComboLegend{"[all]"}>}),
              comdare_test_pmc::ceb_erwartung_aus_tier_zeile(abi::measurement_stamp_line_full_set()));
    // DRIFT-GUARD AN DER EINKOMPILIERTEN LEGENDE: das ist die Form, die auch in einem combo-hart
    // konfigurierten Verzeichnis gilt. Sie ist die eigentliche "EINE Wahrheit"-Zusage.
    EXPECT_EQ(std::string{ceb::kCebMeasurementStamp},
              comdare_test_pmc::ceb_erwartung_aus_tier_zeile(
                  abi::measurement_stamp_line_from_combo_legend(ceb::kCebCtComboLegend)));
    // SHA-512-Provenienz: 128 hex, == Host-Nachrechnung via anatomy_fingerprint_hex ("","",mess).
    // A13-M3/K-1: hier stand die 4-arg-Form mit dem merge-"" -- genau der Alt-Aufruf, den die Sperre faengt.
    // Sie hat literal gefeuert ("die merge-ZEILE existiert nicht mehr ... das 4. Argument ist der
    // OverlayHash-TYP"); der Aufruf ist auf die 3-arg-Form gezogen.
    static_assert(ceb::kCebFingerprint.size() == 128);
    EXPECT_EQ(ceb::kCebFingerprint.size(), std::size_t{128});
    // E-E (07.08.2026): die Nachrechnung muss die GLIEDER des Produktions-Aufrufs spiegeln, sonst prueft
    // sie nicht mehr denselben Wert. Der Punkt ist NICHT das Overlay-Glied, sondern die Mess-ZEILE -- und
    // die bleibt der einzige Freiheitsgrad. Seit der Scharfschaltung traegt der DEFAULT des Overlay-Glieds
    // den Quell-Hash dieses Baums; die CEB rechnet aber bewusst OHNE ihn (ceb_version_stamp.hpp: sie ist
    // kein Tier-Binary, ihr Schluessel soll ruhig liegen, damit der Byte-Anker in
    // test_d4_ceb_schluessel_wahl weiter etwas aussagen kann). Stuende hier die 3-arg-Form, verglichen die
    // beiden Wege ab jetzt VERSCHIEDENE Glied-Saetze -- der Test waere rot, ohne dass etwas driftet.
    constexpr auto host =
        abi::anatomy_fingerprint_hex("", "", ceb::kCebMeasurementStamp, abi::ToolchainGlied{abi::kToolchainStampGlied},
                                     abi::BvsetGlied{abi::kBuildVariantSetSignatureGlied}, abi::OverlayHash{""});
    EXPECT_EQ(ceb::kCebFingerprint, std::string_view(host.data(), 128));
    // ceb_version_stamp() traegt beide Teile + die X.Y.Z-Form (keine rohe @v1).
    // D-4: die Vorspann-Erwartung nennt nicht mehr "wallclock" (das ist nur bei [all] das erste Tooling),
    // sondern setzt die EINKOMPILIERTE Zeile ein -- dieselbe Aussage, aber combo-robust.
    std::string const stamp = ceb::ceb_version_stamp();
    EXPECT_NE(stamp.find("ceb-measurement=" + std::string{ceb::kCebMeasurementStamp}), std::string::npos)
        << "stamp=" << stamp;
    // I-PMC-2: die Klammer schliesst unmittelbar vor der SHA-Zeile -- unabhaengig davon, WIE VIELE Glieder
    // sie fuehrt. Der alte Pin nannte den Klammer-INHALT und waere damit bei jeder Meta-Meta-Erweiterung rot
    // geworden, ohne dass etwas driftet; die Aussage, um die es geht, ist die STELLUNG des Anhangs.
    EXPECT_NE(stamp.find("load_framework=ycsb@1.0.0.c"), std::string::npos) << "stamp=" << stamp;
    EXPECT_NE(stamp.find(std::string{abi::kMetaMetaGroupClose} + ";sha512="), std::string::npos) << "stamp=" << stamp;
    EXPECT_NE(stamp.find(";sha512="), std::string::npos);
    EXPECT_EQ(stamp.find("@v1"), std::string::npos);
}

// =====================================================================================================
// PMC ALS META-META-MESS-ACHSE (Owner 10.08.2026) -- REGISTRY, CEB-GLIED, UND DIE TIER-SEITIGE NULL.
// =====================================================================================================

// (1) DIE ZWEI KOMPONENTEN. Der Nenner ist FREMD (T-3): die erwartete id-Menge steht hier als Literal,
//     die Registry-Menge wird dagegen in BEIDE Richtungen geprueft. Ein Test, der die Registry gegen sich
//     selbst prueft, ist gruen, egal was drinsteht.
TEST(MW12StampBausteine, PmcVendorRegistryFuehrtGenauZweiHardwareKomponenten) {
    namespace cm = ::comdare::cache_engine::measurement;
    // OWNER-WORTLAUT (10.08.2026): "JEDE HARDWAREFORM EINER PMC AMD/INTEL IST ZU UNTERSCHEIDEN, ES SIND 2
    // VERSCHIEDENE HARDWARE KOMPONENTEN NICHT EIN PMC."
    std::vector<std::string> const erwartet{"amd", "intel"};
    ASSERT_EQ(cm::kPmcVendorCount, erwartet.size())
        << "die Zahl der Hardware-Komponenten hat sich geaendert -- das ist eine Owner-Frage, keine "
           "Nebenwirkung (eine dritte Form braucht eine eigene Version und eine eigene Erkennung)";

    // Richtung 1: jede erwartete id existiert.
    for (auto const& id : erwartet)
        EXPECT_NE(cm::pmc_vendor_from_id(id), nullptr) << "erwartete Hardwareform fehlt in der Registry: " << id;
    // Richtung 2: keine unerwartete id existiert (sonst waere ein stiller dritter Wert moeglich).
    std::vector<std::string> gefunden;
    cm::for_each_pmc_vendor([&gefunden](auto const& e) { gefunden.emplace_back(e.id); });
    EXPECT_EQ(gefunden, erwartet) << "Registry-Menge != erwartete Menge (Reihenfolge ist Teil der Zusage: "
                                     "Index == Enum-Wert)";

    // Die Versionen laufen ueber DIE Grammatik des Hauses, nicht ueber einen String-Vergleich.
    for (auto const& id : erwartet) {
        auto const* const e = cm::pmc_vendor_from_id(id);
        ASSERT_NE(e, nullptr);
        auto const parsed = cm::parse_algo_semver(e->version);
        EXPECT_FALSE(parsed.is_sentinel())
            << "die Version der Komponente '" << id << "' steht auf dem Sentinel -- sie wuerde still als "
            << "@0.0.0 stempeln";
    }

    // FAIL-CLOSED: eine unbekannte Hardwareform liefert nullptr, nie einen Default.
    EXPECT_EQ(cm::pmc_vendor_from_id("via"), nullptr);
    EXPECT_EQ(cm::pmc_vendor_from_cpuid("CentaurHauls"), nullptr);
    // ... und die GEGENPROBE, dass die Suche ueberhaupt findet:
    EXPECT_NE(cm::pmc_vendor_from_cpuid("AuthenticAMD"), nullptr);
    EXPECT_NE(cm::pmc_vendor_from_cpuid("GenuineIntel"), nullptr);
}

// (2) DAS CEB-GLIED -- und der ADDITIVITAETS-BEWEIS in derselben Wache.
//     OHNE Vendor-Makro MUSS die Zeile byte-identisch zum Vor-10.08.-Stand sein; MIT Vendor-Makro traegt
//     sie GENAU EIN zusaetzliches Glied im BESTEHENDEN Klammer-Anhang. Die erwartete Zeile wird aus dem
//     RUNTIME-Renderer + der Registry KONSTRUIERT (fremder Nenner), nicht aus dem CEB-Header abgeschrieben.
TEST(MW12StampBausteine, PmcGliedImCebStempelUndByteIdentitaetOhnePmc) {
    namespace ceb = ::comdare::cache_engine::builder;
    namespace abi = ::comdare::cache_engine::abi;
    namespace cm  = ::comdare::cache_engine::measurement;

    std::string const gebaut{ceb::kCebMeasurementStampFor<ceb::CebComboLegend{"[all]"}>};

#if COMDARE_CEB_HAT_PMC_GLIED
    // Die Erwartung kommt aus der EINEN Asymmetrie-Stelle (pmc_ceb_asymmetrie.hpp) -- gespeist aus dem
    // RUNTIME-Renderer, nicht aus dem CEB-Header.
    EXPECT_EQ(gebaut, comdare_test_pmc::ceb_erwartung_aus_tier_zeile(abi::measurement_stamp_line_full_set()))
        << "das PMC-Glied steht als ';'-Geschwister HINTER load_framework, INNERHALB derselben Klammer "
           "(Owner-E2 'ans Ende der Kette', RF-7 keine neue Zeile)";
    // ... und dieselbe Sache noch einmal als LITERALE Aussage ueber den Inhalt, damit die Zeile oben nicht
    // die einzige Quelle ihrer eigenen Erwartung ist: der Token steht woertlich da.
    auto const* const v = comdare_test_pmc::eingebauter_vendor();
    ASSERT_NE(v, nullptr);
    EXPECT_NE(gebaut.find(std::string{cm::kPmcStampName} + "=" + std::string{v->id} + "@"), std::string::npos)
        << "gebaut=" << gebaut;
    // Die ZWEITE Hardwareform darf NICHT zugleich dastehen -- eine CEB wird auf genau EINEM Host gebaut.
    auto const& andere = v->vendor == cm::PmcVendor::Amd ? cm::pmc_vendor_info(cm::PmcVendor::Intel)
                                                         : cm::pmc_vendor_info(cm::PmcVendor::Amd);
    EXPECT_EQ(gebaut.find(std::string{cm::kPmcStampName} + "=" + std::string{andere.id} + "@"), std::string::npos)
        << "gebaut=" << gebaut;
#else
    // ADDITIVITAETS-BEWEIS: OHNE Vendor-Makro rendert der Header BYTE FUER BYTE den Vor-10.08.-Stand.
    // Diese Bytes sind dieselben, die der A5-Test oben pinnt -- der gesamte Nicht-PMC-Bestand behaelt
    // seinen ceb_key_sha512, es faellt keine Migration und keine Neumessung an.
    EXPECT_EQ(gebaut, std::string{"measurement_tooling=wallclock@1.0.0.c;"
                                  "measurement_tooling=macro@1.0.0.c;"
                                  "measurement_tooling=micro@1.0.0.c;"
                                  "[load_framework=ycsb@1.0.0.c]"});
    EXPECT_EQ(gebaut.find("pmc="), std::string::npos)
        << "ohne Vendor-Makro darf KEIN pmc-Glied entstehen -- sonst waere der Bestand still entwertet";
    // GEGENPROBE, dass die Suche findet: das load_framework-Glied IST da (Trefferzahl 1).
    EXPECT_NE(gebaut.find("load_framework="), std::string::npos);
    EXPECT_EQ(comdare_test_pmc::eingebauter_vendor(), nullptr);
#endif
}

// (3) T-6 SCHWESTERPFLICHT -- DIE GRENZE, DIE NICHT UEBERSCHRITTEN WERDEN DARF.
//     OWNER-WORTLAUT (10.08.2026, verbatim): "binary_id identifiziert das TIER-Binary; die Varianten sind
//     CEB-Binaries. Eine aus-/eingebaute Messeinrichtung erzeugt eine andere CEB, NICHT ein anderes
//     Tier-Binary. Wer beides gleichsetzt, kommt auf einen ABI-Bump, der nicht anfaellt."
//     Diese Wache haelt genau das: die TIER-seitigen Renderer fuehren in KEINER Konfiguration ein
//     pmc-Glied. Waere es dort, kippten emittierte Tier-Quellen, Tier-SHA512, golden-CRC und binary_id --
//     524.288 Phantom-Binaries verdoppelt, ohne dass sich eine einzige .so aendert.
//     AUSSAGE STATT ANWESENHEIT (T-2): die 0 kommt mit einer Gegenprobe, die 1 findet.
TEST(MW12StampBausteine, TierSeiteFuehrtNiemalsEinPmcGlied) {
    namespace abi = ::comdare::cache_engine::abi;

    std::vector<std::pair<char const*, std::string>> const tier_zeilen{
        {"measurement_stamp_line_full_set()", abi::measurement_stamp_line_full_set()},
        {"measurement_stamp_line_from_combo_legend(\"[wallclock]\")",
         abi::measurement_stamp_line_from_combo_legend("[wallclock]")},
        {"measurement_stamp_line_from_combo_legend(\"[all]\")", abi::measurement_stamp_line_from_combo_legend("[all]")},
        {"measurement_stamp_line(\"micro\")", abi::measurement_stamp_line(std::string_view{"micro"})},
        {"measurement_meta_meta_suffix()", abi::measurement_meta_meta_suffix()},
    };

    std::size_t pmc_treffer = 0;
    std::size_t lf_treffer  = 0; // GEGENPROBE-Nenner: dieselbe Suchart findet das load_framework-Glied
    for (auto const& [wo, zeile] : tier_zeilen) {
        if (zeile.find("pmc=") != std::string::npos) {
            ++pmc_treffer;
            ADD_FAILURE() << wo << " fuehrt ein pmc-Glied: " << zeile
                          << "\n  Das verschiebt den TIER-Fingerprint und erzwingt einen ABI-Bump, der nicht "
                             "anfaellt (Owner 10.08.2026). Das Glied gehoert AUSSCHLIESSLICH in "
                             "builder/ceb_version_stamp.hpp.";
        }
        if (zeile.find("load_framework=") != std::string::npos) ++lf_treffer;
    }
    EXPECT_EQ(pmc_treffer, 0u) << "TIER-seitige pmc-Glieder gefunden";
    // Die 0 oben ist nur dann eine Aussage, wenn dieselbe Suche ueberhaupt etwas findet.
    EXPECT_EQ(lf_treffer, tier_zeilen.size())
        << "GEGENPROBE LEER: von " << tier_zeilen.size() << " Tier-Zeilen fuehren nur " << lf_treffer
        << " ein load_framework-Glied. Dann sagt die pmc-Null NICHTS -- die Suche greift ins Leere.";
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
    p.cxx_driver               = "hausgemachter-treiber";
    std::string const g        = abi::render_toolchain_stamp_glied(p);
    std::string const erwartet = "cxx=" + std::string{abi::kDetectedCompilerDialect} + "-" +
                                 std::string{abi::kDetectedCompilerRealVersion} + ":hausgemachter-treiber@1.0.0.c";
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
    //     "cxx=gcc-13.2.0@1.0.0.c". Deshalb traegt der Dialekt zusaetzlich kein '-'.
    abi::ToolchainStampParts e{};
    e.cxx_dialect = "gcc-13.2.0";
    e.cxx_driver  = "g++-13";
    abi::ToolchainStampParts f{};
    f.cxx_dialect     = "gcc";
    f.cxx_realversion = "13.2.0";
    f.cxx_driver      = "g++-13";
    EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(e), std::string_view{"cxx_dialect"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(e), std::invalid_argument);
    EXPECT_EQ(abi::render_toolchain_stamp_glied(f), std::string{"tc=1;cxx=gcc-13.2.0:g++-13@1.0.0.c"});

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
    //     beide "tc=1;cxx=gcc@1.0.0.c" -- also denselben Fingerprint fuer zwei verschiedene Compiler.
    std::string const g17 = glied_fuer("g++-17", {});
    std::string const g18 = glied_fuer("g++-18", {});
    EXPECT_NE(g17, g18) << "g17='" << g17 << "' g18='" << g18 << "'";
    EXPECT_EQ(g17, std::string{"tc=1;cxx=gcc:g++-17@1.0.0.c"});
    EXPECT_EQ(g18, std::string{"tc=1;cxx=gcc:g++-18@1.0.0.c"});

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
    EXPECT_TRUE(::comdare::cache_engine::abi::injizierter_glied_wert_ist_wohlgeformt(bx::kDriverBuildVariantSignature));
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
    EXPECT_THROW((void)pfn::build_variant_set_signature_define_arg("bvset=1;page_type[{a b}]"), std::invalid_argument);

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
    std::string               wert = "tc=1";
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

// -- T2-B (1): DAS GLIED [5] TRENNT DIE opt-STUFEN WIEDER --------------------------------------------
// Codex-Zweitreview [CX-B1], KRITISCH, verbatim: "Live-Glied[5] nur cxx/ceb/bt -> O2/O3 derselben Zelle
// identischer Fingerprint = falscher Skip". Der Test misst genau diese Aussage am Renderer-Weg, den die
// Perm-Schleife wirklich benutzt -- nicht an einem nachgebauten Zwilling.
TEST(MW12StampBausteine, T2bGliedTrenntOptStufenUndSimd) {
    namespace pfn = ::comdare::cache_engine::profile_facade;

    pfn::PermToolchainAchsen o2{};
    o2.opt       = "O2";
    o2.opt_flags = "-O2";
    pfn::PermToolchainAchsen o3{};
    o3.opt       = "O3";
    o3.opt_flags = "-O3";
    pfn::PermToolchainAchsen o3avx{};
    o3avx.opt               = "O3";
    o3avx.opt_flags         = "-O3";
    o3avx.simd              = "avx512";
    o3avx.gate_contribution = "avx512";

    std::string const g2    = pfn::compose_toolchain_stamp_glied_for_perm(o2);
    std::string const g3    = pfn::compose_toolchain_stamp_glied_for_perm(o3);
    std::string const g3avx = pfn::compose_toolchain_stamp_glied_for_perm(o3avx);

    // (1) DER BEFUND, GEHEILT: verschiedene opt-Stufen => verschiedene Glieder => verschiedene Preimages.
    EXPECT_NE(g2, g3);
    // (2) Die Flags stehen MIT im Glied -- zwei Optionen gleicher id mit anderen Flags waeren sonst gleich
    //     (Owner-KERN abend-5: die Flags sind Teil der Achsen-DEFINITION).
    EXPECT_NE(g2.find("opt=O2{-O2}"), std::string::npos) << g2;
    EXPECT_NE(g3.find("opt=O3{-O3}"), std::string::npos) << g3;
    // (3) ext und gate wandern ebenfalls ins Glied.
    EXPECT_NE(g3avx.find("ext=avx512"), std::string::npos) << g3avx;
    EXPECT_NE(g3avx.find("gate=avx512"), std::string::npos) << g3avx;
    EXPECT_NE(g3, g3avx);
    // (4) DIE IDENTITAETS-ZUSAGE: leere Perm-Achsen == der run-konstante Wert. Der Einzel-Pfad rechnet
    //     damit byte-identisch weiter -- es gibt nur EINEN Renderer-Weg, keinen zweiten fuer "live".
    EXPECT_EQ(pfn::compose_toolchain_stamp_glied_for_perm(pfn::PermToolchainAchsen{}),
              pfn::compose_live_toolchain_stamp_glied());
    // (5) Jedes erzeugte Glied bleibt ein gueltiger, transportfaehiger Preimage-Wert.
    for (std::string const& g : {g2, g3, g3avx}) {
        EXPECT_TRUE(::comdare::cache_engine::abi::injizierter_glied_wert_ist_wohlgeformt(g)) << g;
        EXPECT_NO_THROW((void)pfn::toolchain_stamp_glied_define_arg(g)) << g;
    }
}

// -- T2-B (2): DIE atomic128-ACHSE HAT EINE QUELLE ----------------------------------------------------
// Vor T2-B stand dasselbe #if in der Facade (perm_compiler_isa_cflags) UND haette im Stempel ein zweites
// Mal stehen muessen. Zwei Orte, eine Entscheidung -- der Bau haette -mcx16 angehaengt, waehrend das Glied
// "no_cx16" behauptet, ohne dass irgendwo etwas bricht.
TEST(MW12StampBausteine, T2bAtomic128HatEineQuelle) {
    namespace pfn = ::comdare::cache_engine::profile_facade;
    namespace cm  = ::comdare::cache_engine::measurement;

    pfn::Atomic128Wahl const w = pfn::active_atomic128_wahl();
    // (1) Die Wahl ist IMMER getroffen -- es gibt keinen "unbekannt"-Zustand (fail-closed: die Achse hat
    //     einen Default, und der steht dann auch im Glied).
    EXPECT_FALSE(w.id.empty());
    // (2) id und Flags gehoeren zusammen: cx16 traegt -mcx16, no_cx16 traegt nichts.
    if (w.id == cm::Cx16Option::atomic128_id()) {
        EXPECT_EQ(w.flags, cm::Cx16Option::gcc_flag());
    } else {
        EXPECT_EQ(w.id, cm::NoCx16Option::atomic128_id());
        EXPECT_TRUE(w.flags.empty());
    }
    // (3) Und genau diese id steht im LIVE-Glied -- der Stempel kann keine andere Wahl behaupten.
    std::string const live = pfn::compose_live_toolchain_stamp_glied();
    EXPECT_NE(live.find(std::string{"atomic128="} + std::string{w.id}), std::string::npos) << live;
}

// -- T2-C: DIE RT-REALVERSIONS-SONDE AM TIER-TREIBER --------------------------------------------------
// Codex-Zweitreview [K], KRITISCH: "Tier-Treiber-REALVERSION wird nie gemessen (g++-16-Binary 16.1->16.3
// = identischer Stempel = falscher Skip)". Der Test misst die Sonde selbst -- ihre Antwort, ihre
// Fail-closed-Wege und die Zusage, dass ein nicht sondierbarer Tag NIE an eine Shell geht.
TEST(MW12StampBausteine, T2cRealversionsSondeIstFailClosed) {
    namespace pfn = ::comdare::cache_engine::profile_facade;

    // (1) DIE ECHTE FLOTTE. Auf einer Maschine ohne den Treiber liefert die Sonde ehrlich nichts --
    //     deshalb wird die Antwort nur GEPRUEFT, wenn es eine gibt (kein Test, der Umgebung behauptet).
    for (char const* tag : {"g++", "g++-16"}) {
        auto const v = pfn::tier_realversion_von(tag);
        if (v.has_value()) {
            EXPECT_TRUE(pfn::sonden_antwort_ist_version(*v)) << tag << " -> '" << *v << "'";
            EXPECT_NE(v->find('.'), std::string::npos) << "erwartet X.Y.Z-Form, bekam '" << *v << "'";
        }
    }
    // (2) EIN TREIBER, DEN ES NICHT GIBT: die Sonde laeuft, bekommt keine brauchbare Antwort, und sagt
    //     das auch -- statt eine Version zu erfinden (Owner-KERN: keine Phantom-Versionen).
    EXPECT_FALSE(pfn::tier_realversion_von("g++-999999").has_value());
    // (3) NICHT SONDIERBARE TAGS gehen GAR NICHT ERST an eine Shell. Die Glied-Wachen allein reichen
    //     dafuer nicht (sie sind Denylisten und lassen '$'/'&'/'|' durch, die im Preimage harmlos sind).
    for (char const* boese : {"g++ -o /tmp/x", "g++;touch /tmp/x", "g++$(id)", "g++|sh", "g++&", ""}) {
        EXPECT_FALSE(pfn::treiber_tag_ist_sondierbar(boese)) << "tag='" << boese << "'";
        EXPECT_FALSE(pfn::tier_realversion_von(boese).has_value()) << "tag='" << boese << "'";
    }
    // (4) Reale Tag-Formen bleiben sondierbar -- die Wache ist kein Verbot des Normalfalls.
    for (char const* gut : {"g++", "g++-16", "clang++-18", "/usr/bin/g++-16"})
        EXPECT_TRUE(pfn::treiber_tag_ist_sondierbar(gut)) << "tag='" << gut << "'";

    // (5) Die Antwort-Wache: nur Ziffern und Punkte, beginnend mit einer Ziffer.
    for (char const* boese : {"gcc (Debian) 15.3.0", "", "v15.3.0", "15.3.0-suse", "abc"})
        EXPECT_FALSE(pfn::sonden_antwort_ist_version(boese)) << "antwort='" << boese << "'";
    for (char const* gut : {"15.3.0", "16.0.1", "16"}) EXPECT_TRUE(pfn::sonden_antwort_ist_version(gut));
}

// -- T2-C (2): UNBEKANNTE VERSION == NICHT SKIP-FAEHIG, UND DIE STEMPEL-FORM -------------------------
TEST(MW12StampBausteine, T2cUnbekanntHeisstNichtSkipFaehig) {
    namespace pfn = ::comdare::cache_engine::profile_facade;

    // (1) Die Skip-Faehigkeits-Frage ist exakt "ist eine Realversion erhoben?" -- kein zweiter Begriff.
    EXPECT_EQ(pfn::tier_realversion_ist_bekannt(), !pfn::active_tier_realversion().empty());

    // (2) DIE STEMPEL-FORM. Ist die Version erhoben, steht sie als <dialekt>-<realversion>:<tag> im Glied;
    //     ist sie es nicht, steht dort <dialekt>:<tag> -- der Treiber bleibt in BEIDEN Faellen drin
    //     (NB2-1 Regel R1), es faellt nur die Versions-Behauptung weg.
    std::string const tag   = pfn::active_cxx_driver_tag();
    std::string const real  = pfn::active_tier_realversion();
    std::string const glied = pfn::compose_live_toolchain_stamp_glied();
    std::string const dialekt{pfn::cxx_driver_dialect(tag)};
    std::string const erwartet =
        real.empty() ? ("cxx=" + dialekt + ":" + tag) : ("cxx=" + dialekt + "-" + real + ":" + tag);
    EXPECT_NE(glied.find(erwartet), std::string::npos) << "erwartet '" << erwartet << "' in '" << glied << "'";

    // (3) Und das Ergebnis bleibt ein gueltiger, transportfaehiger Preimage-Wert.
    EXPECT_TRUE(::comdare::cache_engine::abi::injizierter_glied_wert_ist_wohlgeformt(glied)) << glied;
}

// -- T2-E: DER tc=1-ALT/NEU-KOLLISIONS-NACHWEIS, AM OBJEKT ------------------------------------------
// Die Auflage lautete: ERST VERIFIZIEREN, ob die Alt-Form (cxx ohne Treiber-Tag) und die Neu-Form
// kollidieren koennen -- und NUR falls ja, ein Format-Bump (den der Frozen-Vektor in END-Form teuer
// macht). Dieser Test fuehrt die Verifikation, statt sie zu behaupten.
TEST(MW12StampBausteine, T2eTc1AltNeuKollidiertNicht) {
    namespace abi = ::comdare::cache_engine::abi;

    // (V1) DIE ALTE FORM IST NICHT MEHR HERSTELLBAR: ein Dialekt ohne Treiber-Tag wird FAIL-LOUD
    //      abgelehnt, nicht still verworfen. Es gibt keinen Eingabe-Vektor, aus dem der Renderer noch
    //      ein tagloses cxx-Feld baut.
    abi::ToolchainStampParts alt{};
    alt.cxx_dialect     = "gcc";
    alt.cxx_realversion = "16.2.0"; // exakt die Alt-Form aus den Frozen-Literalen
    EXPECT_EQ(abi::toolchain_stamp_parts_abhaengigkeits_diagnose(alt), std::string_view{"cxx_dialect"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(alt), std::invalid_argument);

    // (V2) SELBST WENN SIE ES WAERE, KOLLIDIERT SIE NICHT: jede herstellbare Neu-Form traegt GENAU EIN
    //      ':', jede denkbare Alt-Form keines. Zeichenketten mit und ohne ':' sind nie gleich.
    abi::ToolchainStampParts neu = alt;
    neu.cxx_driver               = "g++-16";
    std::string const g          = abi::render_toolchain_stamp_glied(neu);
    EXPECT_NE(g.find("cxx=gcc-16.2.0:g++-16"), std::string::npos) << g;
    EXPECT_EQ(std::count(g.begin(), g.end(), ':'), 1) << g;

    // (V2b) Auch OHNE Realversion (der haeufige Fall auf einer Maschine ohne Sonden-Antwort) bleibt es
    //       bei genau einem ':' -- der Tag ist Pflicht, nicht Kuer.
    abi::ToolchainStampParts ohne_version{};
    ohne_version.cxx_dialect = "gcc";
    ohne_version.cxx_driver  = "g++-16";
    std::string const g2     = abi::render_toolchain_stamp_glied(ohne_version);
    EXPECT_NE(g2.find("cxx=gcc:g++-16"), std::string::npos) << g2;
    EXPECT_EQ(std::count(g2.begin(), g2.end(), ':'), 1) << g2;

    // (V3) Ein zweites ':' kann nicht hineinkommen: alle drei Bestandteile sperren es einzeln, und die
    //      Ergebnis-Wache im Renderer prueft zusaetzlich das Zusammengesetzte.
    abi::ToolchainStampParts boese = neu;
    boese.cxx_driver               = "g++:16";
    EXPECT_EQ(abi::toolchain_stamp_parts_diagnose(boese), std::string_view{"cxx_driver"});
    EXPECT_THROW((void)abi::render_toolchain_stamp_glied(boese), std::invalid_argument);

    // (V4) SCHLUSS: tc=1 bleibt -- die Format-Kennung wurde NICHT gebumpt.
    EXPECT_EQ(abi::kToolchainStampGliedFormat, std::string_view{"1"});
    EXPECT_TRUE(g.starts_with("tc=1;"));
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
    EXPECT_NO_THROW((void)abi::ToolchainGlied{std::string_view{"tc=1;cxx=gcc-16.0.1@1.0.0.c;opt=O3{-O3}@1.0.0.c"}});
    EXPECT_NO_THROW(
        (void)abi::BvsetGlied{std::string_view{"bvset=1;bv=2;page_type[{bplus;hw_cache_line=64}];simd_extension[]"}});
    EXPECT_NO_THROW((void)abi::ToolchainGlied{std::string_view{}});
    EXPECT_TRUE(abi::injizierter_glied_wert_ist_wohlgeformt(abi::kToolchainStampGlied));
    EXPECT_TRUE(abi::injizierter_glied_wert_ist_wohlgeformt(abi::kBuildVariantSetSignatureGlied));

    // (5) Die LAUFZEIT-Preimage-Bildung faengt den Separator auch in den Stempel-ZEILEN [1]-[3], die nicht
    //     durch einen Traeger-Typ laufen. Ohne diesen Zweig bliebe genau dort eine Luecke.
    std::array<std::string_view, abi::kAnatomyFingerprintGliedCount> const kaputt{
        abi::kAnatomyFingerprintFormat, "organ\nzeile", "", "", "", "", "", ""};
    EXPECT_THROW(
        (void)abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{kaputt.data(), kaputt.size()}),
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
    auto const glieder =
        abi::anatomy_fingerprint_glieder("ORGAN", "SYSTEM", "MESS", abi::ToolchainGlied{tc}, abi::BvsetGlied{bv});
    EXPECT_EQ(glieder[abi::kAnatomyFingerprintToolchainGlied], tc);
    EXPECT_EQ(glieder[abi::kAnatomyFingerprintBvsetGlied], bv);
    std::string const preimage =
        abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{glieder.data(), glieder.size()});
    EXPECT_NE(preimage.find("cxx="), std::string::npos) << "preimage='" << preimage << "'";
    EXPECT_NE(preimage.find("bvset="), std::string::npos) << "preimage='" << preimage << "'";

    // (4) WIRKSAMKEIT: mit den Live-Gliedern ergibt sich ein ANDERER Digest als mit den leeren Defaults.
    //     Das ist die eigentliche Aussage -- vorher waren beide Wege byte-gleich.
    auto const leer = abi::anatomy_fingerprint_glieder("ORGAN", "SYSTEM", "MESS");
    EXPECT_NE(abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{leer.data(), leer.size()}), preimage);

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
    std::string const ct_major{
        abi::kDetectedCompilerRealVersion.substr(0, abi::kDetectedCompilerRealVersion.find('.'))};
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

// -- E-E: DAS OVERLAY-GLIED [7] STEHT LIVE IM PREIMAGE -------------------------------------------------
//
// DIE ARBEITSTEILUNG (dieselbe wie zwischen dem Frozen-Vektor oben und NbCx4LiveGliederStehenImPreimage):
// der eingefrorene Vektor pinnt einen LITERALEN Overlay-Wert und beweist damit das FORMAT; dieser Test
// beweist die WIRKSAMKEIT des LIVE-Werts. Ohne ihn koennte das Glied weiter leer durchlaufen und niemand
// merkte es -- genau der Zustand, in dem es vor E-E war, und genau der, den ein gruener Frozen-Vektor
// NICHT ausschliesst.
TEST(MW12StampBausteine, EeOverlayGliedStehtLiveImPreimage) {
    namespace abi = ::comdare::cache_engine::abi;
    namespace pf  = ::comdare::cache_engine::profile_facade;

    // (1) Der LIVE-Wert ist da und ist genau EIN SHA-512-Hex. Ein leeres Glied hiesse: der Pre-Build-
    //     Codegen ist nicht gelaufen (oder seine Bau-Kante fehlt) -- dann deckt der Fingerprint reine
    //     Quell-Code-Aenderungen NICHT, und der Skip im Lager waere wieder falsch.
    std::string_view const ovl = pf::live_overlay_source_hash_glied();
    ASSERT_FALSE(ovl.empty()) << "das Overlay-Glied [7] ist LIVE LEER -- der Codegen hat nicht gewirkt";
    EXPECT_EQ(ovl.size(), std::size_t{128}) << "glied='" << ovl << "'";
    EXPECT_EQ(ovl.find_first_not_of("0123456789abcdef"), std::string_view::npos) << "glied='" << ovl << "'";
    EXPECT_EQ(ovl, abi::kOverlaySourceHash) << "die Naht muss GENAU die einkompilierte Konstante liefern";

    // (2) Er steht LITERAL im Preimage, an seiner benannten Position.
    auto const glieder = abi::anatomy_fingerprint_glieder("ORGAN", "SYSTEM", "MESS", abi::ToolchainGlied{""},
                                                          abi::BvsetGlied{""}, abi::OverlayHash{ovl});
    EXPECT_EQ(glieder[abi::kAnatomyFingerprintOverlayGlied], ovl);
    std::string const preimage =
        abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{glieder.data(), glieder.size()});
    EXPECT_NE(preimage.find(ovl), std::string::npos);

    // (3) WIRKSAMKEIT -- die eigentliche Aussage: mit belegtem Glied ergibt sich ein ANDERER Digest als
    //     mit leerem. Vor E-E waren beide Wege byte-gleich, weil das Glied immer leer war.
    auto const leer = abi::anatomy_fingerprint_glieder("ORGAN", "SYSTEM", "MESS", abi::ToolchainGlied{""},
                                                       abi::BvsetGlied{""}, abi::OverlayHash{""});
    EXPECT_NE(abi::anatomy_fingerprint_preimage(std::span<std::string_view const>{leer.data(), leer.size()}), preimage);

    // (4) DIE DRIFT-FREIHEIT DER NAHT: der argumentlose Aufruf liefert denselben Wert wie die Konstante,
    //     die der CEB-Laufzeit-Zwilling liest. Genau darauf ruht, dass Bau-Kanal (-D an perm_compile) und
    //     Zwilling ueber DASSELBE Glied rechnen -- es gibt keinen Parameter, ueber den eine zweite
    //     Wahrheit hereinkaeme.
    EXPECT_EQ(pf::live_overlay_source_hash_glied(), ovl);

    // (5) Das Define-ARGUMENT traegt die Wertform als C-String-Literal und enthaelt keinen Whitespace --
    //     sonst zerfiele es in der gcc-Response-Datei in mehrere Optionen.
    std::string const arg = pf::overlay_source_hash_define_arg(ovl);
    EXPECT_TRUE(arg.starts_with("-DCOMDARE_OVERLAY_SOURCE_HASH=")) << arg;
    EXPECT_EQ(arg.find(' '), std::string::npos) << arg;
    EXPECT_NE(arg.find(ovl), std::string::npos) << arg;
    EXPECT_EQ(pf::overlay_source_hash_define_arg(""), std::string{}) << "leer => kein Define => Identitaet";

    // (6) Die Transport-Wache ist scharf: ein Wert mit Domain-Separator kommt nicht durch.
    EXPECT_THROW((void)pf::overlay_source_hash_define_arg("ab\ncd"), std::invalid_argument);
    EXPECT_THROW((void)pf::overlay_source_hash_define_arg("a b"), std::invalid_argument);
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

    constexpr std::string_view kOrgan   = "search_algo=k_ary@1.0.0.c";
    constexpr std::string_view kSystem  = "target_isa=code@1.0.0.c";
    constexpr std::string_view kMeasure = "measurement_tooling=wallclock@1.0.0.c";

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
    constexpr std::string_view kTc = "tc=1;cxx=gcc:g++-16@1.0.0.c";
    EXPECT_EQ(abi::ToolchainGlied{kTc}.wert(), kTc);
    EXPECT_EQ(abi::BvsetGlied{std::string_view{"bvset=1"}}.wert(), std::string_view{"bvset=1"});
    EXPECT_EQ(abi::OverlayHash{std::string_view{}}.wert(), std::string_view{});
}
