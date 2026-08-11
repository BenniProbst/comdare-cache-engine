// test_t6_wachen_inventar -- DIE NEUERHEBUNG DER T-6-LUECKE, als Werkzeug.  (2026-08-10)
// =============================================================================
// WAS DIESER TEST IST -- und was er ausdruecklich NICHT ist:
// Er ist KEIN Bericht. Der Nenner steht in SEINER AUSGABE (V-1), er wird bei jedem
// ctest-Lauf neu erhoben, und er ist FAIL-CLOSED: eine neue CI-gerufene Wache ohne
// Eintrag macht ihn ROT. Eine Zahl in einer Markdown-Datei kann veralten, ohne dass
// irgendetwas klappert; diese hier kann es nicht.
//
// DER BEFUND, GEGEN DEN ER GEBAUT IST:
// Eine frueher gemessene Zahl nannte "27 CI-gerufene Wachen ohne Selbsttest". Eine
// Wache ohne Selbsttest ist eine Wache, deren Biss niemand je geprueft hat -- sie kann
// seit Monaten gruen melden, ohne je etwas gemessen zu haben. Genau diese Klasse hat
// am 09.08.2026 viermal an einem Tag zugeschlagen.
//
// DIE GRUNDGESAMTHEIT KOMMT VON AUSSEN (T-3 / V-7). Beide Nenner dieses Tests stammen
// aus einer ANDEREN Quelle als dem Prueflig:
//   KLASSE A  .gitlab-ci.yml -- WER wird von der CI wirklich gerufen. Nicht der
//             Datei-Bestand unter scripts/: ein Skript, das niemand ruft, ist keine
//             Wache, und ein Skript, das ohne Ausfuehrungs-Bit dasteht, ist eine.
//             AM OBJEKT GEMESSEN (10.08., git ls-tree ueber scripts/): 11 der 20
//             scripts/*.sh tragen Modus 100644 und werden per `sh <datei>` gerufen.
//             Von den NEUN Wachen unten sind DREI davon betroffen
//             (ci_diff_ascii_width_guard, ci_test_coverage_guard, ci_yaml_key_guard) --
//             eine Erhebung ueber das Ausfuehrungs-Bit haette also 3 von 9 Wachen
//             unterschlagen und einen Nenner von 6 gemeldet.
//   KLASSE C  tests/unit/CMakeLists.txt + cli_smoke.cmake -- die ctest-Eintraege mit
//             PASS_REGULAR_EXPRESSION.
//
// DER SELBST-NENNER WAERE HIER DIE FALLE, und sie ist am Objekt vorgefuehrt: ein
// rohes `grep -c PASS_REGULAR_EXPRESSION` ueber dieselben zwei Dateien liefert 10.
// WIRKSAM sind 3 -- die uebrigen 7 stehen in Kommentaren, die genau erklaeren, warum
// die Eigenschaft hier richtig oder falsch ist. Wer den Nenner roh zaehlt, meldet die
// dreifache Abdeckung. Dieser Test druckt BEIDE Zahlen nebeneinander.
//
// DAS DECKUNGS-KRITERIUM, ausdruecklich benannt (T-2: Aussage, nicht Anwesenheit):
//   GTEST  ein Google Test, der den BISS der Wache faehrt (Wache entfernen -> rot,
//          wieder einbauen -> gruen). Er muss als Quelldatei existieren UND in
//          tests/unit/CMakeLists.txt registriert sein -- T-7: ein Test existiert erst,
//          wenn er in `ctest -N` erscheint. Beides wird hier am Objekt nachgesehen.
//   SHELL  ein <stamm>.selbsttest.sh, der SELBST von der CI gerufen wird. Ein
//          Selbsttest, den niemand ruft, deckt nichts -- deshalb wird auch dafuer die
//          .gitlab-ci.yml befragt und nicht der Datei-Bestand.
//          Diese Form ist die ABZULOESENDE (Owner 09.08.: "SKRIPTE SAGEN GAR NICHTS")
//          und wird deshalb getrennt gezaehlt, nicht mit GTEST verrechnet.
//   KEINE  ungedeckt. Das ist die Warteliste, und sie hat hier eine ZAHL.
//
// WAS IHN ROT MACHT -- vier Wege, jeder davon ein echter Regress:
//   (1) eine neue Wache steht in der .gitlab-ci.yml und nicht in der Tabelle
//   (2) ein Tabellen-Eintrag steht nicht mehr in der .gitlab-ci.yml (tote Zeile)
//   (3) eine als GTEST gedeckte Wache verliert ihren Test oder dessen Registrierung
//   (4) die Zahl der ungedeckten Wachen steigt ueber die eingetragene Obergrenze
//
// V-8 -- "Was waere der Zustand, in dem diese Ausgabe erscheint und die Sache trotzdem
// nicht existiert?": ein GTEST-Eintrag, dessen Testdatei zwar daliegt, aber leer ist.
// Dagegen hilft diese Datei nicht -- dagegen hilft, dass jeder der drei neuen Tests
// seinen Biss selbst mit Koeder und Gegenprobe vorfuehrt. Das ist ausdruecklich die
// Arbeitsteilung und keine Luecke, die hier zugedeckt wird.
//
// TESTKRITIK (T-9): der Zeilen-Scanner unten ist kein YAML-Parser. Er schneidet jede
// Zeile am ERSTEN '#' ab -- ein Skript-Pfad hinter einem '#' in einer Zeichenkette
// wuerde uebersehen. In dieser Datei gibt es keinen solchen Fall (nachgesehen), und
// die abgeloeste `grep`-Erhebung konnte genau dasselbe nicht. Kein Rueckschritt, aber
// benannt. Ebenso: `include:`-Dateien und YAML-Anker sieht er nicht.
//
// ASCII-only, Zeilen <= 120 Byte.
// =============================================================================

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

// ---------------------------------------------------------------------------
// DIE TABELLE. Sie ist das LITERAL dieses Tests -- bewusst nicht aus dem Repo
// abgeleitet. Wer eine Wache hinzufuegt oder ihre Deckung entfernt, muss diese
// Datei mit anfassen; das ist der Zweck, nicht die Huerde. Dieselbe Bauart traegt
// scripts/ci_achsen_roundtrip_wache.sh, und aus demselben Grund (V-7): zwei Zahlen
// aus derselben Quelle sind kein Vergleich.
// ---------------------------------------------------------------------------
enum class Art {
    Wache,      // faellt ein Urteil und beendet den Job -> gehoert in den Nenner
    Selbsttest, // ist selbst die Deckung einer Wache    -> gehoert NICHT in den Nenner
    Datenquelle // wird gesourct, urteilt nicht          -> gehoert NICHT in den Nenner
};

enum class Deckung { Gtest, Shell, Keine };

// Die zwei Vorgaben sind KEINE Formsache, sie zeigen fail-closed. Wer eine Zeile ergaenzt und
// ein Feld vergisst, bekommt: art=Wache -> die Zeile zaehlt in den Nenner und verlangt Deckung
// (Selbsttest/Datenquelle wuerden sie still herausnehmen); deckung=Keine -> sie landet auf der
// Warteliste, und die steht mit 3 von 3 bereits an ihrer Obergrenze, faellt also auf. Das
// wertfreie '= {}' waere hier genau falsch: es hiesse Gtest, also BEHAUPTETE Deckung.
struct Eintrag {
    std::string_view pfad; // wie er in der .gitlab-ci.yml steht
    Art              art     = Art::Wache;
    Deckung          deckung = Deckung::Keine;
    std::string_view beleg; // repo-relativ; bei Gtest die .cpp, bei Shell der Selbsttest
    std::string_view ziel;  // bei Gtest: der ctest-Zielname, der registriert sein MUSS
    std::string_view bemerkung;
};

constexpr Eintrag kTabelle[] = {
    // -- gedeckt in der HAUSFORM (Google Test) ------------------------------------
    {"scripts/ci_test_coverage_guard.sh", Art::Wache, Deckung::Gtest, "tests/unit/test_d2_abdeckungs_wache_nenner.cpp",
     "test_d2_abdeckungs_wache_nenner", "D2 09.08.: der Nenner-Zweig. Traegt zusaetzlich noch einen Shell-Selbsttest."},
    {"scripts/ci_test_sichtbarkeit_wache.sh", Art::Wache, Deckung::Gtest,
     "tests/unit/test_t6_biss_sichtbarkeits_wache.cpp", "test_t6_biss_sichtbarkeits_wache",
     "P1 des kritischen Pfades: ohne sie war der Abdeckungs-Nenner um 27 zu klein."},
    {"scripts/ci_test_registrierungs_wache.sh", Art::Wache, Deckung::Gtest,
     "tests/unit/test_t6_biss_registrierungs_wache.cpp", "test_t6_biss_registrierungs_wache",
     "P2: Stufe VOR der Sichtbarkeits-Wache -- Quelldatei gegen Bauweg."},
    {"scripts/ci_achsen_roundtrip_wache.sh", Art::Wache, Deckung::Gtest,
     "tests/unit/test_t6_biss_achsen_roundtrip_wache.cpp", "test_t6_biss_achsen_roundtrip_wache",
     "P3: die einzige Wache mit literalem Nenner -- sie deckt die Selbst-Inventur der drei anderen."},

    // -- gedeckt in der ABZULOESENDEN Form (Shell-Selbsttest) ----------------------
    {"scripts/ci_diff_ascii_width_guard.sh", Art::Wache, Deckung::Shell,
     "scripts/ci_diff_ascii_width_guard.selbsttest.sh", "", "Umbau nach gtest offen -- Warteliste."},
    {"scripts/ci_test_bauweg_wache.sh", Art::Wache, Deckung::Shell, "scripts/ci_test_bauweg_wache.selbsttest.sh", "",
     "Umbau nach gtest offen -- Warteliste."},

    // -- UNGEDECKT: die Warteliste, mit Namen -------------------------------------
    {"scripts/ci_xml_wellformed_guard.sh", Art::Wache, Deckung::Keine, "", "",
     "NICHT auf dem kritischen Pfad (lint). In super existiert die gtest-Fassung bereits."},
    {"scripts/ci_yaml_key_guard.sh", Art::Wache, Deckung::Keine, "", "",
     "NICHT auf dem kritischen Pfad (lint auf .gitlab-ci.yml)."},
    {"scripts/lint_layer_includes.sh", Art::Wache, Deckung::Keine, "", "",
     "NICHT auf dem kritischen Pfad (Schichten-Lint)."},
    // Nachgetragen 11.08.2026: kam mit dem Stage-Topologie-Paket (#21) in die CI und wurde
    // von DIESEM Inventar rot gemeldet ("NEUE WACHE OHNE EINTRAG"), ce-Pipeline 15665.
    // Deckung SHELL, nicht Gtest: der Selbsttest daneben ist die abzuloesende Form -- sie zaehlt
    // hier bewusst NICHT als Hausform, damit die Warteliste ehrlich bleibt.
    {"scripts/ci_stage_topologie_wache.sh", Art::Wache, Deckung::Shell,
     "scripts/ci_stage_topologie_wache.selbsttest.sh", "",
     "Umbau nach gtest offen -- Warteliste. Prueft die Stufen-Ordnung: keine needs-Kante vorwaerts."},

    // -- keine Wachen: sie stehen hier, damit der Abgleich beidseitig aufgeht ------
    {"scripts/ci_diff_ascii_width_guard.selbsttest.sh", Art::Selbsttest, Deckung::Keine, "", "", ""},
    {"scripts/ci_stage_topologie_wache.selbsttest.sh", Art::Selbsttest, Deckung::Keine, "", "", ""},
    {"scripts/ci_test_bauweg_wache.selbsttest.sh", Art::Selbsttest, Deckung::Keine, "", "", ""},
    {"scripts/ci_test_coverage_guard.selbsttest.sh", Art::Selbsttest, Deckung::Keine, "", "", ""},
    {"scripts/ci_test_coverage_manifest.sh", Art::Datenquelle, Deckung::Keine, "", "",
     "wird per '.' gesourct und setzt Variablen -- faellt kein Urteil."},
};

// DIE OBERGRENZE DER WARTELISTE. Sie ist die ZAHL, die den Posten am Leben haelt --
// 'wir achten darauf' gilt als nicht abgenommen. Sie darf nur SINKEN; jede neue
// ungedeckte Wache hebt den Ist-Wert darueber und macht diesen Test rot.
constexpr std::size_t kWartelisteObergrenze = 3;

// Die drei ctest-Eintraege mit PASS_REGULAR_EXPRESSION sind allesamt ungedeckt --
// niemand prueft, ob diese Negativ-Fixturen wirklich beissen. Sie stehen als eigener,
// gezaehlter Posten auf der Warteliste und werden hier nur ERHOBEN, nicht behauptet.
constexpr std::size_t kKlasseCObergrenze = 3;

// ---------------------------------------------------------------------------
// Der Zeilen-Scanner. Wirksam ist, was VOR dem ersten '#' steht.
// ---------------------------------------------------------------------------
[[nodiscard]] std::vector<std::string> lies_zeilen(fs::path const& datei) {
    std::vector<std::string> zeilen;
    std::ifstream            ein{datei};
    if (!ein.good()) { return zeilen; }
    std::string z;
    while (std::getline(ein, z)) {
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        zeilen.push_back(z);
    }
    return zeilen;
}

[[nodiscard]] std::string wirksamer_teil(std::string const& zeile) {
    std::size_t const raute = zeile.find('#');
    return raute == std::string::npos ? zeile : zeile.substr(0, raute);
}

[[nodiscard]] bool ist_pfadzeichen(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' ||
           c == '-';
}

// Alle 'scripts/<name>.sh' aus dem WIRKSAMEN Teil der Datei, ohne Doppelte.
[[nodiscard]] std::set<std::string> skript_aufrufe(std::vector<std::string> const& zeilen) {
    static constexpr std::string_view kPraefix = "scripts/";
    std::set<std::string>             treffer;
    for (auto const& roh : zeilen) {
        std::string const zeile = wirksamer_teil(roh);
        std::size_t       pos   = 0;
        while ((pos = zeile.find(kPraefix, pos)) != std::string::npos) {
            std::size_t ende = pos + kPraefix.size();
            while (ende < zeile.size() && ist_pfadzeichen(zeile[ende])) { ++ende; }
            std::string const kandidat = zeile.substr(pos, ende - pos);
            if (kandidat.size() > 3 && kandidat.compare(kandidat.size() - 3, 3, ".sh") == 0) {
                treffer.insert(kandidat);
            }
            pos = ende;
        }
    }
    return treffer;
}

// Wirksame vs. rohe Treffer einer Zeichenfolge -- die Gegenprobe des Nenners.
struct Zaehlung {
    std::size_t wirksam{0};
    std::size_t roh{0};
};

[[nodiscard]] Zaehlung zaehle(std::vector<std::string> const& zeilen, std::string_view nadel) {
    Zaehlung z;
    for (auto const& roh : zeilen) {
        if (roh.find(nadel) != std::string::npos) { ++z.roh; }
        if (wirksamer_teil(roh).find(nadel) != std::string::npos) { ++z.wirksam; }
    }
    return z;
}

[[nodiscard]] fs::path wurzel() { return fs::path{COMDARE_T6_QUELLBAUM}; }

[[nodiscard]] char const* deckungs_wort(Deckung d) {
    switch (d) {
        case Deckung::Gtest: return "GTEST";
        case Deckung::Shell: return "SHELL";
        case Deckung::Keine: return "KEINE";
    }
    return "?";
}

} // namespace

// =============================================================================
// (1) DIE ERHEBUNG SELBST. Sie druckt beide Zahlen und faellt, wenn die Warteliste
//     waechst. Das ist der Posten mit ZAHL, nicht das stille Vergessen.
// =============================================================================
TEST(T6WachenInventar, NennerUndWartelisteStehenInDerAusgabe) {
    fs::path const ci_datei = wurzel() / ".gitlab-ci.yml";
    ASSERT_TRUE(fs::exists(ci_datei)) << "Die Grundgesamtheit kommt aus '" << ci_datei.string()
                                      << "'. Fehlt sie, ist das ABBRUCH und ausdruecklich kein Gruen.";

    std::vector<std::string> const ci_zeilen = lies_zeilen(ci_datei);
    ASSERT_FALSE(ci_zeilen.empty()) << "leere .gitlab-ci.yml -- der Nenner waere null und jede Aussage wahr";

    std::set<std::string> const gerufen = skript_aufrufe(ci_zeilen);
    ASSERT_FALSE(gerufen.empty()) << "0 Skript-Aufrufe gefunden -- das Suchmuster greift nicht (fail-closed)";

    // GEGENPROBE DES MESSGERAETS (V4): eine Zeichenfolge, von der wir wissen, dass die
    // CI sie ruft, MUSS treffen. Trifft sie nicht, ist nicht die Tabelle falsch,
    // sondern die Messung -- und dann darf dieser Test kein Urteil faellen.
    ASSERT_TRUE(gerufen.count("scripts/ci_test_coverage_guard.sh") == 1U)
        << "Die Gegenprobe greift nicht: der Scanner findet den bekannten Aufruf nicht.";

    // -- Abgleich Richtung 1: jede gerufene Datei steht in der Tabelle --------------
    std::vector<std::string> nicht_eingetragen;
    for (auto const& p : gerufen) {
        bool gefunden = false;
        for (auto const& e : kTabelle) {
            if (e.pfad == p) {
                gefunden = true;
                break;
            }
        }
        if (!gefunden) { nicht_eingetragen.push_back(p); }
    }

    // -- Abgleich Richtung 2: jeder Tabellen-Eintrag wird auch gerufen --------------
    std::vector<std::string> tote_zeilen;
    for (auto const& e : kTabelle) {
        if (gerufen.count(std::string{e.pfad}) == 0U) { tote_zeilen.emplace_back(e.pfad); }
    }

    // -- Die Zaehlung --------------------------------------------------------------
    std::size_t              wachen = 0;
    std::size_t              gtest  = 0;
    std::size_t              shell  = 0;
    std::vector<std::string> warteliste;
    for (auto const& e : kTabelle) {
        if (e.art != Art::Wache) { continue; }
        ++wachen;
        switch (e.deckung) {
            case Deckung::Gtest: ++gtest; break;
            case Deckung::Shell: ++shell; break;
            case Deckung::Keine: warteliste.emplace_back(e.pfad); break;
        }
    }

    std::cout << "-----------------------------------------------------------------------------\n"
              << "T-6-INVENTAR, KLASSE A -- CI-gerufene Wachen dieses Repos (ce)\n"
              << "  Quelle des Nenners : " << ci_datei.string() << " (fremd, T-3)\n"
              << "  Aufrufe insgesamt  : " << gerufen.size() << " Skript-Pfad(e), wirksam gerufen\n"
              << "  davon WACHEN       : " << wachen << "   (Rest: Selbsttests und Datenquellen)\n"
              << "  gedeckt GTEST      : " << gtest << "   (Hausform, Debug UND Release)\n"
              << "  gedeckt SHELL      : " << shell << "   (abzuloesende Form -- zaehlt NICHT als Hausform)\n"
              << "  UNGEDECKT          : " << warteliste.size() << "   Obergrenze " << kWartelisteObergrenze << "\n"
              << "-----------------------------------------------------------------------------\n";
    for (auto const& e : kTabelle) {
        if (e.art != Art::Wache) { continue; }
        std::cout << "  [" << deckungs_wort(e.deckung) << "] " << e.pfad << "\n";
        if (!e.beleg.empty()) { std::cout << "          Beleg: " << e.beleg << "\n"; }
    }
    std::cout << "WARTELISTE KLASSE A (" << warteliste.size() << "):\n";
    for (auto const& w : warteliste) { std::cout << "  - " << w << "\n"; }
    std::cout << "-----------------------------------------------------------------------------\n";

    EXPECT_TRUE(nicht_eingetragen.empty()) << [&] {
        std::string s = "NEUE WACHE OHNE EINTRAG -- die CI ruft sie, das Inventar kennt sie nicht:\n";
        for (auto const& p : nicht_eingetragen) { s += "  " + p + "\n"; }
        return s;
    }();
    EXPECT_TRUE(tote_zeilen.empty()) << [&] {
        std::string s = "TOTE TABELLEN-ZEILE -- eingetragen, aber von der CI nicht mehr gerufen:\n";
        for (auto const& p : tote_zeilen) { s += "  " + p + "\n"; }
        return s;
    }();
    EXPECT_LE(warteliste.size(), kWartelisteObergrenze)
        << "Die Warteliste ist gewachsen. Eine neue CI-gerufene Wache ohne Selbsttest ist genau die "
           "Klasse, gegen die dieser Posten gebaut ist.";
}

// =============================================================================
// (2) DIE DECKUNG WIRD AM GEGENSTAND NACHGESEHEN, NICHT GEGLAUBT (V-8).
//     Ein GTEST-Eintrag ist erst wahr, wenn die Quelldatei da ist UND das Ziel in
//     tests/unit/CMakeLists.txt registriert wird (T-7). Ein SHELL-Eintrag ist erst
//     wahr, wenn der Selbsttest existiert UND die CI ihn selbst ruft.
// =============================================================================
TEST(T6WachenInventar, JederDeckungsBelegExistiertUndIstRegistriert) {
    fs::path const cmake_datei = wurzel() / "tests/unit/CMakeLists.txt";
    ASSERT_TRUE(fs::exists(cmake_datei)) << cmake_datei.string();
    std::vector<std::string> const cmake_zeilen = lies_zeilen(cmake_datei);
    std::set<std::string> const    gerufen      = skript_aufrufe(lies_zeilen(wurzel() / ".gitlab-ci.yml"));

    std::size_t geprueft = 0;
    for (auto const& e : kTabelle) {
        if (e.deckung == Deckung::Keine) { continue; }
        ++geprueft;

        fs::path const beleg = wurzel() / std::string{e.beleg};
        EXPECT_TRUE(fs::exists(beleg)) << "Deckungs-Beleg fuer " << e.pfad << " fehlt: " << beleg.string();

        if (e.deckung == Deckung::Gtest) {
            // T-7: die Quelldatei genuegt nicht. Sie muss registriert sein, sonst
            // erscheint der Test in keinem 'ctest -N' und existiert nicht.
            std::string const marke    = "comdare_add_test(" + std::string{e.ziel};
            bool              gefunden = false;
            for (auto const& z : cmake_zeilen) {
                if (wirksamer_teil(z).find(marke) != std::string::npos) {
                    gefunden = true;
                    break;
                }
            }
            EXPECT_TRUE(gefunden) << "Ziel '" << e.ziel << "' ist in tests/unit/CMakeLists.txt nicht registriert -- "
                                  << "eine Deckung, die in keinem 'ctest -N' erscheint, ist keine.";
        } else {
            // Ein Selbsttest, den niemand ruft, deckt nichts.
            EXPECT_TRUE(gerufen.count(std::string{e.beleg}) == 1U)
                << "Der Selbsttest '" << e.beleg << "' wird von der .gitlab-ci.yml nicht gerufen.";
        }
    }
    ASSERT_GT(geprueft, 0U) << "0 Deckungs-Belege geprueft -- dann sagt dieser Fall nichts aus (fail-closed).";
    std::cout << "T-6-INVENTAR: " << geprueft << " Deckungs-Beleg(e) am Gegenstand nachgesehen.\n";
}

// =============================================================================
// (3) KLASSE C -- DER D4-QUERBEFUND. Hier ist der Nenner am leichtesten zu faelschen:
//     roh 10 Treffer, wirksam 3. Die Differenz sind Kommentare, die erklaeren, warum
//     die Eigenschaft an dieser Stelle richtig oder falsch ist -- wer roh zaehlt,
//     meldet die dreifache Abdeckung.
// =============================================================================
TEST(T6WachenInventar, KlasseCPassRegularExpressionRohGegenWirksam) {
    static constexpr std::string_view kNadel = "PASS_REGULAR_EXPRESSION";

    std::vector<fs::path> const quellen = {wurzel() / "tests/unit/CMakeLists.txt",
                                           wurzel() / "tests/unit/cli_smoke.cmake"};

    Zaehlung gesamt;
    for (auto const& q : quellen) {
        ASSERT_TRUE(fs::exists(q)) << "Quelle der Klasse C fehlt: " << q.string() << " -- ABBRUCH, kein Gruen.";
        Zaehlung const z = zaehle(lies_zeilen(q), kNadel);
        gesamt.wirksam += z.wirksam;
        gesamt.roh += z.roh;
        std::cout << "  " << q.filename().string() << ": roh " << z.roh << ", wirksam " << z.wirksam << "\n";
    }

    std::cout << "-----------------------------------------------------------------------------\n"
              << "T-6-INVENTAR, KLASSE C -- ctest-Eintraege mit PASS_REGULAR_EXPRESSION\n"
              << "  ROH (jede Zeile mit der Zeichenfolge) : " << gesamt.roh << "\n"
              << "  WIRKSAM (ausserhalb von Kommentaren)  : " << gesamt.wirksam << "\n"
              << "  davon mit Selbsttest                  : 0\n"
              << "  WARTELISTE KLASSE C                   : " << gesamt.wirksam << "\n"
              << "-----------------------------------------------------------------------------\n";

    // Die Aussage ist NICHT 'roh == 10' -- Kommentare duerfen sich aendern, und ein Test,
    // der an jeder Kommentarzeile faellt, ist ein Daueralarm (T-1). Die Aussage ist:
    // die beiden Zaehlweisen sind VERSCHIEDEN, und wer die rohe nimmt, ueberzaehlt.
    EXPECT_GT(gesamt.roh, gesamt.wirksam)
        << "Beide Zaehlweisen liefern dasselbe -- dann ist entweder der Kommentar-Bestand weg "
           "oder der Scanner erkennt Kommentare nicht mehr. Beides gehoert angesehen.";
    EXPECT_EQ(gesamt.wirksam, kKlasseCObergrenze)
        << "Die Zahl der Negativ-Compile-Fixturen hat sich geaendert. Jede davon ist eine Wache "
           "ohne Selbsttest; kommt eine dazu, gehoert sie auf die Warteliste -- nicht unbemerkt.";
}
