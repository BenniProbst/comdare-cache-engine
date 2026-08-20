// tests/unit/test_s6c_kategorien_ordnung_zwei_welten.cpp -- S-6c/S-21 (N-1, #15-Nachlande
// 20.08.2026): DIE KATEGORIEN-ORDNUNGS-WACHE UEBER ZWEI WELTEN.
//
// GEGENSTAND. Das Haus fuehrt die drei Achsen-KATEGORIEN (MESS, SYSTEM, ORGAN) in ZWEI WELTEN mit
// je EIGENER Ordnung (ZWEI-WELTEN-AUFLAGE, Bauplan-15 N-1; Owner-Ausnahme KON6-02/4):
//
//   WELT A  AUSSEN-EBENEN (3):        MESS, SYSTEM, ORGAN         (KON21-03 "Ja genau, meint auch
//           Makro-Argumentfolge (#87), POD-Feldfolge (#78),        #87 und #78"; Begruendung ist
//           Preimage-Glieder [1][2][3]                             die Traeger-Kette, kein Realm-
//                                                                  Attribut)
//   WELT B  LAGER-MESSDATEN-KASKADE:  MESS -> SYSTEM -> ORGAN     (A9-Doc 6.1; Ebene 4 mess,
//                                                                  Ebene 5 system, Ebenen 6-10
//                                                                  Organ-Gruppen)
//   WELT C  LAGER-BINARIES-KASKADE:   SYSTEM -> ORGAN -> MESS     (D-12: der Mess-Typ liegt als
//                                                                  LETZTER/TIEFSTER Haupt-Achsen-
//                                                                  Typ im Baum)
//
// NIE EINE KANONISCHE FOLGE UEBER BEIDE WELTEN: B und C sind verschiedene Folgen -- eine einzige
// Konstante KANN nicht beide Welten speisen. Diese TU zementiert deshalb DREI GETRENNTE
// FREMD-ABSCHRIFTEN (je Welt eine, T-3/T-5: Orakel nicht aus dem Pruefling) und prueft jede Welt
// GEGEN IHRE EIGENE QUELLE: die Aussen-Ebenen gegen POD-Offsets und Preimage-Glieder, die zwei
// Lager-Welten gegen die KASKADEN-AUSGABE der Realm-Policies (lager_baum_writer.hpp; Vorbild der
// Wachen-Form: organ_gruppen_decken_die_komposition ebenda -- S-21). Die bestehenden lb0-Pins
// pinnen BYTES je Beispiel; diese Wache pinnt die KATEGORIEN-ORDNUNG als Regel und macht die
// Fehlerklassen R-A..R-F BENENNBAR (Plan-Artefakt par.68a, KONSOLIDIERUNG 20260810 V-2/V-16).
//
// WAS DIESE TU AUSDRUECKLICH NICHT TUT: keine kanonische Gesamt-Folge ableiten, keine
// Ordnungs-Quelle umschreiben (S-6a/S-6c LESEN Ordnungs-Quellen nur, Bauplan-15 Abs.5).
//
// ASCII-only.

#include "bestandslog/lager_baum_writer.hpp"               // WELT B/C: die zwei Realm-Kaskaden (EIGENE Quellen)
#include <anatomy/anatomy_base.hpp>                        // Wurzel-Paar der Kaskaden (Gattung/Genus)
#include <cache_engine/abi/anatomy_fingerprint.hpp>        // WELT A: anatomy_fingerprint_glieder [1][2][3]
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // WELT A: AnatomyVersionLines (POD-Feldfolge)
#include <cache_engine/abi/anatomy_version_stamp.hpp>      // R-E/R-F: system_stamp_line (Meta-Meta-Anhang)
#include <cache_engine/abi/system_axis_order.hpp>          // R-A..R-D: die EINE System-Haupt-Achsen-Ordnung
#include <cache_engine/abi/system_cell_values.hpp>         // R-E: der wertfreie Hub (kSystemCellValueHubAxis)

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace abi = ::comdare::cache_engine::abi;
namespace bl  = ::comdare::cache_engine::builder::bestandslog;
namespace cea = ::comdare::cache_engine::anatomy;
using std::string_view;

/// Die drei Kategorien als TU-lokales Vokabular der Wache (kein Haus-Typ: die Welten teilen sich
/// gerade KEINE gemeinsame Ordnungs-Konstante, also bekommt auch die Wache keine).
enum class Kategorie : unsigned char { mess = 0, system = 1, organ = 2 };

// ================================================================================================
// DIE DREI FREMD-ABSCHRIFTEN (je Welt EINE; T-5: eingefroren aus den Plan-/Owner-Texten, nicht aus
// dem Pruefling abgelesen)
// ================================================================================================
inline constexpr std::array<Kategorie, 3> kWeltAussen{Kategorie::mess, Kategorie::system, Kategorie::organ};
inline constexpr std::array<Kategorie, 3> kWeltLagerMessdaten{Kategorie::mess, Kategorie::system, Kategorie::organ};
inline constexpr std::array<Kategorie, 3> kWeltLagerBinaries{Kategorie::system, Kategorie::organ, Kategorie::mess};

// NIE-EINE-FOLGE-BEWEIS (ZWEI-WELTEN-AUFLAGE, mechanisch): die Lager-Binaries-Folge ist VERSCHIEDEN
// von den beiden MESS-zuerst-Folgen -- es existiert also KEINE einzelne Permutation, die alle
// Welten zugleich erfuellt. Wer eine "kanonische" Konstante ueber beide Welten einzieht, muss
// mindestens eine der drei Abschriften faelschen und faellt hier.
static_assert(kWeltLagerBinaries != kWeltAussen && kWeltLagerBinaries != kWeltLagerMessdaten,
              "ZWEI-WELTEN-AUFLAGE (N-1/D-12): die Binaries-Kaskade folgt einer ANDEREN Ordnung als die "
              "Aussen-Ebenen und die Messdaten-Kaskade -- NIE eine kanonische Folge ueber beide Welten");
// Und die zwei MESS-zuerst-Folgen sind ZWEI ABSCHRIFTEN, kein geteilter Traeger: dass sie heute
// gleich LAUTEN, ist Befund der Quellen (KON21-03 vs. A9-Doc 6.1), keine geteilte Konstante.
static_assert(kWeltAussen == kWeltLagerMessdaten,
              "Befund-Pin: Aussen und Lager-Messdaten lauten HEUTE beide MESS,SYSTEM,ORGAN -- aendert sich "
              "eine der beiden QUELLEN, ist das ein Owner-Ereignis je Welt, nie ein stiller Gleichzug");

// ================================================================================================
// WELT A -- AUSSEN-EBENE 1: POD-FELDFOLGE (#78). offsetof ist am standard_layout-POD legal; die
// Wache prueft ORDNUNG (Zeilen-Felder UND Entry-Array-Felder), nicht Offsets (die pinnt test_m_w12).
// ================================================================================================
static_assert(offsetof(abi::AnatomyVersionLines, measurement_line) < offsetof(abi::AnatomyVersionLines, system_line) &&
                  offsetof(abi::AnatomyVersionLines, system_line) < offsetof(abi::AnatomyVersionLines, organ_line),
              "WELT A/#78: die POD-Zeilen-Felder muessen in der Kategorien-Ordnung MESS, SYSTEM, ORGAN "
              "liegen (S-6a, Layout 7) -- eine Umsortierung waere ein stiller Layout-Bruch");
static_assert(offsetof(abi::AnatomyVersionLines, measurement_entries) <
                      offsetof(abi::AnatomyVersionLines, system_entries) &&
                  offsetof(abi::AnatomyVersionLines, system_entries) <
                      offsetof(abi::AnatomyVersionLines, organ_entries),
              "WELT A/#78: die drei Entry-Arrays muessen DERSELBEN Kategorien-Ordnung folgen wie die "
              "Zeilen -- zwei Ordnungen in EINEM POD waeren genau die S-6a-Falle");

// ================================================================================================
// WELT A -- AUSSEN-EBENEN 2+3: PREIMAGE-GLIEDER [1][2][3] (und darueber die Makro-Argumentfolge
// #87, denn das Makro speist die Glieder benannt -- der POD-Assert oben deckt seine Feld-Seite).
// ================================================================================================
TEST(S6cKategorienOrdnung, WeltAussenPreimageGliederEinsZweiDreiSindMessSystemOrgan) {
    // Drei UNVERWECHSELBARE Marker; die Glied-Positionen kommen aus der EINEN Quelle
    // anatomy_fingerprint_glieder (S-6a: [1] Mess, [2] System, [3] Organ).
    std::string const mess   = "marker_mess=m@1.0.0.c";
    std::string const system = "marker_system=s@1.0.0.c";
    std::string const organ  = "marker_organ=o@1.0.0.c";
    auto const        glieder =
        abi::anatomy_fingerprint_glieder(abi::MessZeile{mess}, abi::SystemZeile{system}, abi::OrganZeile{organ},
                                         abi::ToolchainGlied{""}, abi::BvsetGlied{""}, abi::OverlayHash{""});
    ASSERT_GE(glieder.size(), 4u);
    EXPECT_EQ(string_view{glieder[1]}, string_view{mess}) << "Glied [1] ist die MESS-Zeile (S-6a)";
    EXPECT_EQ(string_view{glieder[2]}, string_view{system}) << "Glied [2] ist die SYSTEM-Zeile (S-6a)";
    EXPECT_EQ(string_view{glieder[3]}, string_view{organ}) << "Glied [3] ist die ORGAN-Zeile (S-6a)";
    // Ordnung gegen die FREMD-Abschrift der Welt A (nicht gegen eine geteilte Konstante):
    std::array<Kategorie, 3> gefunden{};
    gefunden[0] = glieder[1] == mess ? Kategorie::mess : (glieder[1] == system ? Kategorie::system : Kategorie::organ);
    gefunden[1] = glieder[2] == mess ? Kategorie::mess : (glieder[2] == system ? Kategorie::system : Kategorie::organ);
    gefunden[2] = glieder[3] == mess ? Kategorie::mess : (glieder[3] == system ? Kategorie::system : Kategorie::organ);
    EXPECT_TRUE(gefunden == kWeltAussen) << "WELT A folgt nicht der Abschrift MESS,SYSTEM,ORGAN";
}

// ================================================================================================
// WELT B -- LAGER-MESSDATEN-KASKADE: MESS -> SYSTEM -> ORGAN (gegen die KASKADEN-AUSGABE geprueft)
// ================================================================================================

inline constexpr bl::LagerWurzelPaar kWurzel{cea::AnatomyGattung::Map, cea::AnatomyGenus::SearchAlgorithm};

std::vector<bl::AchsenWert> organ_18() {
    return {
        {"search_algo", "prt_art"},   {"cache_traversal", "bfs"},   {"mapping", "direct"},
        {"path_compression", "none"}, {"node_type", "b_tree"},      {"memory_layout", "soa"},
        {"allocator", "arena"},       {"prefetch", "off"},          {"concurrency", "single"},
        {"serialization", "flat"},    {"value_handle", "inline_v"}, {"index_organization", "clustered"},
        {"io_dispatch", "sync"},      {"migration_policy", "lru"},  {"filter", "bloom"},
        {"queuing_q1", "none"},       {"queuing_q2", "none"},       {"persistence_target", "none"},
    };
}
std::vector<bl::AchsenWert> system_drei() {
    return {{"target_isa", "amd64_v3"}, {"operating_system", "linux"}, {"external_utils", "avx2"}};
}

/// Kategorie einer Kaskaden-Ebene, an ihrem ERSTEN Achsen-Namen erkannt. Wurzel-/Blatt-/Unter-Ebenen
/// liefern kein Ergebnis (die Wache misst NUR die drei Haupt-Kategorien).
[[nodiscard]] bool ebene_kategorie(std::string const& ebene, Kategorie& out) {
    if (ebene.rfind("mess=", 0) == 0) {
        out = Kategorie::mess;
        return true;
    }
    // System-Ebene beginnt mit der ERSTEN System-Haupt-Achse der bindenden Ordnung:
    if (ebene.rfind(std::string{abi::kSystemAxisOrder[0]} + "=", 0) == 0) {
        out = Kategorie::system;
        return true;
    }
    // Organ-Gruppen-Ordner beginnen mit dem Gruppen-Namen ("01_"..."05_"):
    if (ebene.size() > 3 && ebene[2] == '_' && ebene[0] == '0' && ebene[1] >= '1' && ebene[1] <= '5') {
        out = Kategorie::organ;
        return true;
    }
    return false;
}

/// Die Kategorien-FOLGE einer Kaskade (Duplikate benachbarter Organ-Gruppen zusammengezogen).
[[nodiscard]] std::vector<Kategorie> kategorien_folge(std::vector<std::string> const& ebenen) {
    std::vector<Kategorie> folge;
    for (auto const& e : ebenen) {
        Kategorie k{};
        if (!ebene_kategorie(e, k)) continue;
        if (folge.empty() || folge.back() != k) folge.push_back(k);
    }
    return folge;
}

TEST(S6cKategorienOrdnung, WeltLagerMessdatenKaskadeIstMessSystemOrgan) {
    bl::MessdatenBaumSpec s{.wurzel       = kWurzel,
                            .mess         = {{"mess", "vereint"}, {"load_framework", "on"}},
                            .system       = system_drei(),
                            .meta_metas   = {{"simd", "avx2"}},
                            .organ        = organ_18(),
                            .haupt_blatt  = {{"blatt", std::string(128, 'a')}},
                            .mess_unter   = {},
                            .system_unter = {},
                            .organ_unter  = {}};
    auto const            k = bl::MessdatenRealmPolicy::kaskade(s);
    ASSERT_TRUE(k.ok()) << "Kaskade der Messdaten-Policy bricht -- Wache kann die Ordnung nicht messen";
    auto const folge = kategorien_folge(k.ebenen);
    ASSERT_EQ(folge.size(), 3u) << "die Messdaten-Kaskade traegt nicht genau die drei Haupt-Kategorien";
    EXPECT_TRUE((std::array<Kategorie, 3>{folge[0], folge[1], folge[2]}) == kWeltLagerMessdaten)
        << "WELT B (Lager-Messdaten) folgt nicht der Abschrift MESS -> SYSTEM -> ORGAN (A9-Doc 6.1)";
}

// ================================================================================================
// WELT C -- LAGER-BINARIES-KASKADE: SYSTEM -> ORGAN -> MESS (D-12: Mess-Typ zutiefst)
// ================================================================================================
TEST(S6cKategorienOrdnung, WeltLagerBinariesKaskadeIstSystemOrganMess) {
    bl::BinariesBaumSpec s{.wurzel     = kWurzel,
                           .system     = system_drei(),
                           .meta_metas = {{"simd", "avx2"}},
                           .organ      = organ_18(),
                           .mess_typ   = {{"mess", "wallclock"}}};
    auto const           k = bl::BinariesRealmPolicy::kaskade(s);
    ASSERT_TRUE(k.ok()) << "Kaskade der Binaries-Policy bricht -- Wache kann die Ordnung nicht messen";
    auto const folge = kategorien_folge(k.ebenen);
    ASSERT_EQ(folge.size(), 3u) << "die Binaries-Kaskade traegt nicht genau die drei Haupt-Kategorien";
    EXPECT_TRUE((std::array<Kategorie, 3>{folge[0], folge[1], folge[2]}) == kWeltLagerBinaries)
        << "WELT C (Lager-Binaries) folgt nicht der Abschrift SYSTEM -> ORGAN -> MESS (D-12)";
    // D-12 woertlich: der Mess-Typ ist die LETZTE/TIEFSTE Haupt-Achsen-Ebene der Kaskade.
    ASSERT_FALSE(k.ebenen.empty());
    EXPECT_EQ(k.ebenen.back().rfind("mess=", 0), 0u) << "D-12: mess_typ liegt nicht zutiefst";
}

// ================================================================================================
// FEHLERKLASSEN R-A..R-F (par.68a-Register, KONSOLIDIERUNG 20260810 V-2/V-16) -- BENENNBAR:
// jede Klasse hat hier IHREN Assert; reisst eine, nennt der Text die Regression beim Namen.
// ================================================================================================
static_assert(abi::is_known_system_axis("operating_system"),
              "R-A (Regression 'OS-Achse fehlt'): operating_system MUSS System-Haupt-Achse sein");
static_assert(!abi::is_known_system_axis("scheduling"),
              "R-B (Regression 'scheduling als Haupt-Achse'): scheduling ist UNTER-Achse von target_isa");
static_assert(!abi::is_known_system_axis("load_framework"),
              "R-C (Regression 'load_framework im System-Realm'): load_framework ist MESS-Meta-Meta");
static_assert(!abi::is_known_system_axis("compiler"),
              "R-D (Regression 'compiler als Haupt-Achse'): compiler ist Unter-Achsen-GRUPPE der "
              "System-Komplex-Achse");
static_assert(abi::kSystemAxisOrder[2] == abi::kSystemCellValueHubAxis,
              "R-E (Regression 'Hub verloren'): external_utils ist der wertfreie Meta-Meta-HUB und "
              "die dritte Haupt-Achse -- seine Meta-Metas sind FESTE Identitaets-Bestandteile");
static_assert(!abi::is_known_system_axis("simd"),
              "R-F (Regression 'SIMD als Haupt-Achse'): simd ist META-META unter external_utils, "
              "nie eine eigene System-Haupt-Achse");

TEST(S6cKategorienOrdnung, RegisterRAEbisRFLaufzeitZeugen) {
    // R-E/R-F an der gerenderten System-Zeile: der simd-Anhang steht GEKLAMMERT am Zeilen-ENDE
    // (Meta-Meta), nie als klammerloses Haupt-Segment.
    std::string const zeile = abi::system_stamp_line();
    EXPECT_NE(zeile.find("[simd="), std::string::npos)
        << "R-E: das simd-Meta-Meta-Glied fehlt in der System-Zeile -- Meta-Metas sind FESTE "
           "Identitaets-Bestandteile (zeile="
        << zeile << ")";
    EXPECT_EQ(zeile.find(";simd="), std::string::npos)
        << "R-F: simd als KLAMMERLOSES Segment waere eine Haupt-Achse (zeile=" << zeile << ")";
    EXPECT_NE(zeile.rfind("simd=", 0), 0u) << "R-F: simd darf die Zeile nie als Haupt-Segment eroeffnen";
    // R-A..R-D am Laufzeit-Protokoll sichtbar machen (dieselben Aussagen wie die CT-Asserts oben):
    EXPECT_TRUE(abi::is_known_system_axis("operating_system")) << "R-A";
    EXPECT_FALSE(abi::is_known_system_axis("scheduling")) << "R-B";
    EXPECT_FALSE(abi::is_known_system_axis("load_framework")) << "R-C";
    EXPECT_FALSE(abi::is_known_system_axis("compiler")) << "R-D";
}

} // namespace
