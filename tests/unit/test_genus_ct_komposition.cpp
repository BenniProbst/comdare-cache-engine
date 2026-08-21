// test_genus_ct_komposition -- K02/F2-7a: die GENUS-LEISTUNGS-Version als CT-Komposition (KON2-06).
//
// OWNER-SOLL (10.08.2026, verbatim im Ledger KON2-06, vom Owner bestaetigt "Das ist jetzt alles
// korrekt."): "Jedes Genus hat fuer sein Interface eine Versionierung fuer das was es Leistet und
// das setzt sich als compile time Versionierungsstempel aus allen Mess/System/Organ-Achsen
// zusammen." KON2-07 nennt den Mechanismus: feste Achsen-Reihenfolge je Glied, die drei Glieder
// isoliert (Organ aus der gewaehlten lokalen Algorithmus-Versionierung, System isoliert fuer
// Hardware, Mess statisch zur Compile-Zeit).
//
// WAS DIESER TEST ZUSICHERT (jede Zusage mit Nenner):
//   (1) CT-BEWEIS: die Komposition ist in static_assert auswertbar (wirklich compile time,
//       nicht nur "constexpr-markiert") -- der Golden-String steht im static_assert.
//   (2) GLIEDER-ORDNUNG: MESS, SYSTEM, ORGAN -- die Aussen-Ebenen-Ordnung (S-6a/#15, KON21-03;
//       dieselbe Feldfolge wie AnatomyVersionLines: measurement_line, system_line, organ_line).
//   (3) PARTITION: die 5 andockenden Genera tragen eine nicht-leere Leistungs-Version,
//       FunctionInterfaceReroute traegt die LEERE (Owner-E-1 "Weg C" -- dieselbe Partition wie
//       pruef_dock_version_for, beide Richtungen benannt).
//   (4) ERGAENZUNG, KEIN ERSATZ: die 5 Dock-VERTRAGS-Literale bleiben "1.0.0.c" (algo_semver.hpp
//       Klasse (i)); die Leistungs-Version ersetzt sie nicht.
//   (5) TOKEN-DRIFT-WACHE: genus_leistungs_token == lager_genus_token je Genus (6/6) -- der
//       leichte Header dupliziert den Switch bewusst (lager_baum_writer ist ein 945-Zeilen-
//       Writer); die Gleichheit ist HIER gepinnt, damit die Kopie nie driftet.
//   (6) GRAMMATIK-EINHEIT: die Glieder laufen ueber build_axis_version_stamp_line -- denselben
//       Renderer wie die Stempel-Zeilen (kein zweiter Grammatik-Ort). RT-Wert == CT-Wert.
//
// T-11c-MUTATIONSANKER: Glied-Reihenfolge im Kompositum vertauschen (mess<->system) bricht (1),
// (2) und (6) literal im static_assert.

#include "pruef_dock/genus_leistungs_version.hpp"

#include "bestandslog/lager_baum_writer.hpp" // Drift-Wache (5): lager_genus_token (FREMDES Orakel)
#include "pruef_dock/pruef_dock_version.hpp" // Zusage (4): die Dock-VERTRAGS-Literale

#include <anatomy/anatomy_base.hpp>
#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace {

namespace pd      = ::comdare::cache_engine::builder::pruef_dock;
namespace anatomy = ::comdare::cache_engine::anatomy;

// Mock-Achsen nach dem Muster von test_m_w12_stamp_bausteine.cpp (name() + algo_version).
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
    using filter             = MockAxisV234; // abweichende Version: prueft je-Achse-Rendering
    using queuing_q1         = MockAxisV1;
    using queuing_q2         = MockAxisV1;
    using persistence_target = MockAxisV1;
};

// Der eine Golden-String: 3 Mess-Toolings (Registry-Reihenfolge wallclock/macro/micro), 3 System-
// Achsen (code-Marker), 18 Organ-Achsen in kCompositionAxisNames-Reihenfolge; filter traegt
// algoB@2.3.4, alle anderen algoA@1.0.0. RENDER-FORM AM OBJEKT GEMESSEN (Probe-TU 2026-08-21):
// render_algo_semver haelt die Flag-Grammatik v2 punkt-getrennt -- "1.0.0.c" bleibt "1.0.0.c".
constexpr std::string_view kGoldenSet =
    "genus=set"
    "|mess=measurement_tooling=wallclock@1.0.0.c;measurement_tooling=macro@1.0.0.c;"
    "measurement_tooling=micro@1.0.0.c"
    "|system=target_isa=code@1.0.0.c;operating_system=code@1.0.0.c;external_utils=code@1.0.0.c"
    "|organ=search_algo=algoA@1.0.0;cache_traversal=algoA@1.0.0;mapping=algoA@1.0.0;"
    "path_compression=algoA@1.0.0;node_type=algoA@1.0.0;memory_layout=algoA@1.0.0;"
    "allocator=algoA@1.0.0;prefetch=algoA@1.0.0;concurrency=algoA@1.0.0;"
    "serialization=algoA@1.0.0;value_handle=algoA@1.0.0;index_organization=algoA@1.0.0;"
    "io_dispatch=algoA@1.0.0;migration_policy=algoA@1.0.0;filter=algoB@2.3.4;"
    "queuing_q1=algoA@1.0.0;queuing_q2=algoA@1.0.0;persistence_target=algoA@1.0.0";

// (1) + (2) + (6): der CT-BEWEIS. Faellt dieser static_assert, ist die Komposition entweder nicht
// compile-time auswertbar oder ihre Glieder-Ordnung/Grammatik ist gewandert.
static_assert(pd::genus_leistungs_version<MockComposition>(anatomy::AnatomyGenus::Set) == kGoldenSet,
              "K02/F2-7a: die Genus-Leistungs-Version muss compile time exakt die dokumentierte "
              "Kompositions-Form MESS,SYSTEM,ORGAN tragen (KON2-06/07 + S-6a-Aussenordnung).");

// (3) PARTITION, beide Richtungen (dasselbe Muster wie pruef_dock_version.hpp):
static_assert(!pd::genus_leistungs_version<MockComposition>(anatomy::AnatomyGenus::SearchAlgorithm).empty());
static_assert(!pd::genus_leistungs_version<MockComposition>(anatomy::AnatomyGenus::Set).empty());
static_assert(!pd::genus_leistungs_version<MockComposition>(anatomy::AnatomyGenus::Sequence).empty());
static_assert(!pd::genus_leistungs_version<MockComposition>(anatomy::AnatomyGenus::Adapter).empty());
static_assert(!pd::genus_leistungs_version<MockComposition>(anatomy::AnatomyGenus::View).empty(),
              "HIN: jedes ANDOCKENDE Ebene-2-Genus traegt eine nicht-leere Leistungs-Version -- "
              "alle fuenf benannt, nicht an einem Wert behauptet.");
static_assert(pd::genus_leistungs_version<MockComposition>(anatomy::AnatomyGenus::FunctionInterfaceReroute).empty(),
              "RUECK: FunctionInterfaceReroute ist ein KLASSIFIKATIONS-Genus (Owner-E-1 'Weg C') -- seine "
              "Leistungs-Version ist LEER wie seine Dock-Version; das Interface leistet das ZIEL-Genus.");

TEST(GenusCtKomposition, RuntimeWertIstIdentischZumCtGolden) {
    // (6): derselbe constexpr-Pfad, zur Laufzeit gerufen, liefert dieselben Bytes wie der
    // static_assert oben -- kein zweiter Renderer, keine RT/CT-Drift.
    std::string const rt = pd::genus_leistungs_version<MockComposition>(anatomy::AnatomyGenus::Set);
    EXPECT_EQ(rt, std::string{kGoldenSet});
}

TEST(GenusCtKomposition, TokenDriftWacheGegenLagerBaumWriter) {
    // (5): 6/6 Genera -- die Header-Kopie des Token-Switch gegen die Writer-Quelle gepinnt.
    using anatomy::AnatomyGenus;
    namespace bl                  = ::comdare::cache_engine::builder::bestandslog;
    constexpr AnatomyGenus alle[] = {
        AnatomyGenus::SearchAlgorithm, AnatomyGenus::Set,  AnatomyGenus::Sequence,
        AnatomyGenus::Adapter,         AnatomyGenus::View, AnatomyGenus::FunctionInterfaceReroute};
    for (AnatomyGenus g : alle) {
        EXPECT_EQ(pd::genus_leistungs_token(g), bl::lager_genus_token(g))
            << "Token-Drift zwischen genus_leistungs_version.hpp und lager_baum_writer.hpp bei Genus "
            << static_cast<int>(g);
    }
}

TEST(GenusCtKomposition, ErgaenzungKeinErsatzDockLiteraleBleiben) {
    // (4): die fuenf VERTRAGS-Literale sind unberuehrt -- die Komposition ist eine Ergaenzung.
    using anatomy::AnatomyGenus;
    EXPECT_EQ(pd::pruef_dock_version_for(AnatomyGenus::SearchAlgorithm), "1.0.0.c");
    EXPECT_EQ(pd::pruef_dock_version_for(AnatomyGenus::Set), "1.0.0.c");
    EXPECT_EQ(pd::pruef_dock_version_for(AnatomyGenus::Sequence), "1.0.0.c");
    EXPECT_EQ(pd::pruef_dock_version_for(AnatomyGenus::Adapter), "1.0.0.c");
    EXPECT_EQ(pd::pruef_dock_version_for(AnatomyGenus::View), "1.0.0.c");
}

TEST(GenusCtKomposition, GliederOrdnungIstMessSystemOrgan) {
    // (2) als RT-Nachweis mit Positionsnennern: "|mess=" vor "|system=" vor "|organ=".
    std::string const v    = pd::genus_leistungs_version<MockComposition>(anatomy::AnatomyGenus::View);
    auto const        mess = v.find("|mess=");
    auto const        sys  = v.find("|system=");
    auto const        org  = v.find("|organ=");
    ASSERT_NE(mess, std::string::npos);
    ASSERT_NE(sys, std::string::npos);
    ASSERT_NE(org, std::string::npos);
    EXPECT_LT(mess, sys);
    EXPECT_LT(sys, org);
    EXPECT_EQ(v.rfind("genus=view", 0), 0u) << "die Zeile beginnt mit dem Genus-Schluessel";
}

} // namespace
