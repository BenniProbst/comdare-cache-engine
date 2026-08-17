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
// NE-16 (2026-08-13, #39-Klasse): geprueft wird EXAKT (EXPECT_EQ, nicht mehr LE).
// Sinkt die Ist-Zahl, wird diese Zahl im SELBEN Change mitgesenkt -- Nachzug-Pflicht
// statt Schlupf, sonst verschwaende die naechste neue ungedeckte Wache im frei
// gewordenen Fenster zwischen Ist und Obergrenze.
constexpr std::size_t kWartelisteObergrenze = 3;

// Die ctest-Eintraege mit PASS_REGULAR_EXPRESSION sind allesamt ungedeckt --
// niemand prueft, ob diese Negativ-Fixturen wirklich beissen. Sie stehen als eigener,
// gezaehlter Posten auf der Warteliste und werden hier nur ERHOBEN, nicht behauptet.
//
// NACHZUG 17.08.2026 (HY-A1-Dock-Schicht): 3 -> 4. Neu hinzugekommen ist
// test_hy_a1_contract_token_negativ (Koeder contract_f0a01c9e gegen die Vertrags-Registry).
// Die Wache hat den Zuwachs GEFANGEN -- genau ihre Aufgabe: "kommt eine dazu, gehoert sie auf
// die Warteliste, nicht unbemerkt". Die Zahl wird deshalb als QUITTUNG mitgezogen, nicht als
// Aufweichung: der neue Eintrag ist damit auf der Warteliste GEZAEHLT und nicht gedeckt.
// Was ihm zur Deckung fehlt, ist derselbe Selbsttest wie den drei anderen -- ein Mechanismus,
// der belegt, dass die Fixture ohne den erwarteten Bruch ROT wird. Fuer diese eine Fixture ist
// der Biss am Objekt nachgemessen (3 Sperren, 2 davon mit Marker in der Diagnose, im Commit
// protokolliert); ein maschineller Selbsttest ist das NICHT und wird hier nicht als solcher
// gezaehlt.
//
// NACHTRAG 17.08.2026 (A2.5-FUND-11) -- WORIN DIE DECKUNGSLUECKE GENAU BESTEHT, damit der
// Warteliste-Posten seinen Gegenstand kennt: die Negativ-Fixturen sind EIN-TU-Bauformen. Ein
// PASS_REGULAR_EXPRESSION prueft, DASS ein Marker in der Diagnose steht -- nicht, WELCHE der
// Sperren ihn erzeugt hat. Faellt Sperre 3 weg, waehrend Sperre 1 weiter bricht, bleibt der
// ctest GRUEN. Verschaerfend: test_hy_a1_contract_token_negativ dokumentiert selbst, dass seine
// Probe 2 gar keinen eigenen Marker druckt (consteval-throw zeigt auf die Wurf-Zeile im Header).
// Ein echter Selbsttest muesste je Sperre EINE eigene TU bauen und je einzeln belegen, dass ihr
// Wegfall rot wird. Das ist der Inhalt dieses Wartelisten-Postens -- nicht "irgendein Test fehlt".
constexpr std::size_t kKlasseCObergrenze = 4;

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

// ---------------------------------------------------------------------------
// KLASSE B (NE-16 / KON59-02, 2026-08-13): die TOP-LEVEL-JOBS der .gitlab-ci.yml.
//
// DER BEFUND: der S-14a-Job contract:axis-version-lock ruft KEIN scripts/*.sh --
// er baut das Tool comdare_axis_version_lock und faehrt --check/--write/git diff
// (.gitlab-ci.yml). Fuer den Klasse-A-Nenner (Skript-Aufrufe) war er STRUKTURELL
// unsichtbar; der Kombibau blieb zu Recht gruen, weil der Nenner zu klein war
// (15 Skript-Pfade sichtbar von 25 Top-Level-Jobs). Der richtige FREMDE Nenner
// (T-3/V-7) ist die Job-Ebene der YAML SELBST: jeder kuenftige Wachen-Job -- ob
// Skript-, Tool- oder ctest-Form -- traegt einen Top-Level-Schluessel und faellt
// damit in diese Menge, bevor irgendwer an ein Inventar gedacht hat.
// ---------------------------------------------------------------------------
enum class JobArt {
    WacheJob,      // faellt selbst ein Urteil ueber einen festen Gegenstand -- Deckung PFLICHT
    WachenTraeger, // ruft eine Klasse-A-Wache; die Deckung steht dort (kTabelle)
    WacheExtern,   // Urteil kommt aus dem fremden ci-templates-Projekt -- Deckung dort
    SuiteTraeger,  // faehrt Manifest-ctest-Auswahlen bzw. Suiten/Smokes
    Bau,           // baut nur, urteilt nicht ueber einen festen Pruefgegenstand
    Manuell,       // rules 'when: manual' -- faehrt nicht von selbst
    Template       // versteckter Top-Level-Schluessel (fuehrender '.'), kein Job
};

// Dieselbe Bauart wie Eintrag/kTabelle oben (LITERAL, bewusst nicht abgeleitet).
struct JobEintrag {
    std::string_view schluessel;                 // der Top-Level-Schluessel, wie er in der YAML steht
    JobArt           art     = JobArt::WacheJob; // Default = strengste Klasse (fail-closed wie Eintrag::art)
    Deckung          deckung = Deckung::Keine;   // nur fuer WacheJob PFLICHT (EXPECT unten)
    std::string_view beleg;                      // bei Gtest die .cpp
    std::string_view ziel;                       // bei Gtest der ctest-Zielname (T-7)
    std::string_view bemerkung;
};

constexpr JobEintrag kJobTabelle[] = {
    // -- versteckte Templates (fuehrender '.'): Top-Level-Schluessel, keine Jobs ---
    {".ccache-pull", JobArt::Template, Deckung::Keine, "", "", "ccache-Warmstart der bauenden Jobs."},
    {".bare_metal", JobArt::Template, Deckung::Keine, "", "", "Runner-Basis der baremetal-Lanes."},
    {".pmc", JobArt::Template, Deckung::Keine, "", "", "Basis der Vendor-Lanes (Performance-Counter)."},

    // -- Jobs, in YAML-Reihenfolge -------------------------------------------------
    {"lint:secrets", JobArt::WacheExtern, Deckung::Keine, "", "",
     "Urteil aus dem fremden ci-templates-Projekt (gitleaks); Deckung liegt dort."},
    {"lint:format", JobArt::WacheExtern, Deckung::Keine, "", "", "ci-templates: Format-Gate."},
    {"lint:static", JobArt::WacheExtern, Deckung::Keine, "", "", "ci-templates: Static-Analysis-Gate."},
    {"lint:layer-includes", JobArt::WachenTraeger, Deckung::Keine, "", "",
     "ruft scripts/lint_layer_includes.sh -- Wache und Deckung stehen in Klasse A."},
    {"lint:xml-wellformed", JobArt::WachenTraeger, Deckung::Keine, "", "",
     "ruft scripts/ci_xml_wellformed_guard.sh -- Klasse A."},
    {"build:clang", JobArt::Bau, Deckung::Keine, "", "", "baut die clang-Matrix, urteilt nicht."},
    {"pmc:amd", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Vendor-Lane, Manifest-Auswahl 'pmc'."},
    {"pmc:intel", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Vendor-Lane, Manifest-Auswahl 'pmc'."},
    {"build:arm64-smoke", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Cross-Smoke (optin: COMDARE_ISA_MATRIX)."},
    {"sanitize:asan-ubsan", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Sanitizer-Suite (ASan+UBSan)."},
    {"contract", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Contract-Suite (Manifest-Auswahl)."},
    {"test:coverage-guard", JobArt::WachenTraeger, Deckung::Keine, "", "",
     "ruft scripts/ci_test_coverage_guard.sh -- Klasse A."},
    {"contract:durability", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Durability-Suite."},
    {"contract:conformance", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Konformanz-Suite."},
    {"contract:pool_flip", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Pool-Flip-Suite."},
    {"contract:harness", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Harness-Suite."},
    {"contract:profile_coverage", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Profil-Abdeckungs-Suite."},
    {"contract:axis-version-lock", JobArt::WacheJob, Deckung::Gtest,
     "tests/unit/test_s14_axis_version_lock_tripwire.cpp", "test_s14_axis_version_lock_tripwire",
     "S-14a-Riegel: baut comdare_axis_version_lock und faehrt --check/--write/git diff -- KEIN "
     "Skript-Aufruf; genau der Job, den der Klasse-A-Nenner strukturell nicht sah (NE-16/KON59-02)."},
    {"contract:experiment_driver", JobArt::SuiteTraeger, Deckung::Keine, "", "",
     "Treiber-Gate (#193-A): baut und faehrt test_experiment_driver_v13."},
    {"contract:mess_report_smoke", JobArt::SuiteTraeger, Deckung::Keine, "", "",
     "Smoke der Mess-Report-CLI (existenz-inert, nicht inhalts-gegatet)."},
    {"contract:node_shape", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Node-Shape-Gate (#234)."},
    {"is_original:relock", JobArt::Manuell, Deckung::Keine, "", "",
     "rules 'when: manual' -- faehrt nur von Hand, deckt deshalb nichts."},
    {"chaos:drift", JobArt::SuiteTraeger, Deckung::Keine, "", "", "Drift-Suite (chaos)."},
    {"test:unit", JobArt::SuiteTraeger, Deckung::Keine, "", "",
     "Voll-Suite ohne die Hardware-Klasse 'pmc' (Manifest test_unit)."},
    // CI-DUAL 14.08. (Owner verbatim; KON22-01/8, KON55/KON55-01, T-11b): die drei
    // Zusatz-Zellen der {gcc,clang} x {Release,Debug}-Matrix. Gleiche Suite, gleicher
    // Manifest-Weg (make check) -- SuiteTraeger wie test:unit; sie rufen keine
    // Klasse-A-Wache und publizieren keine zweite Inventur (D1c bleibt bei test:unit).
    {"test:unit:debug", JobArt::SuiteTraeger, Deckung::Keine, "", "",
     "Zelle gcc x Debug der CI-DUAL-Matrix (Kette GCC hinter test:unit)."},
    {"test:unit:clang", JobArt::SuiteTraeger, Deckung::Keine, "", "",
     "Zelle clang x Release der CI-DUAL-Matrix (Kette CLANG hinter build:clang)."},
    {"test:unit:clang:debug", JobArt::SuiteTraeger, Deckung::Keine, "", "",
     "Zelle clang x Debug der CI-DUAL-Matrix (Kette CLANG hinter test:unit:clang)."},
    {"sanitize:tsan", JobArt::SuiteTraeger, Deckung::Keine, "", "", "TSan-Suite."},
};

// Reservierte GitLab-Schluessel: Pipeline-Struktur, keine Jobs. Kompilierte Menge,
// damit ein Tippfehler hier ein Compile- oder Testfehler ist und kein stilles Loch.
constexpr std::string_view kReserviert[] = {"include", "workflow", "stages",   "cache",         "variables",
                                            "default", "image",    "services", "before_script", "after_script"};

[[nodiscard]] bool ist_schluesselzeichen(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' ||
           c == ':' || c == '-';
}

struct TopLevelSchluessel {
    std::set<std::string> jobs;
    std::set<std::string> templates;
    std::set<std::string> reserviert;
    // Spalte-0-Zeilen, deren Form der Scanner NICHT kennt (A2.5-FIX F4, fail-closed):
    // sie zaehlen in keine der drei Mengen und machen den Inventar-Fall unten ROT --
    // benannt statt still verschluckt.
    std::vector<std::string>  unverstanden;
    [[nodiscard]] std::size_t gesamt() const { return jobs.size() + templates.size() + reserviert.size(); }
};

// Der Job-Scanner (gehaertet, A2.5-FIX F4, 2026-08-13): eine Zeile traegt einen
// Top-Level-Schluessel, wenn sie in Spalte 0 mit Nicht-Leerraum/Nicht-'#'/
// Nicht-'-' beginnt und ein YAML-Mapping-Schluessel folgt. ERKANNT werden alle
// drei am Objekt moeglichen Formen:
//   1. 'schluessel:'              Block-Form (heute alle 33 von 33 Schluessel)
//   2. 'schluessel: <wert>'       Wert-/Flow-Form, z.B. 'job: {script: x}'
//   3. '"schluessel":' bzw. '...' quotierter Schluessel, mit oder ohne Wert
// Der unquotierte Schluessel endet am ERSTEN ':', auf das Zeilenende oder
// Leerraum folgt -- innere ':' ohne Folge-Leerraum ('lint:secrets') bleiben Teil
// des Schluessels; wie bisher wird genau EIN abschliessender ':' gestrippt.
// Quotierte Schluessel duerfen beliebige Zeichen tragen (die Quote IST die
// YAML-Schreibform dafuer); unquotierte muessen Schluesselzeichen bleiben.
// HISTORIE (bis A2.5-F4): verlangt war '^<schluessel>:$' -- ein Top-Level-Job in
// Flow-Form oder mit quotiertem Schluessel fiel STILL aus der Klasse-B-Menge
// (Richtung-1-Abgleich sah ihn nie; die V4-ASSERTs fingen nur Totalausfall).
// FAIL-CLOSED-REST: jede Spalte-0-Zeile, die keiner der drei Formen entspricht,
// landet benannt in 'unverstanden' -- der Test macht daraus einen Fehler.
[[nodiscard]] TopLevelSchluessel job_schluessel(std::vector<std::string> const& zeilen) {
    TopLevelSchluessel s;
    for (auto const& z : zeilen) {
        if (z.empty()) { continue; }
        char const anfang = z.front();
        if (anfang == ' ' || anfang == '\t' || anfang == '#' || anfang == '-') { continue; }
        std::string kern;
        bool        verstanden = false;
        if (anfang == '"' || anfang == '\'') {
            std::size_t const ende = z.find(anfang, 1);
            if (ende != std::string::npos && ende + 1 < z.size() && z[ende + 1] == ':' &&
                (ende + 2 == z.size() || z[ende + 2] == ' ' || z[ende + 2] == '\t')) {
                kern       = z.substr(1, ende - 1);
                verstanden = !kern.empty();
            }
        } else {
            for (std::size_t i = 0; i < z.size(); ++i) {
                if (z[i] != ':') { continue; }
                if (i + 1 == z.size() || z[i + 1] == ' ' || z[i + 1] == '\t') {
                    kern       = z.substr(0, i);
                    verstanden = !kern.empty();
                    break;
                }
            }
            if (verstanden) {
                for (char const c : kern) {
                    if (!ist_schluesselzeichen(c)) {
                        verstanden = false;
                        break;
                    }
                }
            }
        }
        if (!verstanden) {
            s.unverstanden.push_back(z);
            continue;
        }
        bool ist_reserviert = false;
        for (std::string_view const r : kReserviert) {
            if (kern == r) {
                ist_reserviert = true;
                break;
            }
        }
        if (ist_reserviert) {
            s.reserviert.insert(kern);
        } else if (kern.front() == '.') {
            s.templates.insert(kern);
        } else {
            s.jobs.insert(kern);
        }
    }
    return s;
}

// T-7-Registrierungs-Marke fuer Gtest-Deckungen (Klasse A und Klasse B), NE-16:
// HAUSFORM comdare_add_test(<ziel> ODER rohes add_test(NAME <ziel> -- der S-14a-
// Riegel ist ein rohes add_test (tests/unit/CMakeLists.txt), und mit der alten
// Nur-Hausform-Marke war seine Deckung ein Rot-Beleg (Zwischenstand protokolliert).
// Beide Marken zaehlen nur im WIRKSAMEN Teil (Kommentar-Zeilen registrieren nichts).
[[nodiscard]] bool ziel_ist_registriert(std::vector<std::string> const& cmake_zeilen, std::string_view ziel) {
    std::string const marke_haus = "comdare_add_test(" + std::string{ziel};
    std::string const marke_roh  = "add_test(NAME " + std::string{ziel};
    for (auto const& z : cmake_zeilen) {
        std::string const wirksam = wirksamer_teil(z);
        if (wirksam.find(marke_haus) != std::string::npos) { return true; }
        if (wirksam.find(marke_roh) != std::string::npos) { return true; }
    }
    return false;
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
    // NE-16 (#39-Klasse): EXAKT statt Obergrenze -- sinkt die Ist-Zahl, wird die
    // Obergrenze im SELBEN Change mitgesenkt; sonst traegt der Schlupf zwischen
    // Ist und Schranke die naechste ungedeckte Wache unbemerkt.
    EXPECT_EQ(warteliste.size(), kWartelisteObergrenze)
        << "Die Warteliste weicht von der eingetragenen Zahl ab. GEWACHSEN: eine neue CI-gerufene "
           "Wache ohne Selbsttest ist genau die Klasse, gegen die dieser Posten gebaut ist. "
           "GESUNKEN: kWartelisteObergrenze im SELBEN Change mitsenken (Nachzug-Pflicht statt Schlupf).";

    // ==========================================================================
    // KLASSE B (NE-16 / KON59-02): die TOP-LEVEL-JOBS derselben YAML.
    // Der Klasse-A-Nenner oben sieht nur scripts/*.sh-Aufrufe. Der S-14a-Job
    // contract:axis-version-lock ruft keines (Tool-Bau + --check/--write/git
    // diff) und war strukturell unsichtbar -- der Nenner hier ist deshalb die
    // Job-Ebene der YAML SELBST: jede kuenftige Wache, egal in welcher Form,
    // traegt einen Top-Level-Schluessel und faellt in diese Menge.
    // ==========================================================================
    TopLevelSchluessel const schluessel = job_schluessel(ci_zeilen);

    // GEGENPROBE DES MESSGERAETS (V4, ASSERT, fail-closed): trifft der Scanner die
    // bekannten Schluessel nicht, ist die MESSUNG defekt, nicht die Tabelle -- und
    // dann darf dieser Fall kein Urteil faellen.
    ASSERT_GT(schluessel.jobs.size(), 0U) << "0 Top-Level-Jobs gefunden -- der Scanner greift nicht.";
    ASSERT_EQ(schluessel.jobs.count("contract:axis-version-lock"), 1U)
        << "Der NE-16-Gegenstand selbst fehlt in der Job-Menge -- Scanner defekt, kein Urteil.";
    ASSERT_EQ(schluessel.jobs.count("test:unit"), 1U)
        << "'test:unit' fehlt in der Job-Menge -- Scanner defekt, kein Urteil.";
    ASSERT_EQ(schluessel.jobs.count("stages"), 0U)
        << "'stages' ist ein reservierter Schluessel und darf nicht als Job zaehlen.";
    ASSERT_EQ(schluessel.jobs.count(".bare_metal"), 0U)
        << "'.bare_metal' ist ein Template und darf nicht als Job zaehlen.";
    // FAIL-CLOSED-REST (A2.5-FIX F4): eine Spalte-0-Zeile, deren Form der Scanner
    // nicht kennt, darf nicht still aus dem Nenner fallen -- sie steht hier mit
    // Wortlaut. Am Objekt ist die Menge leer (0 von 33 Schluesseln).
    ASSERT_TRUE(schluessel.unverstanden.empty()) << [&] {
        std::string s = "UNVERSTANDENE TOP-LEVEL-ZEILE(N) -- der Scanner kennt die Form nicht (fail-closed):\n";
        for (auto const& u : schluessel.unverstanden) { s += "  '" + u + "'\n"; }
        s += "Scanner in job_schluessel() erweitern oder die Zeile begruendet ausnehmen -- nicht liegen lassen.";
        return s;
    }();

    // -- Abgleich Richtung 1: jeder Job und jedes Template steht in der Tabelle -----
    std::vector<std::string> jobs_ohne_eintrag;
    auto                     in_tabelle = [](std::string const& name) {
        for (auto const& e : kJobTabelle) {
            if (e.schluessel == name) { return true; }
        }
        return false;
    };
    for (auto const& j : schluessel.jobs) {
        if (!in_tabelle(j)) { jobs_ohne_eintrag.push_back(j); }
    }
    for (auto const& t : schluessel.templates) {
        if (!in_tabelle(t)) { jobs_ohne_eintrag.push_back(t); }
    }

    // -- Abgleich Richtung 2: jede Tabellenzeile existiert noch als Schluessel ------
    std::vector<std::string> tote_job_zeilen;
    for (auto const& e : kJobTabelle) {
        std::string const name{e.schluessel};
        if (schluessel.jobs.count(name) == 0U && schluessel.templates.count(name) == 0U) {
            tote_job_zeilen.push_back(name);
        }
    }

    // -- Zaehlung je ART (EXAKTE Ausgabe mit Nenner, V-1) ---------------------------
    std::size_t wache_job = 0, wachen_traeger = 0, wache_extern = 0, suite_traeger = 0;
    std::size_t nur_bau = 0, manuell = 0, tmpl = 0, wache_job_ungedeckt = 0;
    for (auto const& e : kJobTabelle) {
        switch (e.art) {
            case JobArt::WacheJob:
                ++wache_job;
                if (e.deckung == Deckung::Keine) { ++wache_job_ungedeckt; }
                break;
            case JobArt::WachenTraeger: ++wachen_traeger; break;
            case JobArt::WacheExtern: ++wache_extern; break;
            case JobArt::SuiteTraeger: ++suite_traeger; break;
            case JobArt::Bau: ++nur_bau; break;
            case JobArt::Manuell: ++manuell; break;
            case JobArt::Template: ++tmpl; break;
        }
    }
    std::size_t const tabellen_zeilen = sizeof(kJobTabelle) / sizeof(kJobTabelle[0]);

    std::cout << "-----------------------------------------------------------------------------\n"
              << "T-6-INVENTAR, KLASSE B -- Top-Level-Jobs der .gitlab-ci.yml (NE-16/KON59-02)\n"
              << "  Nenner (fremd, T-3) : " << schluessel.jobs.size() << " Jobs von " << schluessel.gesamt()
              << " Top-Level-Schluesseln (" << schluessel.reserviert.size() << " reserviert, "
              << schluessel.templates.size() << " Templates)\n"
              << "  je ART, von " << tabellen_zeilen << " Tabellenzeilen: WacheJob " << wache_job << ", WachenTraeger "
              << wachen_traeger << ", WacheExtern " << wache_extern << ",\n"
              << "    SuiteTraeger " << suite_traeger << ", Bau " << nur_bau << ", Manuell " << manuell << ", Template "
              << tmpl << "\n"
              << "  WacheJob UNGEDECKT  : " << wache_job_ungedeckt << " von " << wache_job
              << " (Deckung ist fuer WacheJob PFLICHT)\n"
              << "  Scanner-Rest        : " << schluessel.unverstanden.size()
              << " unverstandene Spalte-0-Zeile(n) (fail-closed, ASSERT oben)\n"
              << "-----------------------------------------------------------------------------\n";

    EXPECT_TRUE(jobs_ohne_eintrag.empty()) << [&] {
        std::string s;
        for (auto const& p : jobs_ohne_eintrag) { s += "NEUER CI-JOB OHNE INVENTAR-EINTRAG: " + p + "\n"; }
        s += "Die CI kennt diesen Top-Level-Schluessel, kJobTabelle kennt ihn nicht. Genau so\n"
             "blieb der S-14a-Riegel unsichtbar: eine Wache in Tool- oder ctest-Form ruft kein\n"
             "Skript und fiel am Klasse-A-Nenner vorbei. Eintragen UND klassifizieren -- eine\n"
             "neue Wache (WacheJob) braucht ihre Deckung im SELBEN Change.";
        return s;
    }();
    EXPECT_TRUE(tote_job_zeilen.empty()) << [&] {
        std::string s;
        for (auto const& p : tote_job_zeilen) { s += "TOTE TABELLEN-ZEILE: " + p + "\n"; }
        s += "Eingetragen, aber kein Top-Level-Schluessel der .gitlab-ci.yml mehr (Job entfernt\n"
             "oder umbenannt) -- die Zeile im SELBEN Change pflegen, nicht liegen lassen.";
        return s;
    }();
    // Deckungs-PFLICHT der Klasse WacheJob (exakt null ungedeckt): eine Schranke
    // waere derselbe Schlupf, den #39 beim Floor geschlossen hat.
    EXPECT_EQ(wache_job_ungedeckt, 0U)
        << "Ein WacheJob ohne Deckung: sein Urteil prueft niemand. Deckung (Gtest) im SELBEN Change "
           "nachreichen oder die Zeile BEWUSST und begruendet umklassifizieren.";
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
            EXPECT_TRUE(ziel_ist_registriert(cmake_zeilen, e.ziel))
                << "Ziel '" << e.ziel << "' ist in tests/unit/CMakeLists.txt nicht registriert -- "
                << "eine Deckung, die in keinem 'ctest -N' erscheint, ist keine.";
        } else {
            // Ein Selbsttest, den niemand ruft, deckt nichts.
            EXPECT_TRUE(gerufen.count(std::string{e.beleg}) == 1U)
                << "Der Selbsttest '" << e.beleg << "' wird von der .gitlab-ci.yml nicht gerufen.";
        }
    }
    // -- KLASSE B (NE-16): dieselbe Pruefung fuer die Gtest-gedeckten CI-JOBS -------
    // Der S-14a-Riegel ist ein ROHES add_test (kein comdare_add_test) -- die Marke
    // muss beide Registrierungs-Formen sehen, sonst ist die Deckung eine Behauptung.
    for (auto const& e : kJobTabelle) {
        if (e.deckung == Deckung::Keine) { continue; }
        ++geprueft;
        fs::path const beleg = wurzel() / std::string{e.beleg};
        EXPECT_TRUE(fs::exists(beleg)) << "Deckungs-Beleg fuer CI-Job " << e.schluessel << " fehlt: " << beleg.string();
        if (e.deckung == Deckung::Gtest) {
            EXPECT_TRUE(ziel_ist_registriert(cmake_zeilen, e.ziel))
                << "Ziel '" << e.ziel << "' (CI-Job " << e.schluessel << ") ist in tests/unit/CMakeLists.txt "
                << "nicht registriert (weder comdare_add_test noch add_test(NAME ...)) -- T-7: eine Deckung, "
                << "die in keinem 'ctest -N' erscheint, ist keine.";
        }
    }

    ASSERT_GT(geprueft, 0U) << "0 Deckungs-Belege geprueft -- dann sagt dieser Fall nichts aus (fail-closed).";
    std::cout << "T-6-INVENTAR: " << geprueft << " Deckungs-Beleg(e) am Gegenstand nachgesehen"
              << " (Klasse A und Klasse B).\n";
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

// =============================================================================
// (4) DAS MESSGERAET SELBST (A2.5-FIX F4, 2026-08-13): der Schluessel-Scanner
//     erkennt alle drei am Objekt moeglichen Formen -- Block, Wert/Flow,
//     quotiert -- und wirft nichts still weg. Bis zu diesem Fix verlangte er
//     '^<schluessel>:$': ein Top-Level-Job in Flow-Form ('job: {script: x}')
//     oder mit quotiertem Schluessel ('"job":') fiel STILL aus der Klasse-B-
//     Menge -- Richtung 1 sah ihn nie, die V4-Gegenproben fingen nur den
//     Totalausfall. Am Objekt war die Kante leer (0 von 33 Schluesseln in
//     diesen Formen, alle 33 Block-Form); der Koeder hier ist deshalb
//     synthetisch, und das Mutations-Protokoll des Commits fuehrt ihn
//     zusaetzlich am lebenden Objekt vor (Flow-Koeder vor der Haertung: still
//     gruen -- danach: ROT mit Namen).
// =============================================================================
TEST(T6WachenInventar, ScannerErkenntBlockFlowUndQuotierteSchluessel) {
    std::vector<std::string> const probe = {
        "# kommentar: kein schluessel",                 // Kommentar -> nichts
        "stages: [lint, build]",                        // reserviert, Wert-Form
        "variables:",                                   // reserviert, Block-Form
        ".tpl_probe: {extends: .bare_metal}",           // Template, Flow-Form
        "zz_block:",                                    // Job, Block-Form (Regressions-Anker)
        "zz_flow: {stage: lint, script: [echo probe]}", // Job, Flow-Form
        "\"zz_quoted\":",                               // Job, doppelt quotiert
        "'zz_quoted2': {stage: lint}",                  // Job, einfach quotiert + Wert
        "lint:secrets:",                                // Job mit innerem ':' -- EIN ':' gestrippt
        "zz_wert: 7",                                   // Schluessel mit Skalar-Wert
        "  eingerueckt:",                               // kein Top-Level
        "- listenelement:",                             // Sequenz-Element
        "kaputt ohne doppelpunkt",                      // keine Schluessel-Form -> unverstanden
    };
    TopLevelSchluessel const s = job_schluessel(probe);

    EXPECT_EQ(s.jobs.count("zz_block"), 1U) << "Block-Form ist der Bestandsfall und muss stehen bleiben.";
    EXPECT_EQ(s.jobs.count("zz_flow"), 1U) << "Flow-Form 'job: {...}' faellt still aus der Klasse-B-Menge (F4).";
    EXPECT_EQ(s.jobs.count("zz_quoted"), 1U) << "Quotierter Schluessel '\"job\":' faellt still aus der Menge (F4).";
    EXPECT_EQ(s.jobs.count("zz_quoted2"), 1U) << "Einfach quotierter Schluessel mit Wert faellt still aus (F4).";
    EXPECT_EQ(s.jobs.count("lint:secrets"), 1U) << "Genau EIN abschliessender ':' wird gestrippt (Bestand).";
    EXPECT_EQ(s.jobs.count("zz_wert"), 1U) << "'schluessel: wert' ist ein Top-Level-Schluessel (F4).";
    EXPECT_EQ(s.reserviert.count("stages"), 1U) << "Reservierter Schluessel in Wert-Form bleibt reserviert.";
    EXPECT_EQ(s.reserviert.count("variables"), 1U) << "Reservierter Schluessel in Block-Form (Bestand).";
    EXPECT_EQ(s.templates.count(".tpl_probe"), 1U) << "'.'-Praefix in Flow-Form bleibt Template.";
    EXPECT_EQ(s.jobs.count("eingerueckt") + s.jobs.count("listenelement"), 0U)
        << "Eingerueckte und '-'-Zeilen sind keine Top-Level-Schluessel.";
    EXPECT_EQ(s.gesamt(), 9U) << "6 Jobs + 2 reserviert + 1 Template -- jede Abweichung ist ein Scanner-Defekt.";

    // FAIL-CLOSED-REST: die eine Zeile ohne Schluessel-Form landet BENANNT in
    // 'unverstanden' -- nicht in einer der drei Mengen und nicht im Nichts.
    ASSERT_EQ(s.unverstanden.size(), 1U)
        << "Genau 1 von 13 Probe-Zeilen hat keine Schluessel-Form -- der Rest ist erkannt oder kein Top-Level.";
    EXPECT_EQ(s.unverstanden.front(), "kaputt ohne doppelpunkt")
        << "Die unverstandene Zeile muss mit WORTLAUT dastehen, sonst ist der Befund nicht adressierbar.";
}
