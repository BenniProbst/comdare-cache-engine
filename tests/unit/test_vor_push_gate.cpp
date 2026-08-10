// test_vor_push_gate -- die Testwerkbank des VOR-PUSH-GATES (2026-08-10), Testklasse T-4.
// Google Test, laeuft in Debug UND Release.
//
// ===================================================================================================
// WAS HIER GEPRUEFT WIRD -- und was ausdruecklich NICHT
// ===================================================================================================
// GEPRUEFT wird der KERN aus tools/vor_push_gate/vor_push_gate.hpp: die Nenner-Erhebung, die
// Klassifikation und der Berichtssatz. Alle Faelle arbeiten auf HANDGESCHRIEBENEN YAML-Schnipseln
// im Test, nicht auf der echten .gitlab-ci.yml -- ein Test, dessen Eingabe der Pruefling selbst
// liefert oder der mit der Produktionsdatei mitwandert, ist per Konstruktion immer gruen.
//
// NICHT GEPRUEFT wird das Ausfuehren der Jobs (fork/exec, clang-format, cppcheck, gitleaks). Das
// haengt an Werkzeugen ausserhalb des Repos und gehoert nicht in einen Unit-Test. Der Bissbeweis
// dafuer ist am 10.08.2026 am Objekt gefahren worden (Rot-Lauf gegen 6c010cdc, Koeder aus
// /dev/urandom in einer UNBERUEHRTEN Datei, Gegenprobe ohne Koeder) und steht im Paketbericht.
//
// ===================================================================================================
// DIE ZWEI FAELLE, DIE DAS WERKZEUG TRAGEN
// ===================================================================================================
// (1) DER NENNER MUSS SICH BEWEGEN LASSEN. Ein Nenner, den man nicht bewegen kann, ist eine
//     Konstante und kein Nenner. Zwei Faelle fahren das in BEIDE Richtungen: ein Job weg -> die
//     Zahl faellt; ein Job dazu -> die Zahl steigt UND der neue Job steht automatisch in der Menge
//     der nicht gefahrenen. Waere die Job-Liste im Werkzeug fest verdrahtet, blieben beide stumm.
//
// (2) DER BERICHTSSATZ MUSS BEIDE MENGEN NENNEN. Ein Gate, das "alles gruen" meldet, ohne zu
//     sagen WELCHE Jobs es gefahren hat, ist genau der Fehler, den dieses Werkzeug verhindern
//     soll -- eine Ebene hoeher. Der Fall haelt den Satz an beiden Zahlen und an den Namen fest.
//
// ASCII-only, Zeilen <= 120 Byte.

#include "vor_push_gate.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace vpg = comdare::vor_push_gate;

namespace {

// Ein kleiner, VOLLSTAENDIG im Test stehender Ausschnitt in der Bauart der echten Datei:
// reservierte Schluessel, eine versteckte Vorlage, ein Job mit eigenem Skript, ein Bau-Job,
// ein Erbe einer fremden Vorlage.
std::vector<std::string> beispiel_yml() {
    return {
        "stages:",    //  reserviert
        "  - lint",   //
        "variables:", //  reserviert
        "  COMDARE_LINT_PATHS: \"libs apps tests\"",
        ".bare_metal:",              //  Vorlage (Punkt-Praefix) -- KEIN Job
        "  before_script:",          //
        "    - cmake --version",     //
        "lint:eigen:",               //  Job, eigenes Skript, kein Bau
        "  stage: lint",             //
        "  script:",                 //
        "    - sh scripts/wache.sh", //
        "test:bau:",                 //  Job, eigenes Skript MIT Bau
        "  script:",                 //
        "    - cmake -B build -G Ninja",
        "lint:geerbt:",            //  Job, Skript liegt in einer fremden Vorlage
        "  extends: .lint-format", //
    };
}

std::size_t anzahl_jobs(const std::vector<std::string>& yml) { return vpg::job_bloecke(yml).size(); }

} // namespace

// ---------------------------------------------------------------------------------------------
// (1) DER NENNER
// ---------------------------------------------------------------------------------------------

TEST(VorPushGateNenner, ZaehltGenauDieJobsUndKeineSchluesselwoerter) {
    const auto jobs = vpg::job_bloecke(beispiel_yml());
    ASSERT_EQ(jobs.size(), 3u) << "erwartet: lint:eigen, test:bau, lint:geerbt";
    EXPECT_EQ(jobs[0].name, "lint:eigen");
    EXPECT_EQ(jobs[1].name, "test:bau");
    EXPECT_EQ(jobs[2].name, "lint:geerbt");
}

TEST(VorPushGateNenner, ReservierteSchluesselwoerterSindKeineJobs) {
    // 'stages:' und 'variables:' sehen syntaktisch aus wie Jobs. Zaehlten sie mit, waere der
    // Nenner zu gross und der Bruch N/M eine Luege in die andere Richtung.
    EXPECT_FALSE(vpg::job_kopf("stages:").has_value());
    EXPECT_FALSE(vpg::job_kopf("variables:").has_value());
    EXPECT_FALSE(vpg::job_kopf("include:").has_value());
    EXPECT_FALSE(vpg::job_kopf("workflow:").has_value());
    EXPECT_FALSE(vpg::job_kopf("cache:").has_value());
    EXPECT_TRUE(vpg::job_kopf("lint:format:").has_value());
}

TEST(VorPushGateNenner, VersteckteVorlagenMitPunktSindKeineJobs) {
    // '.bare_metal' und '.ccache-pull' sind YAML-Vorlagen, GitLab startet sie nie als Job.
    EXPECT_FALSE(vpg::job_kopf(".bare_metal:").has_value());
    EXPECT_FALSE(vpg::job_kopf(".ccache-pull:").has_value());
    EXPECT_FALSE(vpg::job_kopf(".lint-format:").has_value());
}

TEST(VorPushGateNenner, SchluesselMitWertOeffnetKeinenJob) {
    // 'image: alpine' ist ein Skalar, kein Job-Block.
    EXPECT_FALSE(vpg::job_kopf("image: alpine").has_value());
    EXPECT_FALSE(vpg::job_kopf("  lint:format:").has_value()) << "eingerueckt = nicht oberste Ebene";
}

// DIE ABNAHME-FORDERUNG, RICHTUNG 1: einen Job aus der Quelle entfernen -> die Zahl faellt.
TEST(VorPushGateNenner, EinJobWenigerSenktDieZahl) {
    const std::vector<std::string> voll   = beispiel_yml();
    const std::size_t              vorher = anzahl_jobs(voll);

    std::vector<std::string> ohne;
    bool                     im_entfernten = false;
    for (const auto& z : voll) {
        if (!z.empty() && z[0] != ' ' && z[0] != '\t') { im_entfernten = (z == "test:bau:"); }
        if (!im_entfernten) { ohne.push_back(z); }
    }

    const std::size_t nachher = anzahl_jobs(ohne);
    EXPECT_EQ(vorher, 3u);
    EXPECT_EQ(nachher, 2u) << "der Nenner MUSS sich bewegen -- sonst ist er eine Konstante";
    for (const auto& j : vpg::job_bloecke(ohne)) { EXPECT_NE(j.name, "test:bau"); }
}

// DIE ABNAHME-FORDERUNG, RICHTUNG 2 -- die wichtigere: ein NEUER Job in der Quelle waechst in den
// Nenner hinein, OHNE dass im Werkzeug eine Zeile dazukommt, und landet automatisch in der Menge
// der nicht gefahrenen. Genau das kann eine fest verdrahtete Liste nicht.
TEST(VorPushGateNenner, EinNeuerJobWaechstVonAlleinInDenNenner) {
    std::vector<std::string> mehr = beispiel_yml();
    mehr.push_back("lint:brandneu:");
    mehr.push_back("  script:");
    mehr.push_back("    - sh scripts/gibt_es_noch_nicht.sh");

    const auto jobs = vpg::job_bloecke(mehr);
    ASSERT_EQ(jobs.size(), 4u);
    EXPECT_EQ(jobs.back().name, "lint:brandneu");

    const std::vector<vpg::Lauf> laeufe{
        {"lint:eigen", vpg::Deckung::gruen, ""},
        {"test:bau", vpg::Deckung::braucht_bau, ""},
        {"lint:geerbt", vpg::Deckung::kein_laeufer, ""},
        {"lint:brandneu", vpg::Deckung::kein_laeufer, ""},
    };
    const std::string satz = vpg::nenner_satz(laeufe);
    EXPECT_NE(satz.find("1 von 4"), std::string::npos) << satz;
    EXPECT_NE(satz.find("lint:brandneu"), std::string::npos)
        << "ein neuer Job MUSS namentlich in der Menge der nicht gefahrenen stehen: " << satz;
}

// ---------------------------------------------------------------------------------------------
// DIE BLOCK-REGEL -- der Fehlgriff vom 10.08.2026, eingefroren
// ---------------------------------------------------------------------------------------------

TEST(VorPushGateBlock, KommentarInSpalteNullBeendetDenBlock) {
    // BEIM KALIBRIEREN DIESES WERKZEUGS PASSIERT (10.08.2026): ein awk-Probelauf hielt
    // lint:xml-wellformed fuer einen Bau-Job, weil der Kommentarkopf des NAECHSTEN Jobs das Wort
    // 'ctest' traegt und der Block ueber die Kommentarzeile hinweglief. Eine Zeile in SPALTE 0
    // beendet den Block -- Kommentarzeilen ausdruecklich eingeschlossen.
    const std::vector<std::string> yml{
        "lint:harmlos:",
        "  script:",
        "    - sh scripts/wache.sh",
        "# Dieser Kommentar gehoert schon zum naechsten Job und nennt cmake und ctest.",
        "test:bau:",
        "  script:",
        "    - cmake -B build",
    };
    const auto jobs = vpg::job_bloecke(yml);
    ASSERT_EQ(jobs.size(), 2u);

    // AM BLOCKINHALT gemessen, NICHT am Urteil. Erste Fassung dieses Falls pruefte nur
    // bauwunsch() -- und blieb gegen den Mutanten "'#' beendet den Block nicht mehr" GRUEN,
    // weil bauwunsch() Kommentarzeilen ohnehin ein zweites Mal ueberspringt. Ein Test, der
    // durch eine zweite Verteidigungslinie gedeckt wird, misst die erste nicht. Der Mutant
    // wurde am 10.08.2026 gefahren; gegen DIESE Fassung faellt er.
    EXPECT_EQ(jobs[0].zeilen.size(), 2u) << "erwartet nur '  script:' und '    - sh ...'";
    for (const auto& z : jobs[0].zeilen) {
        EXPECT_EQ(z.find('#'), std::string::npos)
            << "eine Zeile in Spalte 0 beendet den Block -- auch ein Kommentar: [" << z << "]";
    }

    EXPECT_EQ(vpg::bauwunsch(jobs[0]).wunsch, vpg::Bauwunsch::ohne_bau)
        << "der Kommentarkopf des Nachbarn darf lint:harmlos nicht zum Bau-Job machen";
    EXPECT_EQ(vpg::bauwunsch(jobs[1]).wunsch, vpg::Bauwunsch::braucht_bau);
}

TEST(VorPushGateBlock, CtestImJobEigenenKommentarIstKeinBau) {
    const std::vector<std::string> yml{
        "lint:harmlos:",
        "  # frueher lief hier ein ctest, heute nicht mehr -- reiner Textscan",
        "  script:",
        "    - sh scripts/wache.sh",
    };
    const auto jobs = vpg::job_bloecke(yml);
    ASSERT_EQ(jobs.size(), 1u);
    EXPECT_EQ(vpg::bauwunsch(jobs[0]).wunsch, vpg::Bauwunsch::ohne_bau);
}

// ---------------------------------------------------------------------------------------------
// FAIL-CLOSED: Unkenntnis ist NICHT Harmlosigkeit
// ---------------------------------------------------------------------------------------------

TEST(VorPushGateKlassifikation, FremdeVorlageGiltAlsUnbekanntNichtAlsOhneBau) {
    const auto jobs = vpg::job_bloecke(beispiel_yml());
    ASSERT_EQ(jobs.size(), 3u);
    const vpg::Befund b = vpg::bauwunsch(jobs[2]); // lint:geerbt, extends: .lint-format
    EXPECT_EQ(b.wunsch, vpg::Bauwunsch::unbekannt)
        << "ein Job, dessen Skript hier nicht steht, darf nicht als harmlos durchgehen";
    EXPECT_NE(b.beleg.find(".lint-format"), std::string::npos) << b.beleg;
    EXPECT_EQ(vpg::extends_ziel(jobs[2]), ".lint-format");
}

TEST(VorPushGateKlassifikation, MehrzeiligesBlockskriptIstNichtVerbatimFahrbar) {
    // '- |' ist ein Blockskript. Es verbatim nachzufahren waere eine stille Abweichung, also
    // liefert der Kern NICHTS und der Aufrufer faellt auf "kein Laeufer" -- fail-closed.
    const std::vector<std::string> yml{
        "lint:block:", "  script:", "    - |", "      set -euo pipefail", "      echo hallo",
    };
    const auto jobs = vpg::job_bloecke(yml);
    ASSERT_EQ(jobs.size(), 1u);
    EXPECT_TRUE(vpg::script_kommandos(jobs[0]).empty());
}

TEST(VorPushGateKlassifikation, EinzeiligeKommandosWerdenAusgelesen) {
    const auto jobs = vpg::job_bloecke(beispiel_yml());
    ASSERT_EQ(jobs.size(), 3u);
    const std::vector<std::string> k = vpg::script_kommandos(jobs[0]);
    ASSERT_EQ(k.size(), 1u);
    EXPECT_EQ(k[0], "sh scripts/wache.sh");
}

// ---------------------------------------------------------------------------------------------
// DIE LINT-MENGE -- die Stelle, an der die alte Wache vom CI-Job abwich
// ---------------------------------------------------------------------------------------------

TEST(VorPushGateLintMenge, FiltertEndungenUndAusschlussWieDieVorlage) {
    const std::string ausschluss = R"((^|/)(ext|build|_archive_code_pre_migration|cmake-build-[^/]*|modules)/)";
    const std::vector<std::string> versioniert{
        "libs/a.hpp",         // nimmt
        "libs/b.cpp",         // nimmt
        "libs/c.txt",         // faellt: falsche Endung
        "libs/ext/d.hpp",     // faellt: Ausschluss
        "build/e.cpp",        // faellt: Ausschluss
        "libs/modules/f.hpp", // faellt: Ausschluss
        "libs/cmake-build-x/g.cpp",
        "apps/h.cc", // nimmt
        "",          // faellt: leer
    };
    const auto treffer = vpg::lint_dateien(versioniert, ausschluss);
    ASSERT_EQ(treffer.size(), 3u);
    EXPECT_EQ(treffer[0], "libs/a.hpp");
    EXPECT_EQ(treffer[1], "libs/b.cpp");
    EXPECT_EQ(treffer[2], "apps/h.cc");
}

TEST(VorPushGateVariable, LiestCOMDARELINTPATHSAusDerDatei) {
    const auto wert = vpg::variable(beispiel_yml(), "COMDARE_LINT_PATHS");
    ASSERT_TRUE(wert.has_value());
    EXPECT_EQ(*wert, "libs apps tests");
    EXPECT_FALSE(vpg::variable(beispiel_yml(), "GIBT_ES_NICHT").has_value());
}

// ---------------------------------------------------------------------------------------------
// (2) DER BERICHTSSATZ -- die Hauptsache
// ---------------------------------------------------------------------------------------------

TEST(VorPushGateBericht, NenntImmerBeideMengenMitZahlUndNamen) {
    const std::vector<vpg::Lauf> laeufe{
        {"lint:format", vpg::Deckung::gruen, ""},
        {"lint:static", vpg::Deckung::rot, ""},
        {"test:unit", vpg::Deckung::braucht_bau, ""},
        {"pmc:amd", vpg::Deckung::kein_laeufer, ""},
    };
    const std::string satz = vpg::nenner_satz(laeufe);
    EXPECT_NE(satz.find("2 von 4"), std::string::npos) << satz;
    EXPECT_NE(satz.find("test:unit"), std::string::npos) << satz;
    EXPECT_NE(satz.find("pmc:amd"), std::string::npos) << satz;
    EXPECT_EQ(satz.find("lint:format"), std::string::npos) << "gefahrene gehoeren nicht in die Restmenge";
}

TEST(VorPushGateBericht, AbweichendZaehltNichtAlsGedeckt) {
    // Ein Job mit ANDEREN Optionen ist nicht derselbe Job. Zaehlte 'abweichend' im Zaehler, waere
    // die Zahl eine Aussage ueber einen anderen Lauf als den, der zaehlt.
    EXPECT_FALSE(vpg::zaehlt_als_gefahren(vpg::Deckung::abweichend));
    EXPECT_FALSE(vpg::zaehlt_als_gefahren(vpg::Deckung::werkzeug_fehlt));
    EXPECT_FALSE(vpg::zaehlt_als_gefahren(vpg::Deckung::braucht_bau));
    EXPECT_FALSE(vpg::zaehlt_als_gefahren(vpg::Deckung::kein_laeufer));
    EXPECT_TRUE(vpg::zaehlt_als_gefahren(vpg::Deckung::gruen));
    EXPECT_TRUE(vpg::zaehlt_als_gefahren(vpg::Deckung::rot));

    const std::vector<vpg::Lauf> laeufe{
        {"lint:format", vpg::Deckung::abweichend, "Vorlage gewechselt"},
        {"lint:static", vpg::Deckung::gruen, ""},
    };
    EXPECT_NE(vpg::nenner_satz(laeufe).find("1 von 2"), std::string::npos);
    EXPECT_NE(vpg::nenner_satz(laeufe).find("lint:format"), std::string::npos);
}

TEST(VorPushGateBericht, LeereRestmengeWirdBenanntStattVerschwiegen) {
    const std::vector<vpg::Lauf> laeufe{{"lint:format", vpg::Deckung::gruen, ""}};
    const std::string            satz = vpg::nenner_satz(laeufe);
    EXPECT_NE(satz.find("1 von 1"), std::string::npos) << satz;
    EXPECT_NE(satz.find("(keine)"), std::string::npos) << satz;
}

TEST(VorPushGateBericht, JedeDeckungHatEinenText) {
    // Ein stummer Zweig waere ein verdeckter Ausgang: die Zeile im Bericht bliebe leer und der
    // Leser hielte den Job fuer geprueft.
    for (const auto d : {vpg::Deckung::gruen, vpg::Deckung::rot, vpg::Deckung::abweichend, vpg::Deckung::braucht_bau,
                         vpg::Deckung::kein_laeufer, vpg::Deckung::werkzeug_fehlt}) {
        EXPECT_FALSE(vpg::deckung_text(d).empty());
    }
}
