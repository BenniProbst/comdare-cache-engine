// test_c13_selektiver_rebuild -- #97/E-12 C-13: das wiederhergestellte Entscheidungs- und
// Ausweis-Modul der Skip-Oekonomie (selektiver_rebuild.hpp), auf dem heutigen Stempel-Stand.
//
// WAS DIESE TU BEWEIST:
//   (1) FLOTTEN-REGEL, BEIDE RICHTUNGEN -- gleiche belegte System-Zeile => KEIN Vollflotten-Zwang;
//       gewechselte Zeile => Zwang; leere Zeile (je Seite einzeln) => Zwang (fail-closed). Der
//       Gegeneingang zu jeder Zusicherung steht daneben (T-4).
//   (2) KOEDER: SYSTEMWECHSEL UEBERSTEUERT DEN AKTUELLEN STEMPEL -- eine Binary, deren Sidecar den
//       erwarteten Stempel EXAKT traegt (dll_is_current waere true), wird unter flotten_neubau
//       trotzdem GEBAUT, mit ausgewiesenem Grund. Eine Implementierung, die unter Systemwechsel
//       doch skippt (der falsch begruendete Skip), wird hier ROT.
//   (3) PER-BINARY-SKIP AM VOLLEN STEMPEL -- Skip nur bei Sidecar==Erwartung; Abweichung =>
//       neubau_stempel_abweichung; fehlende Binary => neubau_ohne_bestand; leere Erwartung =>
//       Neubau (fail-closed, das A2-Erbe).
//   (4) BVSET-RICHTUNG (A subset B), DELEGIERT -- Erweiterung (recorded subset current, Bindung
//       reproduziert) => Skip; Einschraenkung (current subset recorded, DIESELBEN Mengen
//       vertauscht) => Neubau; gebrochene Bindung => Neubau. Beide Richtungen an DEMSELBEN
//       Mengen-Paar, wie es der #59-Vertrag verlangt.
//   (5) AUSWEIS MIT FREMDEM NENNER -- die Zaehlwerk-Summe wird gegen die Groesse einer
//       TESTLOKALEN Entscheidungs-Liste geprueft (T-3: der Nenner kommt nie aus dem Prueflig),
//       die Abschlusszeile literal (V36.E-Ausweis-Pflicht).
//
// Temp-Disziplin wie test_a2_sha512_skip_gate (per-User-/per-Build-Wurzel, comdare_test_tmp.hpp);
// synthetische 128-hex-Vektoren (das Gate ist wert-agnostisch, Eich-Disziplin D7).

#include "builder/build_orchestrator/build_orchestrator.hpp" // write_fingerprint_sidecar (Fixture-Seite)
#include "builder/selektiver_rebuild.hpp"                    // der Prueflig
#include "comdare_test_tmp.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace ex = comdare::cache_engine::builder::experiment;
namespace fs = std::filesystem;

namespace {

std::string hexkey(char c) { return std::string(128, c); }

// bvset-Mengen fuer (4): B erweitert A um GENAU ein simd_extension-Element. Grammatik am Emitter
// abgelesen (bvset_teilmenge.hpp): kopf "bvset=..;bv=..", drei Achsen-Sektionen, Elemente
// geklammert.
constexpr char const* kBvsetA = "bvset=1;bv=1;page_type[{p4k}];simd_extension[{sse42}];general_hardware[{cl64}]";
constexpr char const* kBvsetB = "bvset=1;bv=1;page_type[{p4k}];simd_extension[{sse42}{avx2}];general_hardware[{cl64}]";

// Fixture: eine "Binary" (Inhalt egal -- entschieden wird ueber das Sidecar) + ihr
// .fingerprint-Sidecar, geschrieben ueber den ECHTEN Schreiber des Orchestrators (kein zweites
// Dateiformat im Test).
fs::path lege_binary_mit_sidecar(fs::path const& dir, std::string const& name, std::string const& sidecar_hex,
                                 std::string const& bvset_glied = {}) {
    fs::create_directories(dir);
    fs::path const out = dir / name;
    {
        std::ofstream f{out, std::ios::binary | std::ios::trunc};
        f << "dll-attrappe";
    }
    ex::write_fingerprint_sidecar(out, sidecar_hex, bvset_glied);
    return out;
}

fs::path frisches_testdir(std::string const& stem) {
    fs::path const  dir = comdare::test::user_tmp_dir() / ("c13_" + stem);
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir);
    return dir;
}

} // namespace

// ---------------------------------------------------------------------------
// (1) Die Flotten-Regel mit Gegeneingang in beiden Richtungen.
// ---------------------------------------------------------------------------
TEST(C13SelektiverRebuild, SystemwechselRegelBeideRichtungen) {
    std::string const zeile_alt = "SYSTEM|ceb=8.0|os=linux";
    std::string const zeile_neu = "SYSTEM|ceb=9.0|os=linux";

    // Gleicher, belegter Stand => KEIN Zwang (der Skip-Versuch je Binary bleibt erlaubt).
    EXPECT_FALSE(ex::system_wechsel_erzwingt_vollflotte(zeile_alt, zeile_alt));
    // Wechsel => Zwang, in BEIDEN Vergleichsrichtungen (die Relation ist Ungleichheit, nicht Ordnung).
    EXPECT_TRUE(ex::system_wechsel_erzwingt_vollflotte(zeile_alt, zeile_neu));
    EXPECT_TRUE(ex::system_wechsel_erzwingt_vollflotte(zeile_neu, zeile_alt));
    // Fail-closed: eine leere Seite ist KEIN Beleg fuer einen unveraenderten Stand -- je Seite einzeln.
    EXPECT_TRUE(ex::system_wechsel_erzwingt_vollflotte("", zeile_neu));
    EXPECT_TRUE(ex::system_wechsel_erzwingt_vollflotte(zeile_alt, ""));
    EXPECT_TRUE(ex::system_wechsel_erzwingt_vollflotte("", ""));
}

// ---------------------------------------------------------------------------
// (2) KOEDER: die Binary traegt den erwarteten Stempel EXAKT -- ohne Flotten-Zwang skippt sie
// (Vorbedingung, am Objekt belegt), unter Flotten-Zwang wird sie trotzdem GEBAUT. Ein falsch
// begruendeter Skip unter Systemwechsel wird hier ROT.
// ---------------------------------------------------------------------------
TEST(C13SelektiverRebuild, KoederSystemwechselUebersteuertAktuellenStempel) {
    auto const dir = frisches_testdir("koeder_systemwechsel");
    auto const fp  = hexkey('a');
    auto const out = lege_binary_mit_sidecar(dir, "perm.dll", fp);

    // VORBEDINGUNG: ohne Zwang waere das ein Skip -- sonst bewiese der Koeder nichts.
    auto const ohne_zwang = ex::entscheide_selektiven_rebuild(out, fp, ex::SkipBvsetKontext{}, false);
    ASSERT_FALSE(ohne_zwang.bauen);
    ASSERT_EQ(ohne_zwang.grund, ex::RebuildGrund::skip_stempel_aktuell);

    // DER BISS: unter flotten_neubau wird GEBAUT, und der Ausweis nennt den Grund beim Namen.
    auto const mit_zwang = ex::entscheide_selektiven_rebuild(out, fp, ex::SkipBvsetKontext{}, true);
    EXPECT_TRUE(mit_zwang.bauen) << "Systemwechsel muss den Einzel-Skip uebersteuern (Vollflotten-Neubau)";
    EXPECT_EQ(mit_zwang.grund, ex::RebuildGrund::neubau_systemwechsel);
}

// ---------------------------------------------------------------------------
// (3) Per-Binary-Entscheidung am vollen Stempel: Skip nur bei Gleichheit; jeder andere Fall ist
// ein begruendeter Neubau (fail-closed).
// ---------------------------------------------------------------------------
TEST(C13SelektiverRebuild, PerBinaryStempelEntscheidetMitGrund) {
    auto const dir = frisches_testdir("per_binary");

    // Skip: Sidecar == Erwartung.
    auto const aktuell = lege_binary_mit_sidecar(dir, "aktuell.dll", hexkey('a'));
    auto const e1      = ex::entscheide_selektiven_rebuild(aktuell, hexkey('a'), ex::SkipBvsetKontext{}, false);
    EXPECT_FALSE(e1.bauen);
    EXPECT_EQ(e1.grund, ex::RebuildGrund::skip_stempel_aktuell);

    // Gegeneingang: Sidecar weicht ab => Neubau, Grund stempel_abweichung (Bestand liegt vor).
    auto const abweichend = lege_binary_mit_sidecar(dir, "abweichend.dll", hexkey('b'));
    auto const e2         = ex::entscheide_selektiven_rebuild(abweichend, hexkey('a'), ex::SkipBvsetKontext{}, false);
    EXPECT_TRUE(e2.bauen);
    EXPECT_EQ(e2.grund, ex::RebuildGrund::neubau_stempel_abweichung);

    // Kein Bestand am Zielpfad => Neubau, Grund ohne_bestand.
    auto const e3 = ex::entscheide_selektiven_rebuild(dir / "fehlt.dll", hexkey('a'), ex::SkipBvsetKontext{}, false);
    EXPECT_TRUE(e3.bauen);
    EXPECT_EQ(e3.grund, ex::RebuildGrund::neubau_ohne_bestand);

    // Leere Erwartung => Neubau trotz vorhandener, beliebig gestempelter Binary (A2-Erbe: ohne
    // CT-Erwartung nie ueberspringen). Der Bestand liegt vor => stempel_abweichung.
    auto const e4 = ex::entscheide_selektiven_rebuild(aktuell, std::string{}, ex::SkipBvsetKontext{}, false);
    EXPECT_TRUE(e4.bauen);
    EXPECT_EQ(e4.grund, ex::RebuildGrund::neubau_stempel_abweichung);
}

// ---------------------------------------------------------------------------
// (4) Die bvset-RICHTUNG (A subset B), delegiert an dll_is_current: Erweiterung skippt (mit
// Bindung), Einschraenkung baut -- BEIDE Richtungen an demselben Mengen-Paar -- und eine
// gebrochene Bindung baut ebenfalls.
// ---------------------------------------------------------------------------
TEST(C13SelektiverRebuild, BvsetRichtungErweiterungSkipptEinschraenkungBaut) {
    auto const dir = frisches_testdir("bvset_richtung");

    std::string const recorded_fp = hexkey('a'); // Stempel der VORHANDENEN Binary (mit bvset A gebaut)
    std::string const current_fp  = hexkey('b'); // HEUTIGE Erwartung (bvset B) -- Hash weicht ab

    // ERWEITERUNG: recorded A subset current B; die Bindung reproduziert recorded_fp aus GENAU A.
    auto const alt = lege_binary_mit_sidecar(dir, "alt.dll", recorded_fp, kBvsetA);

    ex::SkipBvsetKontext erweiterung;
    erweiterung.current_glied = kBvsetB;
    erweiterung.recompute     = [recorded_fp](std::string const& bvset) {
        return (bvset == kBvsetA) ? recorded_fp : hexkey('f'); // Bindung: nur die ECHTE Menge reproduziert
    };
    auto const e1 = ex::entscheide_selektiven_rebuild(alt, current_fp, erweiterung, false);
    EXPECT_FALSE(e1.bauen) << "Erweiterung (A subset B) mit gebundener Aufzeichnung muss skippen";
    EXPECT_EQ(e1.grund, ex::RebuildGrund::skip_stempel_aktuell);

    // EINSCHRAENKUNG: dieselben Mengen vertauscht -- die Binary wurde mit B gebaut, der Treiber
    // fuehrt nur noch A. Sie traegt Faehigkeiten, die es nicht mehr gibt => Neubau.
    auto const breit = lege_binary_mit_sidecar(dir, "breit.dll", recorded_fp, kBvsetB);

    ex::SkipBvsetKontext einschraenkung;
    einschraenkung.current_glied = kBvsetA;
    einschraenkung.recompute     = [recorded_fp](std::string const& bvset) {
        return (bvset == kBvsetB) ? recorded_fp : hexkey('f');
    };
    auto const e2 = ex::entscheide_selektiven_rebuild(breit, current_fp, einschraenkung, false);
    EXPECT_TRUE(e2.bauen) << "Einschraenkung (current subset recorded) darf NIE skippen";
    EXPECT_EQ(e2.grund, ex::RebuildGrund::neubau_stempel_abweichung);

    // GEBROCHENE BINDUNG: Richtung stimmt (A subset B), aber die Neuberechnung reproduziert den
    // aufgezeichneten Hash NICHT => Neubau (Zusicherung (b) des #59-Vertrags).
    ex::SkipBvsetKontext gebrochen;
    gebrochen.current_glied = kBvsetB;
    gebrochen.recompute     = [](std::string const&) { return hexkey('f'); };
    auto const e3           = ex::entscheide_selektiven_rebuild(alt, current_fp, gebrochen, false);
    EXPECT_TRUE(e3.bauen) << "ohne reproduzierte Bindung ist Zeile 2 eine unbeglaubigte Behauptung";
    EXPECT_EQ(e3.grund, ex::RebuildGrund::neubau_stempel_abweichung);
}

// ---------------------------------------------------------------------------
// (5) Der Ausweis: Zaehlwerk gegen FREMDEN Nenner (testlokale Entscheidungs-Liste) und die
// Abschlusszeile literal.
// ---------------------------------------------------------------------------
TEST(C13SelektiverRebuild, AusweisZaehltGegenFremdenNenner) {
    auto const dir = frisches_testdir("ausweis");

    // Die Flotte dieses Tests: 5 Faelle, als LISTE angelegt -- ihr size() ist der fremde Nenner.
    auto const aktuell    = lege_binary_mit_sidecar(dir, "aktuell.dll", hexkey('a'));
    auto const abweichend = lege_binary_mit_sidecar(dir, "abweichend.dll", hexkey('b'));
    struct Fall {
        fs::path    output;
        std::string expected;
        bool        flotten_neubau{}; // NSDMI: cppcheck uninitMemberVarNoCtor (CI 16095 383082); Aggregat-Init bleibt
    };
    std::vector<Fall> const flotte = {
        {aktuell, hexkey('a'), false},             // -> skip
        {abweichend, hexkey('a'), false},          // -> neubau_stempel_abweichung
        {dir / "fehlt-1.dll", hexkey('a'), false}, // -> neubau_ohne_bestand
        {dir / "fehlt-2.dll", hexkey('a'), true},  // -> neubau_systemwechsel
        {aktuell, hexkey('a'), true},              // -> neubau_systemwechsel (Koeder-Fall verbucht)
    };

    ex::SelektiverRebuildAusweis ausweis;
    for (auto const& f : flotte)
        ausweis.verbuche(
            ex::entscheide_selektiven_rebuild(f.output, f.expected, ex::SkipBvsetKontext{}, f.flotten_neubau));

    // FREMDER NENNER: die Summe muss der testlokalen Fenstergroesse entsprechen.
    EXPECT_EQ(ausweis.entschieden(), flotte.size());
    EXPECT_EQ(ausweis.geskippt, std::size_t{1});
    EXPECT_EQ(ausweis.geschrieben, std::size_t{4});
    EXPECT_EQ(ausweis.systemwechsel_erzwungen, std::size_t{2});
    EXPECT_EQ(ausweis.ohne_bestand, std::size_t{1});
    EXPECT_EQ(ausweis.stempel_abweichung, std::size_t{1});

    // Die AUSWEIS-PFLICHT literal (V36.E-Geist: "N geschrieben, M skipped, ...").
    EXPECT_EQ(ausweis.abschlusszeile(),
              "4 geschrieben, 1 geskippt, 2 davon systemwechsel-erzwungen (C-13 selektiver Rebuild)");

    // Grund-Texte des Ausweises bleiben benennbar (kein stummes Enum).
    EXPECT_EQ(ex::rebuild_grund_text(ex::RebuildGrund::skip_stempel_aktuell), "skip_stempel_aktuell");
    EXPECT_EQ(ex::rebuild_grund_text(ex::RebuildGrund::neubau_systemwechsel), "neubau_systemwechsel");
}
