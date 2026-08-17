// test_g2_semantik_duplikatverbot.cpp -- #17 G-2-SEMANTIK-NACHZUG (KON13-03 / KON9-09, 17.08.2026).
//
// DER GEGENSTAND: G-2 ist die zweiphasige Grammatik der System- UND Organ-Zeilen (KON13-03:
// "existiert schon als volle System-Achsen-Syntax und Semantik muss nachgeholt werden in
// Reihenfolge der Achsen-Nummerierung"). Die SYNTAX ist seit 07.08. gebaut (Flag-Grammatik v2 +
// S2-Katalog + S-3b-Voraussetzungs-Ketten). Die hier geschlossene SEMANTIK-Luecke ist die im
// Wachen-Kasten (m8) von measurement/algo_semver.hpp seit S2 AUSDRUECKLICH als "NICHT gebaut"
// gefuehrte Fehlerklasse REDUNDANZ (Geschwister-Duplikat), die das G-1-Design (M-11,
// docs/plaene/20260813-DESIGN-g1-grammatik-dreiphasige-messachse.md) woertlich DIESEM Auftrag
// (#17) zuweist: ein (token,eltern)-Element, das ZWEIMAL in der Flag-MENGE steht ("1.0.0.c.c",
// "1.0.0.c{p.p}", "1.0.0.x512{f.f}"), ist syntaktisch wohlgeformt (Parser), katalog-konform (S2)
// und ketten-konform (S-3b) -- und sagt trotzdem nichts Zweites: zwei rohe Zeichenfolgen fuer
// DENSELBEN Stand sind eine Alias-Identitaet (B11-Doktrin: der Stempel IST Identitaet).
//
// KOEDER-DOKTRIN (T-1): die Negativ-Proben dieses Tests sind SEMANTISCH ungueltige, SYNTAKTISCH
// gueltige Literale -- der Parser nimmt jede von ihnen an (Beleg je Probe im Test). Vor dem Bau
// der Wache fiel dieser Test literal ("Actual: true / Expected: false" an den EXPECT_FALSE der
// Voll-Wache) -- der Riss war die Luecke, nicht der Test.
//
// ACHSEN-NUMMERIERUNGS-REIHENFOLGE (Owner KON13-03): die Durchsetzung laeuft je Kategorie in der
// kanonischen Achsen-Ordnung ihrer EINEN Tabelle/Typ-Liste -- Kategorie SYSTEM zuerst (die
// System-Achse ist die freigebende Quelle, KON8-08): kSystemAxisCodeVersions [0] target_isa,
// [1] operating_system, [2] external_utils (abi/system_axis_code_versions.hpp, Ordnung ==
// kanonische System-Stempel-Ordnung). Danach Kategorie ORGAN: die 18 Organ-Haupt-Achsen in
// kCompositionAxisNames-/T00..T17-Ordnung -- deren VOLLE registrierte Population laeuft
// compile-time durch dieselbe Voll-Wache (assert_version_grammar je Variante,
// builder/experiment_tree/axis_variant_version_table.hpp; dedizierte schwere TU:
// test_reflect_versions_all_registered.cpp). Dieser Test traegt die leichte Flaeche
// (header-only), die schwere Population traegt die bestehende TU -- KEIN Doppelbau.
//
// MESS bleibt hier BEWUSST aussen vor: G-2 deckt System+Organ; die Mess-Zeile ist G-1
// (dreiphasig, benannte Leerstelle im Mess-Home, eigenes Design #53). Die Mess-Tooling-Literale
// ("1.0.0.c") laufen als ce-eigene Versionen durch dieselbe B12-Politik und bleiben gruen.

#include <cache_engine/abi/system_axis_code_versions.hpp>
#include <cache_engine/measurement/algo_semver.hpp>

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace meas = ::comdare::cache_engine::measurement;
namespace abi_ = ::comdare::cache_engine::abi;

// ------------------------------------------------------------------------------------------------
// KATEGORIE SYSTEM (Achsen-Nummerierung [0] target_isa, [1] operating_system, [2] external_utils)
// ------------------------------------------------------------------------------------------------

// Bestand: jedes System-Achsen-Literal besteht die VOLLE Politik (inkl. der neuen Semantik-Stufe).
// Die Ordnung der Tabelle IST die kanonische System-Stempel-Ordnung -- hier mitgeprueft, damit die
// Nummerierungs-Reihenfolge nicht stillschweigend kippt (eine Umsortierung waere ein Byte-Ereignis).
TEST(G2SemantikSystem, BestandInNummerierungsOrdnungBleibtGruen) {
    ASSERT_EQ(abi_::kSystemAxisCodeVersions.size(), 3u);
    EXPECT_EQ(abi_::kSystemAxisCodeVersions[0].axis, std::string_view{"target_isa"});
    EXPECT_EQ(abi_::kSystemAxisCodeVersions[1].axis, std::string_view{"operating_system"});
    EXPECT_EQ(abi_::kSystemAxisCodeVersions[2].axis, std::string_view{"external_utils"});
    for (auto const& e : abi_::kSystemAxisCodeVersions) {
        EXPECT_TRUE(meas::ce_owned_version_is_wellformed(e.version))
            << "System-Achse " << e.axis << " Literal " << e.version;
    }
}

// KOEDER je System-Achse, in Nummerierungs-Ordnung: das ECHTE Tabellen-Literal, um sein eigenes
// Basis-Element verdoppelt ("1.0.0.c" -> "1.0.0.c.c"). Fuer den PARSER wohlgeformt (Beleg), fuer
// den Katalog zulaessig ('c' existiert), ketten-frei -- NUR die Redundanz-Semantik kann ablehnen.
TEST(G2SemantikSystem, KoederDoppeltesBasisElementFaellt) {
    for (auto const& e : abi_::kSystemAxisCodeVersions) {
        std::string const koeder = std::string{e.version} + ".c";
        ASSERT_FALSE(meas::parse_algo_semver(koeder).is_sentinel())
            << "Koeder muss syntaktisch gueltig sein: " << koeder;
        EXPECT_FALSE(meas::ce_owned_version_is_wellformed(koeder))
            << "System-Achse " << e.axis << ": semantisch ungueltiger Koeder ging durch: " << koeder;
    }
}

// ------------------------------------------------------------------------------------------------
// KATEGORIE ORGAN (18 Haupt-Achsen, T00..T17; Population compile-time via Voll-Registry-Wache)
// ------------------------------------------------------------------------------------------------

// Bestand: die drei dokumentierten Organ-Literal-Formen (axis_variant_version_table.hpp:
// "1.0.0.c" Grundbestand, "1.0.1.c" Allokator 1. Bump, "1.0.2.c" reallocate-Bump) bleiben gruen.
TEST(G2SemantikOrgan, DokumentierteBestandsFormenBleibenGruen) {
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed("1.0.0.c"));
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed("1.0.1.c"));
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed("1.0.2.c"));
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed("1.0.0.c{p.e}"));
}

// KOEDER der Organ-Zeilen-Formen -- exakt die (m8)-Fehlerklasse: Duplikat auf Tiefe 0, Duplikat
// im cpu_sub (die M-11-Luecke der G-2-Zeilen), Duplikat in einer Breiten-Basis-Gruppe.
TEST(G2SemantikOrgan, KoederGeschwisterDuplikateFallen) {
    for (std::string_view koeder : {
             std::string_view{"1.0.0.c.c"},          // Basis-Element doppelt (m8-Beispiel 1)
             std::string_view{"1.0.1.c{p.p}"},       // cpu_sub-Duplikat (G-1 M-11 -> #17)
             std::string_view{"1.0.2.c{e.e}"},       // cpu_sub-Duplikat, e-Form
             std::string_view{"1.0.0.c.x512{f.f}"},  // Sub-Duplikat unter Breiten-Basis (m8-Beispiel 2)
             std::string_view{"1.0.0.c{p}.c{e}"},    // Basis doppelt mit verschiedenen Klammern
             std::string_view{"1.0.0.c.x512{f}.x512{bw}"}, // Breiten-Basis doppelt auf Tiefe 0
         }) {
        ASSERT_FALSE(meas::parse_algo_semver(koeder).is_sentinel())
            << "Koeder muss syntaktisch gueltig sein: " << koeder;
        EXPECT_FALSE(meas::ce_owned_version_is_wellformed(koeder))
            << "semantisch ungueltiger Koeder ging durch: " << koeder;
    }
}

// SCHAERFE-GEGENPROBE: das Element ist das (token,eltern)-PAAR (S-3-Doktrin), NICHT das Token
// allein. 'vnni' existiert unter x256 UND x512 als ZWEI verschiedene Elemente (verschiedene
// CPUID-Bits, Katalog-Wache) -- diese Form ist KEIN Duplikat und muss gruen bleiben, sonst
// verbietet die Wache dokumentierte Hardware-Aussagen.
TEST(G2Semantik, TokenUnterVerschiedenenElternIstKeinDuplikat) {
    EXPECT_TRUE(meas::ce_owned_version_is_wellformed("1.0.0.c.x256{vnni}.x512{f.vnni}"));
}

// LAUFZEIT-SPIEGEL des Praedikats (Muster test_s3_ordnung_relation: die CT-Batterie im Header ist
// der Beweis, dieser Aufrufer ausserhalb des Headers haelt den Namen gerufen -- Abschrift schlaegt
// Loeschung). Dazu der Wache-greift-wirklich-Beleg: "1.0.0.c{p.p}" ist katalog- und ketten-konform
// und faellt AUSSCHLIESSLICH am neuen Term (e).
TEST(G2Semantik, PraedikatDirektUndTermEinhaengung) {
    EXPECT_TRUE(meas::flag_menge_ist_duplikatfrei(meas::parse_algo_semver("1.0.0.c{p.e}")));
    EXPECT_FALSE(meas::flag_menge_ist_duplikatfrei(meas::parse_algo_semver("1.0.0.c.c")));
    EXPECT_TRUE(meas::version_flag_menge_ist_duplikatfrei("1.0.2.c"));
    EXPECT_FALSE(meas::version_flag_menge_ist_duplikatfrei("1.0.0.x512{f.f}"));
    // Wache-greift-wirklich: Katalog und Ketten allein liessen den cpu_sub-Koeder durch.
    EXPECT_TRUE(meas::flag_catalog_is_satisfied(meas::parse_algo_semver("1.0.0.c{p.p}")));
    EXPECT_TRUE(meas::voraussetzungen_erfuellt(meas::parse_algo_semver("1.0.0.c{p.p}")));
    EXPECT_FALSE(meas::ce_owned_version_is_wellformed("1.0.0.c{p.p}"));
}
