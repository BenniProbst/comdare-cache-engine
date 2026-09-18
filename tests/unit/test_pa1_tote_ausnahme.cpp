// test_pa1_tote_ausnahme -- ERLOSCHEN hat ZWEI Richtungen.                 (2026-08-10)
// =============================================================================
// PRUEFLING: scripts/ci_test_registrierungs_wache.sh
//
// DER BEFUND, GEGEN DEN DIESE DATEI GEBAUT IST
//
// Die Wache definierte ERLOSCHEN in EINER Richtung: "existiert der Gegenstand WIEDER,
// traegt die Ausnahme nicht mehr". Das faengt den Fall, dass der Gegenstand zurueck-
// kehrt. Es faengt NICHT den Fall, dass er NIE zurueckkehren kann -- und genau der lag
// vor: alle vier prt-art-Zeilen der Allowlist banden an
//     datei:prt_art/include/prt_art/prt_art.hpp
// einen Pfad, den nichts mehr erzeugen kann. Am Objekt gemessen (2026-08-10):
//   prt_art.hpp .................. 0 Treffer unter /home/comdare
//   Gegenprobe cache_engine.hpp .. 45 Treffer -- das Werkzeug sucht, der Nullbefund gilt
//   Historie ..................... in ce vom 2026-05-12 (5c210581) bis 2026-05-14
//                                  getrackt, dann per 'git mv' nach
//                                  libs/deprecated/prt_art_legacy/ (b9abc720, R100),
//                                  dieser Baum am 2026-06-01 entfernt (6b3ed0d9).
//                                  Im Nachbarrepo comdare-prt-art NIE getrackt.
// FOLGE: die vier Zeilen konnten per Konstruktion nie erloeschen. Sie waren die
// Freistellung mit unbegrenzter Laufzeit, vor der der Kopf ihrer eigenen Datei warnt.
//
// DER ERSTLAUF, WOERTLICH (Rot zuerst, T-1). Gegen einen Bau-Baum, dem GENAU diese vier
// Dateien fehlten, meldete die Wache VOR der Heilung:
//     davon 4 begruendet, 0 mit ERLOSCHENER, 0 mit UNPRUEFBARER Begruendung, 0 ohne.
//     TEST-REGISTRIERUNGS-WACHE: OK (470 Quelldateien, 0 ohne Begruendung ausserhalb).
// Vier Ausnahmen ohne Ablauf, und die Wache sagte "OK".
//
// WAS DIE WACHE JETZT MISST -- und was das NICHT beweist:
// "Kann nie entstehen" ist nicht entscheidbar, solange man nur den eigenen Baum kennt.
// Gemessen wird deshalb etwas Engeres: KEINE QUELLE DIESES REPOS ERKLAERT DEN
// ZWEIG, IN DEM ER LIEGT -- weder der Git-Index (getrackte Datei ODER Submodul-Gitlink)
// noch eine .gitignore-Regel, und das bis hinauf zur Wurzel. Dann hat er in diesem
// Repository keinen Erzeuger.
// DAS BEWEIST NICHT, dass er nie existieren kann: jemand kann ihn morgen 'git add'en.
// Diese Datei prueft deshalb GENAU DIESE ENGERE AUSSAGE und keine groessere -- Fall (2),
// (3) und (4) belegen ausdruecklich, dass ein Pfad mit Erzeuger NICHT als tot gilt.
//
// WIE DIE VIER GRUEN-FAELLE MESSEN, und warum nicht anders (am Objekt gelernt): sie
// fragen NICHT, ob die Zeichenkette "TOTE AUSNAHME" in der Ausgabe fehlt. Der NENNER
// der Wache nennt die Klasse bei JEDEM Lauf ("... 0 TOTE AUSNAHME ..."), ein solches
// EXPECT_FALSE waere also immer verletzt -- beim ersten Lauf dieser Datei ist genau das
// passiert. Geprueft wird deshalb der ZAEHLER: "0 TOTE AUSNAHME" fuer gruen,
// "1 TOTE AUSNAHME" fuer den Biss. Dieselbe Klasse wie '/build' gegen '/builds': eine
// Teilzeichenkette ist kein Feld.
//
// WARUM ROT UND KEINE EIGENE WARNKLASSE (begruendete Entscheidung, PA-1): eine Warnung
// reproduziert exakt die Fehlerklasse -- an genau so einer Stelle sind vier Dateien 14
// bzw. 68 Tage unbemerkt geblieben. Die Hausregel kennt ZELLE=Warnung, JOB=hart rot.
// Der Preis ist ein moeglicher Fehlalarm; er ist billig zu beheben (erreichbaren
// Gegenstand nennen oder 'frist:' setzen) und faellt sofort auf, waehrend das Gegenteil
// -- eine tote Zeile -- per Konstruktion nie auffaellt.
//
// WARUM EIN GOOGLE TEST UND KEINE WEITERE SHELL-PROBE (Owner-KERN 09.08.2026):
//   "Ich sehe einen Haufen shells statt vernuenftiger google tests, was soll das? ...
//    SKRIPTE SAGEN GAR NICHTS."
// Debug UND Release, ueber die normale ctest-Registrierung.
//
// K13 BEIDSEITIG: jeder Fall wuerfelt frisch aus /dev/urandom. Fall (1) verlangt, dass
// ein unmoeglicher Pfad BEISST; Fall (2) und (3) verlangen, dass ein moeglicher Pfad
// NICHT beisst. Ein Orakel, das immer ROT liefert, faellt an (2)/(3); eines, das immer
// GRUEN liefert, faellt an (1).
//
// GRENZE, EHRLICH BENANNT (T-9): Fall (8) faehrt die ECHTE Allowlist, laesst dabei aber
// die 'isa:'-Zeile ausdruecklich AUS dem Pruefsatz -- ihre Auswertung braucht den
// CMakeCache des gemessenen Baums, und ein kuenstlich erzeugtes Fehlen wuerde sie auf
// einem AVX-512-Host als ERLOSCHEN melden, obwohl sie im echten Baum nie gefragt wird.
// Geprueft sind dort also die vier 'frist:'-Zeilen; NICHT geprueft ist die 'isa:'-Zeile.
// Beide Mengen stehen in der Ausgabe des Falls.
//
// NACHTRAG (2026-09-17, Owner-Order 206 "OV-2 Archiv-Variante gilt und frist Zeilen entfernen"):
// der Satz "geprueft sind dort die vier 'frist:'-Zeilen" ist UEBERHOLT -- die vier Zeilen sind aus
// der Allowlist entfernt. An ihre Stelle tritt die ARCHIV-Klasse der Wache: eine getrackte
// Test-Quelldatei unter tests/deprecated/<ordner>/ zaehlt nicht zum SOLL, wenn und nur wenn
// tests/deprecated/<ordner>/VERMERK.md im Index liegt UND die Form eines Ankers hat (regulaeres Blob
// 100644/100755 auf Index-Stufe 0 mit Nicht-Leerraum-Inhalt; NACHTRAG 3). Fall (8) misst ab jetzt GENAU DAS am echten
// Repo (die vier fehlen im Bauweg und muessen trotzdem gruen sein, weil sie ARCHIV sind), Fall (10)
// die Regel selbst an einem Wegwerf-Repo -- beidseitig, samt Nachbar-Anker-Probe.
// UNVERAENDERT: die 'frist:'-ART der Wache bleibt in Gebrauch und wird von Fall (7) beidseitig
// gefahren; sie hat nach diesem Entscheid nur keinen Gegenstand mehr in der committeten Allowlist.
//
// NACHTRAG 2 (2026-09-18, Lens-Funde r1 des OV-2-Zuges: Lens A LA-04, Lens B LB-01/LB-02/LB-03/LB-05):
// Fall (10) traegt jetzt die Stufen (d)/(e) -- eine Allowlist-Zeile fuer eine ARCHIV-Datei ist eine
// Ausnahme OHNE ANLASS und ROT; die Faelle (11a-c) geben den drei im Wachen-Kopf behaupteten Grenzen
// der Archiv-Regel je einen Traeger; Fall (12) verlangt einen Anker mit Inhalt und Form (0 Byte oder
// Symlink ankern nicht); Fall (13) faengt die Schwesterklasse "Allowlist-Zeile ohne Gegenstand im
// SOLL-Bestand". Alle Archiv-Nenner sind ab hier FELD-verankert gepinnt (", 4 archiviert)" mit Komma davor
// und Klammer danach statt "4 archiviert" -- Teilzeichenkette von "14 archiviert", Klasse s. oben).
//
// NACHTRAG 3 (2026-09-18, Fix-r2 des OV-2-Zuges: Lens A r3, Lens B r2, Lens C r2): Fall (8) pinnt die
// Nenner-Zeilen der Wache mit den GEMESSENEN Zahlen (LCT-09); jeder Zaehler-Pin der Faelle (10d/e), (12),
// (13) und der neuen Faelle ist eine GANZE Nenner-Zeile (Helfer nenner_ohne_bauweg, nenner_davon,
// endzeile_rot, endzeile_ok; LCT-10/LCT-11, Klasse LB-03). Fall (12) verlangt zusaetzlich ein
// Nicht-Leerraum-Zeichen im Anker (F1) und traegt Modus 100755 als gruene Stufe (LCT-12); Fall (12b) haelt
// fest, dass ein Anker im MERGE-KONFLIKT (Index-Stufe 1-3) nicht ankert (LCW-01). Fall (13) heisst jetzt
// ...ImSollBestand... (das Etikett "ohne Gegenstand im Index" war falsch: eine Datei unter ext/ steht im
// Index, aber nicht im SOLL-Bestand; F2) und ueberspringt Leerraum-Zeilen (F3). Neue Faelle: (14) letzte
// Zeile ohne Zeilenumbruch (LCW-04), (15) Allowlist-Zeile fuer einen Pfad unter tests/deprecated/ (LCW-05),
// (16) doppelte Zeile je Pfad (LCW-08), (17) leeres Feld 3 (LCW-09), (18) Werkzeug-Ausfall in der
// Nenner-Pipeline = Exit 2 per PATH-Koeder (LCW-02/LCW-03). Rot zuerst je Stufe gegen die Wache d8e8f53d
// bzw. einen Mutanten -- Belege im Beweisort des OV-2-Zuges (FIX-r2.md).
//
// NACHTRAG 4 (2026-09-18, Fix-r3 des OV-2-Zuges: Lens C r3 LC3W-01..08): neue Faelle (19) leeres ISA-
// Teilmerkmal (LC3W-07), (20) ISA-Belege eindeutig (LC3W-08), (21) Werkzeug-Ausfall im ISA-Pfad per
// wc-Koeder an der ISA-Zwischendatei (LC3W-01; sed ist aus der Wache entfernt, der Koeder trifft das
// verbliebene PATH-Werkzeug), (22) grep-Status 2 = Exit 2 (LC3W-02; IST-Datei unlesbar per chmod 000
// oder procfs), (23) git-Fehler 128 in der Erreichbarkeits-Probe (LC3W-03), (24) der Anker muss ein
// Blob in der Objektdatenbank sein (LC3W-06; cat-file-Fehler = Exit 2, LC3W-03), (25) date-Ausfall
// (LC3W-04), (26) Formfehler der stummen Zeile einer Datei im Bauweg (LC3W-05 -- die Test-Auflage der
// Triage zeigte Exit 0 gegen 806629ca, der Fund war echt). Die Nenner-Zeile "dazu UNPRUEFBAR ..." traegt
// ein sechstes Feld ("N mit Formfehler fuer Dateien im Bauweg"); nenner_ohne_bauweg() hat dafuer einen
// sechsten Parameter mit Vorgabe 0, alle aelteren Pins bleiben ganze Zeilen. Rot zuerst je Stufe gegen
// die Wache 806629ca (das neue Binary gegen die alte Wache per COMDARE_PA1_WACHE_PFAD) -- FIX-r3.md.
//
// ASCII-only, Zeilen <= 120 Byte.
// =============================================================================

#if defined(_WIN32)

#include <gtest/gtest.h>

TEST(Pa1ToteAusnahme, NurPosix) {
    GTEST_SKIP() << "scripts/ci_test_registrierungs_wache.sh ist ein POSIX-sh-Skript "
                    "(kein Windows-Job faehrt es).";
}

#else

#include "support/wachen_werkbank.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using comdare::test::wachen::enthaelt;
using comdare::test::wachen::fahre;
using comdare::test::wachen::koeder;
using comdare::test::wachen::Lauf;
using comdare::test::wachen::WegwerfRepo;
using comdare::test::wachen::zitiert;

namespace {

// Die Wache verlangt diese Datei als MESSGERAET-GEGENPROBE: sie muss existieren UND im
// Bauweg stehen, sonst faellt die Wache kein Urteil (Exit 2).
constexpr char const* kGegenprobe = "tests/unit/test_pressure_state.cpp";

// Die vier geparkten Posten. Sie stehen hier als Literale, weil Fall (8) und Fall (9)
// genau sie gegen die beiden committeten Allowlists fahren.
// W-B (2026-08-15, Vorlage-B2-GO): per 'git mv' nach tests/deprecated/prt_art_legacy_waisen/
// archiviert (VERMERK.md dort). Der SOLL ist am DATEINAMEN verankert, die vier bleiben also
// im SOLL und in der Allowlist -- nur der Pfad in Feld 1 und hier ist nachgezogen.
// UEBERHOLT (2026-09-17, Owner-Order 206): der letzte Satz gilt nicht mehr. Die vier stehen in
// KEINER Allowlist mehr und sind auch nicht mehr im SOLL der Registrierungs-Wache -- sie sind
// ARCHIV, weil tests/deprecated/prt_art_legacy_waisen/VERMERK.md im Index liegt und die Wache
// diesen Anker liest. Die Literale hier bleiben trotzdem noetig: Fall (8) nimmt sie aus dem
// Bauweg heraus (Anlass des Archiv-Abzugs) und Fall (9) sucht ihre Namen in der Schwesterliste.
constexpr char const* kGeparkt[] = {
    "tests/deprecated/prt_art_legacy_waisen/test_concepts_compile.cpp",
    "tests/deprecated/prt_art_legacy_waisen/test_value_handle.cpp",
    "tests/deprecated/prt_art_legacy_waisen/test_three_layer_audit.cpp",
    "tests/deprecated/prt_art_legacy_waisen/test_six_page_structures.cpp",
};
constexpr std::size_t kGeparktN = sizeof kGeparkt / sizeof kGeparkt[0];
// PFLEGE-KOPPLUNG (Lens B LB-03-Rest, Lens C LCT-09; 2026-09-18): Fall (8) pinnt die Archiv-Zahlen der Wache
// aus DIESEN Literalen -- die Datei-Zahl aus kGeparktN, die Ordner-Zahl aus kGeparktOrdnerN (alle vier liegen
// in EINEM Ordner). Ein fuenfter Archiv-Ordner oder eine fuenfte archivierte Datei im echten Repo aendert die
// Zahlen der Wache; dann sind kGeparkt UND kGeparktOrdnerN nachzuziehen, sonst faellt Fall (8) laut (nie
// still gruen: der Nenner der Wache und der SOLL des Falls stammen aus derselben Quelle, git ls-files).
constexpr std::size_t kGeparktOrdnerN = 1;

// Ein EHRLICHER CMakeCache fuer die ISA-Faelle (19)-(21) (2026-09-18, Fix-r3): avx2 vorhanden (Wert 1,
// _EXITCODE 0), avx512f fehlt (Wert leer, _EXITCODE 1) -- dieselbe Form wie kEhrlichAvx2 in
// tests/unit/test_mt_l4_registrierungs_wache_isa.cpp. Damit traegt 'isa:avx512f' (das Merkmal fehlt dem
// Bau-Host) -- die Faelle hier brauchen die tragende Seite als Arrangement und Gegenrichtung.
constexpr char const* kIsaCacheAvx2DaAvx512fFehlt = "COMDARE_HOST_RUNS_AVX2:INTERNAL=1\n"
                                                    "COMDARE_HOST_RUNS_AVX2_COMPILED:INTERNAL=TRUE\n"
                                                    "COMDARE_HOST_RUNS_AVX2_EXITCODE:INTERNAL=0\n"
                                                    "COMDARE_HOST_RUNS_AVX512F:INTERNAL=\n"
                                                    "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
                                                    "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=1\n";

[[nodiscard]] std::string wachen_pfad() {
    if (char const* const ueberschrieben = std::getenv("COMDARE_PA1_WACHE_PFAD");
        ueberschrieben != nullptr && *ueberschrieben != '\0') {
        return std::string{ueberschrieben};
    }
    return std::string{COMDARE_PA1_WACHE};
}

[[nodiscard]] std::string repo_wurzel() { return std::string{COMDARE_PA1_REPO}; }

void berichten(char const* fall, Lauf const& lauf, std::string const& marke) {
    std::cout << "  [PA-1] Fall '" << fall << "' | Koeder " << marke << " | Prueflig " << wachen_pfad() << " | Exit "
              << lauf.code << "\n";
}

// ---------------------------------------------------------------------------
// DIE NENNER-ZEILEN DER WACHE ALS GANZE ZEILEN (Lens C LCT-10/LCT-11, 2026-09-18). Ein Zaehler-Pin wie
// "1 unpruefbar" ist eine Teilzeichenkette von "11 unpruefbar", "1 ohne Gegenstand" eine von "11 ohne
// Gegenstand" -- dieselbe Klasse wie '/build' gegen '/builds' (Kopf). Gepinnt wird deshalb die ganze Zeile
// mit allen Feldern: wer ein Feld umbenennt, verschiebt oder seinen Zaehler in einen fremden Kontext
// traegt, faellt hier laut. Die Reihenfolge der Felder ist die der Wache (scripts/ci_test_registrierungs_
// wache.sh, Block NENNER und Endzeile); eine Aenderung dort zieht diese vier Helfer nach.
// SECHSTES FELD (2026-09-18, Fix-r3, Lens C LC3W-05): "N mit Formfehler fuer Dateien im Bauweg" -- der
// Parameter 'form' hat die Vorgabe 0, damit jeder aeltere Pin weiter die GANZE Zeile prueft.
// ---------------------------------------------------------------------------
[[nodiscard]] std::string z(std::size_t n) { return std::to_string(n); }

[[nodiscard]] std::string nenner_ohne_bauweg(std::size_t anker, std::size_t archiv_zeilen, std::size_t ort_zeilen,
                                             std::size_t geist, std::size_t doppelt, std::size_t form = 0) {
    return "dazu UNPRUEFBAR ohne Bezug zum Bauweg: " + z(anker) + " ARCHIV-Anker ohne Inhalt/Form, " +
           z(archiv_zeilen) + " Allowlist-Zeile(n) fuer ARCHIV-Dateien, " + z(ort_zeilen) +
           " fuer Pfade unter tests/deprecated/ ohne wirksamen Anker, " + z(geist) +
           " ohne Gegenstand im SOLL-Bestand, " + z(doppelt) + " Pfad(e) mit doppelter Zeile, " + z(form) +
           " mit Formfehler fuer Dateien im Bauweg.";
}

[[nodiscard]] std::string nenner_davon(std::size_t begruendet, std::size_t erloschen, std::size_t tot,
                                       std::size_t unpruefbar, std::size_t ohne) {
    return "davon " + z(begruendet) + " begruendet, " + z(erloschen) + " mit ERLOSCHENER, " + z(tot) +
           " TOTE AUSNAHME, " + z(unpruefbar) + " mit UNPRUEFBARER Begruendung, " + z(ohne) + " ohne.";
}

[[nodiscard]] std::string endzeile_rot(std::size_t ohne, std::size_t soll, std::size_t erloschen, std::size_t tot,
                                       std::size_t unpruefbar, std::size_t archiviert) {
    return "TEST-REGISTRIERUNGS-WACHE: ROT (" + z(ohne) + " von " + z(soll) + " ohne Begruendung, " + z(erloschen) +
           " erloschen, " + z(tot) + " tot, " + z(unpruefbar) + " unpruefbar, " + z(archiviert) + " archiviert).";
}

[[nodiscard]] std::string endzeile_ok(std::size_t soll, std::size_t archiviert) {
    return "TEST-REGISTRIERUNGS-WACHE: OK (" + z(soll) + " Quelldateien, 0 ohne Begruendung ausserhalb, " +
           z(archiviert) + " archiviert).";
}

// Ein Wegwerf-Repo-Kommando in der Umgebung der Werkbank (git im Repo, Ausgabe getrimmt).
[[nodiscard]] Lauf im_repo(WegwerfRepo const& repo, std::string const& befehl) {
    Lauf l = fahre("cd " + zitiert(repo.pfad()) + " && " + WegwerfRepo::umgebung() + " " + befehl);
    while (!l.ausgabe.empty() && (l.ausgabe.back() == '\n' || l.ausgabe.back() == '\r')) { l.ausgabe.pop_back(); }
    return l;
}

// Ein PATH-Koeder: ein Werkzeug gleichen Namens VOR dem echten im PATH (Fall (18)).
[[nodiscard]] testing::AssertionResult koeder_bin_anlegen(WegwerfRepo const& repo, std::string const& name,
                                                          std::string const& inhalt) {
    testing::AssertionResult const r = repo.schreibe("koeder_bin/" + name, inhalt);
    if (!r) { return r; }
    std::error_code ec;
    fs::permissions(repo.pfad() / "koeder_bin" / name,
                    fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read |
                        fs::perms::others_exec,
                    fs::perm_options::replace, ec);
    if (ec) { return testing::AssertionFailure() << "chmod +x '" << name << "' fehlgeschlagen: " << ec.message(); }
    return testing::AssertionSuccess();
}

// ---------------------------------------------------------------------------
// Ein Fall: ein Wegwerf-Repo mit Gegenprobe-Datei, einer Waise und einem Bau-Baum, in
// dem die Waise fehlt. Was die Allowlist-Zeile dann in Feld 2 nennt, ist der Gegenstand
// der Untersuchung.
// ---------------------------------------------------------------------------
class Fall {
public:
    explicit Fall(std::string const& marke) : Fall{marke, "tests/unit/test_waise_" + marke + ".cpp"} {}

    // Zweiter Konstruktor (2026-09-17): der Waisen-PFAD ist waehlbar. Die ARCHIV-Klasse der Wache
    // haengt am ORT der Datei, nicht an ihrem Namen -- ein Fall dafuer braucht eine Waise unter
    // tests/deprecated/<ordner>/ statt unter tests/unit/. Alles andere bleibt identisch.
    Fall(std::string const& marke, std::string const& waisen_pfad)
        : marke_{marke}, repo_{marke}, baum_{repo_.pfad().string() + "_baum"}, waise_{waisen_pfad} {
        std::error_code ec;
        fs::create_directories(baum_, ec);
    }
    Fall(Fall const&)            = delete;
    Fall& operator=(Fall const&) = delete;
    ~Fall() {
        std::error_code ec;
        fs::remove_all(baum_, ec);
    }

    [[nodiscard]] testing::AssertionResult init() {
        testing::AssertionResult r = repo_.init();
        if (!r) { return r; }
        r = repo_.ist_eigene_wurzel();
        if (!r) { return r; }
        r = repo_.schreibe_und_verfolge(kGegenprobe, "// Gegenprobe-Datei des Falls\n");
        if (!r) { return r; }
        // Die Waise ist getrackt und fehlt gleich im Bauweg -- sie ist der Anlass,
        // aus dem die Wache ueberhaupt in die Allowlist schaut.
        r = repo_.schreibe_und_verfolge(waise_, "// Waise des Falls " + marke_ + "\n");
        if (!r) { return r; }
        // Nur die Gegenprobe steht im Bauweg, die Waise nicht.
        return bauweg_schreiben({kGegenprobe});
    }

    // Feld 2 der einen Allowlist-Zeile setzen. Feld 3 traegt die Koeder-Marke, damit
    // ein GRUEN-Fall belegen kann, dass die Wache GENAU DIESE Zeile gelesen hat.
    [[nodiscard]] testing::AssertionResult allowlist_setzen(std::string const& feld2) const {
        return repo_.schreibe("scripts/ci_test_registrierungs_allowlist.txt", "# Allowlist des Falls " + marke_ + "\n" +
                                                                                  waise_ + " | " + feld2 +
                                                                                  " | Koeder " + marke_ + "\n");
    }

    [[nodiscard]] testing::AssertionResult bauweg_schreiben(std::vector<std::string> const& relativ) const {
        std::ofstream aus{baum_ / "compile_commands.json", std::ios::trunc};
        if (!aus.good()) { return testing::AssertionFailure() << "compile_commands.json nicht schreibbar"; }
        aus << "[\n";
        for (std::size_t i = 0; i < relativ.size(); ++i) {
            aus << "  {\"directory\": \"" << baum_.string() << "\",\n"
                << "   \"command\": \"c++ -c " << (repo_.pfad() / relativ[i]).string() << "\",\n"
                << "   \"file\": \"" << (repo_.pfad() / relativ[i]).string() << "\"}"
                << (i + 1 == relativ.size() ? "\n" : ",\n");
        }
        aus << "]\n";
        aus.close();
        if (!fs::exists(baum_ / "compile_commands.json")) {
            return testing::AssertionFailure() << "compile_commands.json fehlt nach dem Schreiben";
        }
        return testing::AssertionSuccess();
    }

    // 'heute' leer = Systemuhr. Sonst wird der Wache ihr Heute vorgegeben, damit beide
    // Seiten einer Frist gefahren werden koennen, ohne die Systemuhr zu stellen.
    // 'zusatz' (2026-09-18, Fall (18)): weitere Umgebung fuer den Wachen-Aufruf, z. B. ein PATH mit
    // einem Koeder-Werkzeug davor; die Shell des Aufrufs expandiert '$PATH' darin.
    [[nodiscard]] Lauf fahren(std::string const& heute = "", std::string const& zusatz = "") const {
        std::string vorspann = WegwerfRepo::umgebung();
        if (!heute.empty()) { vorspann += " COMDARE_WACHE_HEUTE=" + heute; }
        if (!zusatz.empty()) { vorspann += " " + zusatz; }
        return fahre("cd " + zitiert(repo_.pfad()) + " && " + vorspann + " sh " + zitiert(wachen_pfad()) + " " +
                     zitiert(baum_));
    }

    // Ein CMakeCache.txt im Bau-Baum (2026-09-18, Fix-r3, Faelle (19)-(21)): 'isa:'-Zeilen lesen ihn;
    // ohne ihn ist jede 'isa:'-Zeile Exit 2. Der Inhalt kommt vom Fall, damit jede Stufe ihren Cache sieht.
    [[nodiscard]] testing::AssertionResult isa_cache_schreiben(std::string const& inhalt) const {
        std::ofstream aus{baum_ / "CMakeCache.txt", std::ios::trunc};
        if (!aus.good()) { return testing::AssertionFailure() << "CMakeCache.txt nicht schreibbar"; }
        aus << inhalt;
        aus.close();
        if (!fs::exists(baum_ / "CMakeCache.txt")) {
            return testing::AssertionFailure() << "CMakeCache.txt fehlt nach dem Schreiben";
        }
        return testing::AssertionSuccess();
    }

    [[nodiscard]] WegwerfRepo const& repo() const { return repo_; }
    [[nodiscard]] std::string const& waise() const { return waise_; }
    [[nodiscard]] fs::path const&    baum() const { return baum_; }

private:
    std::string marke_;
    WegwerfRepo repo_;
    fs::path    baum_;
    std::string waise_;
};

} // namespace

// =============================================================================
// (1) DER UNMOEGLICHE GEGENSTAND BEISST. Der Pfad kommt frisch aus /dev/urandom und
//     liegt unter einem Verzeichnis, das dieses Wegwerf-Repo nicht kennt -- weder im
//     Index noch in einer .gitignore-Regel. Vor der Heilung war dieser Fall GRUEN.
// =============================================================================
TEST(Pa1ToteAusnahme, UnmoeglicherGegenstandWirdTOT) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    // Der gewuerfelte Pfad liegt in einem gewuerfelten Verzeichnis: beide Segmente
    // kommen in keiner Datei dieses Repos vor.
    std::string const tot = "nirgends_" + marke + "/tief/" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + tot));

    Lauf const lauf = fall.fahren();
    berichten("UnmoeglicherGegenstandWirdTOT", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Eine Ausnahme, die per Konstruktion nie erloeschen kann, muss ROT sein.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "TOTE AUSNAHME"))
        << "Der Befund muss seinen Namen tragen -- sonst ist er von jedem anderen Rot nicht zu trennen.\n"
        << lauf.ausgabe;
    // Der Koeder muss WOERTLICH zurueckkommen: nur dann stammt die Meldung aus dem
    // Gegenstand, den dieser Fall angelegt hat, und nicht aus einer anderen Quelle.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, tot)) << "Der gewuerfelte Pfad fehlt in der Ausgabe.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "1 TOTE AUSNAHME")) << "Der Nenner muss die Klasse zaehlen (V-1).\n"
                                                           << lauf.ausgabe;
}

// =============================================================================
// (2) DER GEGENKOEDER (K13, andere Richtung). Derselbe Aufbau, derselbe Wuerfel -- aber
//     der Pfad liegt unter tests/unit/, einem Verzeichnis MIT getracktem Inhalt. Er
//     existiert heute nicht und kann trotzdem jederzeit entstehen. Er darf NICHT
//     beissen. Ohne diesen Fall waere Fall (1) auch von einer Wache erfuellt, die
//     jeden abwesenden Gegenstand fuer tot erklaert.
// =============================================================================
TEST(Pa1ToteAusnahme, MoeglicherGegenstandBleibtGRUEN) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    // tests/unit/ traegt in diesem Repo die Gegenprobe-Datei und die Waise -- das
    // Verzeichnis ist dem Index also bekannt.
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + lebendig));

    Lauf const lauf = fall.fahren();
    berichten("MoeglicherGegenstandBleibtGRUEN", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein Pfad unter einem bekannten Verzeichnis kann entstehen -- kein Befund.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME")) << "Fehlalarm: der Gegenstand hat einen Erzeuger.\n"
                                                           << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, marke))
        << "Die Wache muss GENAU diese Allowlist-Zeile gelesen haben -- sonst belegt das Gruen nichts.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (3) DAS BAUPRODUKT IST ERREICHBAR. Ein Pfad unter einer .gitignore-Regel ist ein
//     angemeldeter Ablageort fuer Erzeugtes: er existiert nicht, ist nicht getrackt --
//     und kann bei jedem Bau entstehen. Die zweite Haelfte des Gegenkoeders, und die
//     Stelle, an der die Falle 'check-ignore <dir>' vs. '<dir>/' sitzt.
// =============================================================================
TEST(Pa1ToteAusnahme, BauproduktUnterGitignoreIstErreichbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(".gitignore", "erzeugt_" + marke + "/\n"));
    std::string const bauprodukt = "erzeugt_" + marke + "/artefakt.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + bauprodukt));

    Lauf const lauf = fall.fahren();
    berichten("BauproduktUnterGitignoreIstErreichbar", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein ignoriertes Verzeichnis ist ein angemeldeter Ablageort -- kein Befund.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME")) << "Fehlalarm auf einem Bauprodukt.\n" << lauf.ausgabe;
}

// =============================================================================
// (4) DER SUBMODUL-PFAD IST ERREICHBAR. Ein nicht ausgechecktes Submodul steht als
//     GITLINK im Index -- sein Inhalt nicht. Ein Gegenstand darunter existiert also
//     nicht, ist nicht getrackt, ist nicht ignoriert, und kann trotzdem jederzeit
//     durch ein 'submodule update' auftauchen. Genau der Fall, den die Regel NICHT
//     faelschlich toeten darf.
// =============================================================================
TEST(Pa1ToteAusnahme, SubmodulGitlinkIstErreichbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());

    std::string const modul = "ext/fremd_" + marke;
    // Einen Gitlink von Hand in den Index legen: 160000 ist der Modus fuer 'commit'.
    // Der SHA muss kein existierendes Objekt sein -- der Index haelt ihn trotzdem.
    Lauf const idx = fahre("cd " + zitiert(fall.repo().pfad()) + " && " + WegwerfRepo::umgebung() +
                           " git update-index --add --cacheinfo 160000,"
                           "0000000000000000000000000000000000000001," +
                           modul);
    ASSERT_EQ(idx.code, 0) << "Gitlink konnte nicht in den Index gelegt werden:\n" << idx.ausgabe;

    std::string const drin = modul + "/include/kopf.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + drin));

    Lauf const lauf = fall.fahren();
    berichten("SubmodulGitlinkIstErreichbar", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein Pfad unter einem Gitlink kann jederzeit ausgecheckt werden -- kein Befund.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME"))
        << "Fehlalarm: ein nicht initialisiertes Submodul ist kein toter Gegenstand.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (5) AUSSERHALB DES REPOS WIRD NICHT GEURTEILT. Ein absoluter Pfad ist mit den
//     Quellen dieses Repos nicht beurteilbar. Die Wache darf ihn deshalb NIE tot
//     nennen -- und muss sagen, dass sie ihn nicht beurteilt hat (T-9, V-1).
// =============================================================================
TEST(Pa1ToteAusnahme, AusserhalbDesRepoWirdNichtBeurteilt) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("datei:/opt/gibtsnicht_" + marke + "/kopf.hpp"));

    Lauf const lauf = fall.fahren();
    berichten("AusserhalbDesRepoWirdNichtBeurteilt", lauf, marke);

    EXPECT_EQ(lauf.code, 0) << "Ein nicht beurteilbarer Pfad ist kein Befund.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME"))
        << "Die Wache hat ueber etwas geurteilt, das sie nicht messen kann.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "AUSSERHALB"))
        << "Der Nenner muss die nicht beurteilte Menge ausweisen -- sonst sieht sie wie geprueft aus.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (6) DIE ERSTE RICHTUNG BLEIBT ERHALTEN. Regressionsschutz: die neue Richtung darf die
//     alte nicht verdraengen. Existiert der Gegenstand, ist die Ausnahme ERLOSCHEN --
//     und ausdruecklich NICHT 'tot'. Die beiden Klassen verlangen verschiedene Abhilfen.
// =============================================================================
TEST(Pa1ToteAusnahme, RichtungEinsBleibtErhalten) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const wieder_da = "tests/unit/wieder_da_" + marke + ".hpp";
    ASSERT_TRUE(fall.repo().schreibe(wieder_da, "// der Gegenstand ist zurueck\n"));
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + wieder_da));

    Lauf const lauf = fall.fahren();
    berichten("RichtungEinsBleibtErhalten", lauf, marke);

    EXPECT_EQ(lauf.code, 1) << "Ein wieder vorhandener Gegenstand muss ROT sein.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ERLOSCHEN")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME"))
        << "Ein vorhandener Gegenstand ist erloschen, nicht tot -- die Abhilfe waere sonst falsch.\n"
        << lauf.ausgabe;
}

// =============================================================================
// (7) DIE FRIST, BEIDE SEITEN. 'frist:' ist die Form fuer die ehrlich unbeweisbare
//     Ausnahme. Sie muss tragen, solange das Datum laeuft, und ROT werden, sobald es
//     verstrichen ist -- sonst waere sie nur die naechste unbegrenzte Freistellung.
//     Beide Seiten in EINEM Fall, mit demselben Aufbau: nur das Heute unterscheidet sie.
// =============================================================================
TEST(Pa1ToteAusnahme, FristTraegtBisZumTagUndDannNichtMehr) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("frist:2026-09-15"));

    Lauf const vorher = fall.fahren("2026-09-15");
    berichten("FristTraegtBisZumTagUndDannNichtMehr/am-Tag", vorher, marke);
    EXPECT_EQ(vorher.code, 0) << "Am Tag der Frist traegt die Ausnahme noch.\n" << vorher.ausgabe;
    EXPECT_TRUE(enthaelt(vorher.ausgabe, "2026-09-15")) << "Das Datum gehoert in die Ausgabe (V-1).\n"
                                                        << vorher.ausgabe;

    Lauf const nachher = fall.fahren("2026-09-16");
    berichten("FristTraegtBisZumTagUndDannNichtMehr/danach", nachher, marke);
    EXPECT_EQ(nachher.code, 1) << "Einen Tag nach der Frist muss die Ausnahme ROT sein.\n" << nachher.ausgabe;
    EXPECT_TRUE(enthaelt(nachher.ausgabe, "ABGELAUFEN")) << nachher.ausgabe;

    // Die Herkunft des Heute muss im Nenner stehen: ein verschiebbarer Zeitbegriff, den
    // niemand sieht, waere selbst wieder ein Freibrief.
    EXPECT_TRUE(enthaelt(nachher.ausgabe, "ueberschrieben"))
        << "Die Wache verschweigt, dass ihr Heute vorgegeben wurde.\n"
        << nachher.ausgabe;
}

// =============================================================================
// (8) AM ECHTEN OBJEKT (T-7): die COMMITTETE Allowlist darf keine tote Zeile tragen.
//     Verfahren: ein Bau-Baum ueber dem ECHTEN Repo, dem GENAU die vier geparkten
//     Dateien fehlen -- damit werden genau ihre Zeilen ausgewertet, von der echten
//     Wache, gegen den echten Baum. Das ist derselbe Aufbau, der den Befund gefunden
//     hat; er bleibt ab hier stehen.
//
//     BEIDE MENGEN: geprueft sind die vier 'frist:'-Zeilen. NICHT geprueft ist die
//     'isa:'-Zeile (test_ap5_simd_extension_coherence) -- ihr Fehlen laesst sich hier
//     nicht ehrlich herstellen, s. Kopf.
//
//     NACHTRAG 2026-09-17 (Owner-Order 206): die vier 'frist:'-Zeilen gibt es nicht mehr; der
//     AUFBAU dieses Falls bleibt Byte fuer Byte derselbe, aber der GEGENSTAND ist jetzt die
//     ARCHIV-Klasse. Die vier Dateien fehlen dem Baum weiterhin -- gruen sind sie nur, weil die
//     Wache sie wegen tests/deprecated/prt_art_legacy_waisen/VERMERK.md gar nicht erst in ihren
//     SOLL nimmt. Genau das prueft der Fall ab hier: Exit 0 UND eine ARCHIV-Zeile, die alle vier
//     namentlich auffuehrt. Faellt der Anker weg, faellt dieser Fall -- und zwar laut.
//     Die 'isa:'-Zeile bleibt aus demselben Grund wie oben ungeprueft; sie ist nach dem Entscheid
//     die einzige verbliebene wirksame Zeile der committeten Allowlist.
// =============================================================================
TEST(Pa1ToteAusnahme, EchteAllowlistTraegtKeineToteZeile) {
    std::string const marke = koeder();
    fs::path const    baum  = comdare::test::user_tmp_dir() / ("pa1_echt_" + marke);
    std::error_code   ec;
    fs::create_directories(baum, ec);

    // Der SOLL der Wache, aus derselben Quelle wie bei ihr: git ls-files, ohne ext/,
    // am DATEINAMEN verankert.
    Lauf const soll = fahre("cd " + zitiert(fs::path{repo_wurzel()}) + " && " + WegwerfRepo::umgebung() +
                            " git ls-files | /usr/bin/grep -v '^ext/' | /usr/bin/grep -v '/ext/'"
                            " | /usr/bin/grep -E '(^|/)test_[^/]*[.]cpp$' | sort -u");
    ASSERT_EQ(soll.code, 0) << "git ls-files im echten Repo fehlgeschlagen:\n" << soll.ausgabe;

    std::vector<std::string> alle;
    for (std::size_t start = 0; start < soll.ausgabe.size();) {
        std::size_t const ende = soll.ausgabe.find('\n', start);
        std::string const z    = soll.ausgabe.substr(start, ende == std::string::npos ? ende : ende - start);
        if (!z.empty()) { alle.push_back(z); }
        if (ende == std::string::npos) { break; }
        start = ende + 1;
    }
    // GEGENPROBE des Messgeraets: ohne belastbaren SOLL beweist der Fall nichts.
    ASSERT_GT(alle.size(), 100U) << "Der SOLL ist unglaubwuerdig klein (" << alle.size() << ") -- fail-closed.";

    std::size_t   weggelassen = 0;
    std::ofstream aus{baum / "compile_commands.json", std::ios::trunc};
    ASSERT_TRUE(aus.good()) << "compile_commands.json nicht schreibbar";
    aus << "[\n";
    bool erste = true;
    for (auto const& datei : alle) {
        bool geparkt = false;
        for (char const* const g : kGeparkt) {
            if (datei == g) { geparkt = true; }
        }
        if (geparkt) {
            ++weggelassen;
            continue;
        }
        if (!erste) { aus << ",\n"; }
        erste = false;
        aus << "  {\"directory\": \"" << baum.string() << "\",\n"
            << "   \"command\": \"c++ -c " << repo_wurzel() << "/" << datei << "\",\n"
            << "   \"file\": \"" << repo_wurzel() << "/" << datei << "\"}";
    }
    aus << "\n]\n";
    aus.close();

    ASSERT_EQ(weggelassen, kGeparktN)
        << "Nicht alle vier geparkten Dateien standen im SOLL -- der Fall maesse etwas anderes.";
    std::size_t const soll_n = alle.size() - weggelassen;

    Lauf const lauf = fahre("cd " + zitiert(fs::path{repo_wurzel()}) + " && " + WegwerfRepo::umgebung() + " sh " +
                            zitiert(wachen_pfad()) + " " + zitiert(baum));
    // ETIKETT (Lens B LB-05, 2026-09-18): 'alle' ist der ROH-Bestand aus git ls-files; der SOLL der Wache
    // ist die Zahl NACH dem Archiv-Abzug. Beide stehen hier, in derselben Sprache wie der Nenner der
    // Wache ("N getrackte ... davon M ARCHIV ... -- SOLL: N-M").
    std::cout << "  [PA-1] Fall 'EchteAllowlistTraegtKeineToteZeile' | getrackt " << alle.size() << " | SOLL "
              << (alle.size() - weggelassen) << " | ARCHIV " << weggelassen
              << " (die vier Archiv-Dateien fehlen dem Baum) | NICHT geprueft: die 'isa:'-Zeile"
              << " | Exit " << lauf.code << "\n";

    EXPECT_EQ(lauf.code, 0) << "Die committete Allowlist traegt eine Zeile, die nie erloeschen kann.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "0 TOTE AUSNAHME")) << "Der Nenner muss die Null ausweisen (V-1).\n"
                                                           << lauf.ausgabe;
    // Die vier fehlen dem Baum und sind trotzdem gruen -- das traegt NUR die ARCHIV-Klasse.
    // Ohne diese Erwartungen waere der Fall auch von einer Wache erfuellt, die sie stillschweigend
    // uebersieht: gemessen wird deshalb die AUSGEWIESENE Menge, nicht nur der Exit.
    // FELD-ANKER statt Teilzeichenkette (Lens B LB-03, 2026-09-18): "4 archiviert" ist Teilzeichenkette
    // von "14 archiviert" -- dieselbe Klasse wie '/build' gegen '/builds' (Kopf). Gepinnt wird deshalb
    // das ganze Feld mit Komma davor und Klammer danach, und die ARCHIV-Zeile mit Datei- UND Ordner-Zahl.
    // MIT DEN GEMESSENEN ZAHLEN (Lens C LCT-09, 2026-09-18): getrackt = alle.size(), ARCHIV = weggelassen
    // (== kGeparktN), SOLL = Differenz -- alle drei Nenner-Zeilen der Wache und ihre Endzeile WOERTLICH.
    // Ein Mutant, der den SOLL falsch zaehlt oder eine Endzeile mit fremden Zahlen druckt, fiele sonst
    // durch die blossen Fragmente "0 TOTE AUSNAHME" und ", 4 archiviert)" hindurch.
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "ARCHIV (tests/deprecated/, VERMERK.md-Anker): " + z(weggelassen) +
                                           " Datei(en) in " + z(kGeparktOrdnerN) + " Ordner(n),"))
        << "Die vier archivierten Dateien muessen als Menge ausgewiesen sein (4 Dateien, 1 Ordner).\n"
        << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, z(alle.size()) + " getrackte Test-Quelldatei(en) im Baum (ohne ext/)."))
        << "Der Nenner der Wache muss den getrackten Bestand nennen, den dieser Fall selbst gezaehlt hat.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "davon " + z(weggelassen) + " ARCHIV-Datei(en) in " + z(kGeparktOrdnerN) +
                                           " Ordner(n) unter tests/deprecated/ mit VERMERK.md-Anker abgezogen"
                                           " -- SOLL: " +
                                           z(soll_n) + "."))
        << "Der Archiv-Abzug muss mit Datei-Zahl, Ordner-Zahl und dem SOLL nach dem Abzug ausgewiesen sein.\n"
        << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0)))
        << "Am echten Objekt muss die Zeile der Klassen ohne Bezug zum Bauweg leer sein (0/0/0/0/0).\n"
        << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, endzeile_ok(soll_n, weggelassen)))
        << "Die Endzeile muss den SOLL und den Archiv-Nenner mit den gemessenen Zahlen tragen.\n"
        << lauf.ausgabe;
    for (char const* const g : kGeparkt) {
        EXPECT_TRUE(enthaelt(lauf.ausgabe, std::string{g})) << "'" << g << "' fehlt in der ARCHIV-Liste der Wache.\n"
                                                            << lauf.ausgabe;
    }
    fs::remove_all(baum, ec);
}

// =============================================================================
// (9) T-6 SCHWESTERPFLICHT -- und eine BERICHTIGUNG der urspruenglichen Annahme.
//
//     Die Annahme lautete: dieselben vier stehen "aus dem spiegelbildlichen Grund" auch
//     in scripts/ci_test_sichtbarkeit_allowlist.txt. Am Objekt gemessen (2026-08-10)
//     stimmt das NICHT MEHR, und zwar zu Recht. Die beiden Stufen haben verschiedene
//     SOLL-Quellen:
//       Stufe 1  SOLL = git ls-files          -> die vier .cpp liegen weiter im Baum
//       Stufe 2  SOLL = die Registrierungs-AUFRUFE im CMake-Quelltext -> seit 8a8f6877
//                                                 entfernt, die vier kommen dort nicht
//                                                 mehr vor
//     Eine Zeile in der Schwesterliste waere daher eine Ausnahme OHNE ANLASS -- und die
//     faengt nur das naechste echte Verschwinden desselben Namens lautlos weg. Genau
//     diese Fehlerform steht im Kopf jener Datei.
//
//     DIESER FALL VERLANGT ALSO DAS GEGENTEIL DER ANNAHME: keiner der vier Namen darf
//     dort als WIRKSAME Zeile stehen. 'Wirksam' heisst: keine Kommentarzeile. Die vier
//     Namen kommen im Kopf jener Datei sehr wohl vor -- in der Begruendung, warum sie
//     entfernt wurden. Ein Orakel, das die ganze Datei nach dem Namen absucht, wuerde
//     diesen Kommentar fuer eine Zeile halten und waere hohl; deshalb wird hier Zeile
//     fuer Zeile das ERSTE FELD verglichen und nicht die Datei nach Teilzeichenketten.
//
//     GEGENPROBE (sonst bewiese ein Nullbefund nichts): der Parser MUSS wirksame Zeilen
//     finden. Findet er keine, ist nicht die Liste sauber, sondern der Parser kaputt.
// =============================================================================
TEST(Pa1ToteAusnahme, T6SchwesterlisteFuehrtDieVierNichtMehr) {
    fs::path const schwester = fs::path{repo_wurzel()} / "scripts/ci_test_sichtbarkeit_allowlist.txt";
    std::ifstream  quelle{schwester};
    ASSERT_TRUE(quelle.good()) << "Schwesterliste nicht lesbar: " << schwester.string();

    std::vector<std::string> wirksam;
    std::string              zeile;
    while (std::getline(quelle, zeile)) {
        std::size_t const erst = zeile.find_first_not_of(" \t\r");
        if (erst == std::string::npos) { continue; }
        if (zeile[erst] == '#') { continue; }
        std::size_t const ende = zeile.find_first_of(" \t", erst);
        wirksam.push_back(zeile.substr(erst, ende == std::string::npos ? ende : ende - erst));
    }

    // GEGENPROBE des Messgeraets vor jedem Nullbefund.
    ASSERT_FALSE(wirksam.empty()) << "Der Parser fand KEINE wirksame Zeile in " << schwester.string()
                                  << " -- ein Nullbefund waere dann eine Aussage ueber den Parser, "
                                     "nicht ueber die Liste. Fail-closed.";

    for (char const* const g : kGeparkt) {
        std::string const datei = std::string{g};
        std::string const name  = datei.substr(datei.rfind('/') + 1, datei.size() - datei.rfind('/') - 1 - 4);
        bool              steht = false;
        for (auto const& w : wirksam) {
            if (w == name) { steht = true; }
        }
        EXPECT_FALSE(steht) << "'" << name << "' steht wieder als wirksame Zeile in " << schwester.string()
                            << ". Seine Registrierung ist aus tests/unit/CMakeLists.txt entfernt, der Name "
                               "kommt im SOLL jener Wache also gar nicht vor -- die Zeile deckt nichts und "
                               "faengt nur das naechste echte Verschwinden desselben Namens lautlos weg.";
    }

    std::cout << "  [PA-1/T-6] Schwesterliste " << schwester.filename().string() << ": " << wirksam.size()
              << " wirksame Zeile(n), davon 0 der 4 geparkten -- richtig, ihr SOLL kennt sie nicht mehr.\n"
              << "  [PA-1/T-6] NICHT geprueft und ausdruecklich OFFEN: jene Liste hat ueberhaupt kein\n"
                 "             Gegenstand-Feld (Format '<name> <begruendung>'). Ihre Wache kennt daher\n"
                 "             KEINE Erloschen-Probe -- weder Richtung 1 noch Richtung 2. Struktur-Nenner:\n"
              << "             " << wirksam.size() << " von " << wirksam.size()
              << " Zeilen ohne nachpruefbaren Gegenstand. Eigenes Paket.\n";
}

// =============================================================================
// (10) DIE ARCHIV-KLASSE, BEIDSEITIG UND MIT NACHBAR-PROBE (Owner 2026-09-17, Order 206).
//      Die Regel lautet: eine getrackte Test-Quelldatei unter tests/deprecated/<ordner>/
//      faellt aus dem SOLL, WENN UND NUR WENN tests/deprecated/<ordner>/VERMERK.md im
//      Index liegt. Dieser Fall faehrt alle drei Zustaende am SELBEN Gegenstand:
//        (a) kein Anker            -> ROT, die Waise steht namentlich als unbegruendet
//        (b) Anker im NACHBARordner -> weiter ROT (ein Anker traegt NUR seinen Ordner)
//        (c) Anker im eigenen Ordner -> GRUEN, die Waise steht in der ARCHIV-Zeile
//      K13 beidseitig: ein Orakel, das immer GRUEN liefert, faellt an (a) und (b); eines,
//      das immer ROT liefert, faellt an (c). (b) ist der Teil, der aus dem "wenn" ein
//      "wenn und nur wenn" macht -- ohne ihn waere die Regel "irgendwo unter
//      tests/deprecated/ liegt ein VERMERK.md" und damit genau der Freibrief, gegen den
//      diese Wache gebaut ist.
//      (a)-(c) AUSDRUECKLICH OHNE ALLOWLIST-ZEILE: die Archiv-Klasse darf nicht an einer Ausnahme
//      haengen, sonst pruefte der Fall die Allowlist und nicht den Ort.
//        (d) Anker PLUS Allowlist-Zeile -> ROT: die Zeile ist eine Ausnahme OHNE ANLASS (LB-01)
//        (e) Anker, Allowlist ohne die Zeile -> wieder GRUEN (Gegenrichtung zu (d))
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivOrdnerZaehltNurMitVermerkAnker) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/archiv_" + marke;
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());

    Lauf const ohne = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/ohne-Anker", ohne, marke);
    EXPECT_EQ(ohne.code, 1) << "Ohne VERMERK.md bleibt die Datei im SOLL -- und fehlt im Bauweg.\n" << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, fall.waise()))
        << "Der gewuerfelte Pfad fehlt in der Ausgabe -- der Befund stammt dann nicht aus diesem Fall.\n"
        << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, ", 0 archiviert)"))
        << "Die Archiv-Klasse gehoert auch dann in den Nenner, wenn sie leer ist (V-1).\n"
        << ohne.ausgabe;

    ASSERT_TRUE(fall.repo().schreibe_und_verfolge("tests/deprecated/anderer_" + marke + "/VERMERK.md",
                                                  "# Nachbar-Anker " + marke + "\n"));
    Lauf const nachbar = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/Nachbar-Anker", nachbar, marke);
    EXPECT_EQ(nachbar.code, 1) << "Ein Anker im NACHBARordner darf nicht tragen.\n" << nachbar.ausgabe;
    EXPECT_TRUE(enthaelt(nachbar.ausgabe, ", 0 archiviert)")) << "Der fremde Anker hat etwas archiviert.\n"
                                                              << nachbar.ausgabe;
    // ROT AM GEGENSTAND (Lens C LCT-08, 2026-09-18): nicht irgendein Rot, sondern die Waise OHNE BEGRUENDUNG.
    EXPECT_TRUE(enthaelt(nachbar.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << nachbar.ausgabe;
    EXPECT_TRUE(enthaelt(nachbar.ausgabe, fall.waise()))
        << "Der Waisen-Pfad fehlt -- das Rot stammt dann nicht aus diesem Fall.\n"
        << nachbar.ausgabe;

    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Archiv-Anker " + marke + "\n"));
    Lauf const mit = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/mit-Anker", mit, marke);
    EXPECT_EQ(mit.code, 0) << "Mit eigenem VERMERK.md ist die Datei ARCHIV und nicht mehr im SOLL.\n" << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "ARCHIV (tests/deprecated/, VERMERK.md-Anker): 1 Datei(en) in 1 Ordner(n),"))
        << "Die Archiv-Menge muss mit Datei- und Ordner-Zahl ausgewiesen sein, sonst schrumpft der Nenner lautlos.\n"
        << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, fall.waise())) << "Die archivierte Datei muss namentlich erscheinen.\n"
                                                     << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, ", 1 archiviert)")) << "Die Endzeile muss den Archiv-Nenner tragen.\n"
                                                          << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << mit.ausgabe;

    // (d) EINE ALLOWLIST-ZEILE FUER DIE ARCHIVIERTE DATEI (Lens B LB-01, 2026-09-18). Die Allowlist-
    //     Schleife der Wache laeuft nur ueber die Dateien, die dem SOLL fehlen; eine archivierte Datei
    //     steht nicht im SOLL, ihre Zeile wuerde also NIE ausgewertet -- eine abgelaufene Frist bliebe
    //     still gruen (Lens-B-Probe P10: 'frist:2000-01-01' -> Exit 0). Genau das ist die "Ausnahme
    //     OHNE ANLASS" aus Fall (9). Die Wache muss die Zeile als UNPRUEFBAR melden: ROT, namentlich,
    //     und mit "0 erloschen" -- sie darf die Frist gar nicht erst bewerten, der Ort traegt.
    //     ROT ZUERST: gegen die Wache vom Stand 54296857 war diese Stufe GRUEN (Exit 0) -- Beleg im
    //     Beweisort des OV-2-Zuges (FIX-r1.md).
    ASSERT_TRUE(fall.allowlist_setzen("frist:2000-01-01"));
    Lauf const zeile = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/Anker-plus-Allowlist-Zeile", zeile, marke);
    EXPECT_EQ(zeile.code, 1) << "Eine Allowlist-Zeile fuer eine ARCHIV-Datei ist eine Ausnahme ohne Anlass -- ROT.\n"
                             << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, fall.waise() + " -- UNPRUEFBAR: ARCHIV-Datei mit Allowlist-Zeile"))
        << "Die Zeile muss namentlich und mit ihrer Klasse gemeldet werden.\n"
        << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, "Ausnahme ohne Anlass")) << zeile.ausgabe;
    // GANZE ZEILEN statt Fragmente (Lens C LCT-10/LCT-11): Endzeile mit "1 unpruefbar" und ", 1 archiviert)",
    // die Klassen-Zeile mit "1 Allowlist-Zeile(n) fuer ARCHIV-Dateien", und "0 erloschen" in der Endzeile --
    // die Frist darf nicht bewertet worden sein, die Zeile hat keinen Gegenstand im SOLL.
    EXPECT_TRUE(enthaelt(zeile.ausgabe, endzeile_rot(0, 1, 0, 0, 1, 1)))
        << "Die Endzeile muss die Zeile als UNPRUEFBAR zaehlen, die Datei bleibt ARCHIV, 0 erloschen.\n"
        << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, nenner_ohne_bauweg(0, 1, 0, 0, 0)))
        << "Der Nenner muss die Klasse getrennt zaehlen (V-1), in der ganzen Zeile.\n"
        << zeile.ausgabe;
    EXPECT_TRUE(enthaelt(zeile.ausgabe, nenner_davon(0, 0, 0, 0, 0)))
        << "Keine dem Bauweg fehlende Datei ist bewertet worden -- der Ort traegt.\n"
        << zeile.ausgabe;

    // (e) GEGENRICHTUNG zu (d): dieselbe Allowlist-Datei OHNE die Zeile -> wieder GRUEN. Damit ist
    //     belegt, dass (d) an der ZEILE hing und nicht an der blossen Anwesenheit einer Allowlist.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt", "# ohne Zeile " + marke + "\n"));
    Lauf const ohne_zeile = fall.fahren();
    berichten("ArchivOrdnerZaehltNurMitVermerkAnker/Anker-ohne-Allowlist-Zeile", ohne_zeile, marke);
    EXPECT_EQ(ohne_zeile.code, 0) << "Ohne die Zeile muss der Anker allein wieder tragen.\n" << ohne_zeile.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_zeile.ausgabe, endzeile_ok(1, 1))) << ohne_zeile.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_zeile.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << ohne_zeile.ausgabe;
}

// =============================================================================
// (11) DIE DREI GRENZEN DER ARCHIV-REGEL, je mit Test-Traeger (Lens B LB-02, 2026-09-18).
//      Der Kopf der Wache behauptet drei Grenzen; am Objekt stimmten sie (Lens-Proben P1/P2/P4),
//      aber "Skripte sagen gar nichts" -- ohne Google-Test-Deckung bleibt jede Grenze Behauptung:
//        (a) eine Datei DIREKT unter tests/deprecated/ (ohne Ordner) hat keinen Anker und bleibt im
//            SOLL -- auch wenn tests/deprecated/VERMERK.md im Index liegt;
//        (b) ein VERMERK.md TIEFER als das dritte Pfadsegment ankert nichts -- der Anker des dritten
//            Segments dagegen traegt auch tiefer liegende Dateien (Gegenrichtung);
//        (c) ein VERMERK.md, das nur im ARBEITSBAUM liegt (nicht im Index), ankert nichts --
//            dieselbe Datei per 'git add' traegt (Gegenrichtung).
//      Jede Stufe misst Exit-Code UND Literal: die OHNE-BEGRUENDUNG-Zeile mit dem Waisen-Pfad und
//      den Archiv-Nenner ", 0 archiviert)" (Feld-Anker, s. Fall (8)).
//      ROT ZUERST am Mutanten: jede Stufe wurde gegen eine absichtlich aufgeweichte Kopie der Wache
//      gefahren (Anker-Muster ohne Segment-Grenze; Anker aus dem Arbeitsbaum statt aus dem Index)
//      und war dort rot -- Beleg im Beweisort des OV-2-Zuges (FIX-r1.md).
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivGrenzeDateiDirektUnterDeprecatedBleibtImSoll) {
    std::string const marke = koeder();
    Fall              fall{marke, "tests/deprecated/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    // Der Anker liegt eine Ebene ZU HOCH: tests/deprecated/VERMERK.md ist kein <ordner>-Anker.
    ASSERT_TRUE(
        fall.repo().schreibe_und_verfolge("tests/deprecated/VERMERK.md", "# kein Ordner-Anker " + marke + "\n"));

    Lauf const lauf = fall.fahren();
    berichten("ArchivGrenzeDateiDirektUnterDeprecatedBleibtImSoll", lauf, marke);
    EXPECT_EQ(lauf.code, 1) << "Eine Datei direkt unter tests/deprecated/ hat keinen Anker und bleibt im SOLL.\n"
                            << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, fall.waise())) << "Der gewuerfelte Pfad fehlt in der Ausgabe.\n" << lauf.ausgabe;
    EXPECT_TRUE(enthaelt(lauf.ausgabe, ", 0 archiviert)")) << "tests/deprecated/VERMERK.md hat etwas archiviert.\n"
                                                           << lauf.ausgabe;

    // (b) GEGENRICHTUNG (Lens C LCT-07, 2026-09-18): DERSELBE Baum, dieselbe Regel. Eine zweite Waise unter
    //     einem <ordner> mit eigenem VERMERK.md wird archiviert; die Datei direkt unter tests/deprecated/
    //     nimmt der Bauweg jetzt auf, damit allein der ORT den Unterschied zwischen (a) und (b) macht.
    std::string const ordner = "tests/deprecated/mit_ordner_" + marke;
    std::string const zweite = ordner + "/test_waise2_" + marke + ".cpp";
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(zweite, "// zweite Waise " + marke + "\n"));
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Ordner-Anker " + marke + "\n"));
    ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe, fall.waise()}));
    Lauf const mit_ordner = fall.fahren();
    berichten("ArchivGrenzeDateiDirektUnterDeprecatedBleibtImSoll/Gegenrichtung-Ordner-Anker", mit_ordner, marke);
    EXPECT_EQ(mit_ordner.code, 0) << "Unter einem <ordner> mit Anker muss dieselbe Regel gruen tragen.\n"
                                  << mit_ordner.ausgabe;
    EXPECT_TRUE(enthaelt(mit_ordner.ausgabe, zweite)) << "Die archivierte Datei muss namentlich erscheinen.\n"
                                                      << mit_ordner.ausgabe;
    EXPECT_TRUE(enthaelt(mit_ordner.ausgabe, endzeile_ok(2, 1))) << mit_ordner.ausgabe;
}

TEST(Pa1ToteAusnahme, ArchivGrenzeAnkerTieferAlsDrittesSegmentAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/tief_" + marke;
    Fall              fall{marke, ordner + "/unter/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    // Der Anker liegt NEBEN der Datei, aber im VIERTEN Segment -- <ordner> ist genau das dritte.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/unter/VERMERK.md", "# zu tiefer Anker " + marke + "\n"));

    Lauf const tief = fall.fahren();
    berichten("ArchivGrenzeAnkerTieferAlsDrittesSegmentAnkertNicht/Anker-im-vierten-Segment", tief, marke);
    EXPECT_EQ(tief.code, 1) << "Ein VERMERK.md tiefer als das dritte Pfadsegment darf nicht ankern.\n" << tief.ausgabe;
    EXPECT_TRUE(enthaelt(tief.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << tief.ausgabe;
    EXPECT_TRUE(enthaelt(tief.ausgabe, fall.waise())) << tief.ausgabe;
    EXPECT_TRUE(enthaelt(tief.ausgabe, ", 0 archiviert)")) << "Der zu tiefe Anker hat etwas archiviert.\n"
                                                           << tief.ausgabe;

    // GEGENRICHTUNG: der Anker im dritten Segment traegt auch die tiefer liegende Datei.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Ordner-Anker " + marke + "\n"));
    Lauf const drittes = fall.fahren();
    berichten("ArchivGrenzeAnkerTieferAlsDrittesSegmentAnkertNicht/Anker-im-dritten-Segment", drittes, marke);
    EXPECT_EQ(drittes.code, 0) << "Der Anker im dritten Segment muss den ganzen Ordner tragen.\n" << drittes.ausgabe;
    EXPECT_TRUE(enthaelt(drittes.ausgabe, fall.waise())) << drittes.ausgabe;
    EXPECT_TRUE(enthaelt(drittes.ausgabe, ", 1 archiviert)")) << drittes.ausgabe;
}

TEST(Pa1ToteAusnahme, ArchivGrenzeAnkerNurImArbeitsbaumAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/ungetrackt_" + marke;
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    // repo().schreibe statt schreibe_und_verfolge: die Datei liegt im Arbeitsbaum, NICHT im Index.
    ASSERT_TRUE(fall.repo().schreibe(ordner + "/VERMERK.md", "# ungetrackter Anker " + marke + "\n"));
    ASSERT_FALSE(fall.repo().ist_verfolgt(ordner + "/VERMERK.md"))
        << "Das Arrangement ist falsch: der Anker steht im Index, die Stufe maesse nichts.";

    Lauf const ungetrackt = fall.fahren();
    berichten("ArchivGrenzeAnkerNurImArbeitsbaumAnkertNicht/nur-Arbeitsbaum", ungetrackt, marke);
    EXPECT_EQ(ungetrackt.code, 1) << "Ein VERMERK.md nur im Arbeitsbaum darf nicht ankern.\n" << ungetrackt.ausgabe;
    EXPECT_TRUE(enthaelt(ungetrackt.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << ungetrackt.ausgabe;
    EXPECT_TRUE(enthaelt(ungetrackt.ausgabe, fall.waise())) << ungetrackt.ausgabe;
    EXPECT_TRUE(enthaelt(ungetrackt.ausgabe, ", 0 archiviert)")) << "Der ungetrackte Anker hat etwas archiviert.\n"
                                                                 << ungetrackt.ausgabe;

    // GEGENRICHTUNG: dieselbe Datei im Index traegt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# jetzt getrackter Anker " + marke + "\n"));
    Lauf const getrackt = fall.fahren();
    berichten("ArchivGrenzeAnkerNurImArbeitsbaumAnkertNicht/im-Index", getrackt, marke);
    EXPECT_EQ(getrackt.code, 0) << "Derselbe Anker im Index muss tragen.\n" << getrackt.ausgabe;
    EXPECT_TRUE(enthaelt(getrackt.ausgabe, ", 1 archiviert)")) << getrackt.ausgabe;
}

// =============================================================================
// (12) DIE FORM DES ANKERS (Lens A LA-04, 2026-09-18). Der Anker traegt die Begruendung der
//      Ablage. Ein Index-Eintrag namens VERMERK.md, der ein Blob mit 0 Byte oder kein regulaeres
//      Blob ist (Symlink, Modus 120000), traegt nichts -- und darf deshalb NICHT ankern. Die
//      Doktrin der Wache macht aus einer leeren Begruendung ROT (Feld 2 leer = UNPRUEFBAR); vor
//      diesem Fall war der Anker die einzige Stelle, an der Leere gruen trug. Drei Stufen am
//      selben Gegenstand:
//        (a) VERMERK.md mit 0 Byte im Index   -> ROT: UNPRUEFBARER ANKER, Waise im SOLL, 0 archiviert
//        (a2) VERMERK.md nur aus Leerraum      -> ROT: UNPRUEFBARER ANKER (ohne Nicht-Leerraum-Zeichen)
//        (b) VERMERK.md als Symlink ins Nichts -> ROT: UNPRUEFBARER ANKER (Modus 120000), 0 archiviert
//        (c) VERMERK.md mit Inhalt             -> GRUEN, 1 archiviert (Gegenrichtung)
//        (d) dasselbe Blob mit Modus 100755    -> GRUEN, 1 archiviert (ausfuehrbar ist regulaer)
//      ROT ZUERST: (a) und (b) waren gegen die Wache vom Stand 54296857 GRUEN (Exit 0, "1 archiviert")
//      -- Beleg im Beweisort des OV-2-Zuges (FIX-r1.md); (a2) war gegen d8e8f53d GRUEN (Lens A LA3-01,
//      Lens B LB2-01, Fix-r2 F1) und (d) faellt am Mutanten, der nur 100644 zulaesst (Lens C LCT-12) --
//      Belege in FIX-r2.md.
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivAnkerOhneInhaltOderFormAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/hohl_" + marke;
    std::string const anker  = ordner + "/VERMERK.md";
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());

    // (a) 0 Byte, im Index.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, ""));
    Lauf const leer = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/0-Byte", leer, marke);
    EXPECT_EQ(leer.code, 1) << "Ein Anker mit 0 Byte traegt keine Begruendung und darf nicht ankern.\n" << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, anker + " -- UNPRUEFBARER ANKER"))
        << "Der Anker muss namentlich und mit seiner Klasse gemeldet werden.\n"
        << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, "UNPRUEFBARER ANKER: Blob mit 0 Byte --")) << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, fall.waise())) << leer.ausgabe;
    // GANZE ZEILEN (Lens C LCT-10/LCT-11): Endzeile "1 von 2 ohne Begruendung ... 1 unpruefbar, 0 archiviert",
    // Klassen-Zeile "1 ARCHIV-Anker ohne Inhalt/Form" mit allen Nachbarfeldern.
    EXPECT_TRUE(enthaelt(leer.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0)))
        << "Die Endzeile muss den Anker als UNPRUEFBAR zaehlen und 0 archiviert melden.\n"
        << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0)))
        << "Der Nenner muss die Klasse getrennt zaehlen (V-1).\n"
        << leer.ausgabe;

    // (a2) NUR LEERRAUM (Lens A LA3-01, Lens B LB2-01; Fix-r2 F1): fuenf Byte aus Zeilenumbruechen, einem
    //      Leerzeichen und einem Tab -- Bytes ohne Inhalt. Die Groesse allein hatte getragen (Exit 0 gegen
    //      d8e8f53d, "1 archiviert"); jetzt zaehlt der Inhalt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "\n\n \t\n"));
    Lauf const leerraum = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/nur-Leerraum", leerraum, marke);
    EXPECT_EQ(leerraum.code, 1) << "Ein Anker aus Leerraum traegt keine Begruendung und darf nicht ankern.\n"
                                << leerraum.ausgabe;
    EXPECT_TRUE(
        enthaelt(leerraum.ausgabe, anker + " -- UNPRUEFBARER ANKER: Blob mit 5 Byte, aber ohne Nicht-Leerraum-Zeichen"))
        << "Der Anker muss namentlich, mit Byte-Zahl und Klasse gemeldet werden.\n"
        << leerraum.ausgabe;
    EXPECT_TRUE(enthaelt(leerraum.ausgabe, fall.waise())) << leerraum.ausgabe;
    EXPECT_TRUE(enthaelt(leerraum.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << leerraum.ausgabe;
    EXPECT_TRUE(enthaelt(leerraum.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << leerraum.ausgabe;

    // (b) derselbe Pfad als Symlink auf ein Ziel, das es nicht gibt. 'git add' legt ihn mit Modus
    //     120000 in den Index; das Blob traegt nur den Link-Text.
    fs::path const  anker_abs = fall.repo().pfad() / anker;
    std::error_code ec;
    fs::remove(anker_abs, ec);
    ASSERT_FALSE(fs::exists(anker_abs)) << "Der 0-Byte-Anker liess sich nicht entfernen.";
    fs::create_symlink("nirgends_" + marke, anker_abs, ec);
    ASSERT_FALSE(ec) << "Symlink nicht anlegbar: " << ec.message();
    Lauf const add =
        fahre("cd " + zitiert(fall.repo().pfad()) + " && " + WegwerfRepo::umgebung() + " git add -- " + zitiert(anker));
    ASSERT_EQ(add.code, 0) << "git add des Symlinks fehlgeschlagen:\n" << add.ausgabe;
    Lauf const modus = fahre("cd " + zitiert(fall.repo().pfad()) + " && " + WegwerfRepo::umgebung() +
                             " git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(enthaelt(modus.ausgabe, "120000 ")) << "Das Arrangement ist falsch: kein Symlink im Index:\n"
                                                    << modus.ausgabe;
    Lauf const symlink = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/Symlink", symlink, marke);
    EXPECT_EQ(symlink.code, 1) << "Ein Symlink namens VERMERK.md ist kein Anker.\n" << symlink.ausgabe;
    EXPECT_TRUE(enthaelt(symlink.ausgabe, anker + " -- UNPRUEFBARER ANKER")) << symlink.ausgabe;
    EXPECT_TRUE(enthaelt(symlink.ausgabe, "Index-Modus 120000 ist kein regulaeres Blob"))
        << "Der Modus gehoert in die Meldung (V-1).\n"
        << symlink.ausgabe;
    // ROT AM GEGENSTAND (Lens C LCT-08): die Waise steht OHNE BEGRUENDUNG, die Klassen-Zeile zaehlt den Anker.
    EXPECT_TRUE(enthaelt(symlink.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << symlink.ausgabe;
    EXPECT_TRUE(enthaelt(symlink.ausgabe, fall.waise())) << symlink.ausgabe;
    EXPECT_TRUE(enthaelt(symlink.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << symlink.ausgabe;
    EXPECT_TRUE(enthaelt(symlink.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << symlink.ausgabe;

    // (c) GEGENRICHTUNG: derselbe Pfad mit Inhalt traegt.
    fs::remove(anker_abs, ec);
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "# Archiv-Anker mit Inhalt " + marke + "\n"));
    Lauf const voll = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/mit-Inhalt", voll, marke);
    EXPECT_EQ(voll.code, 0) << "Derselbe Pfad mit Inhalt muss tragen.\n" << voll.ausgabe;
    EXPECT_TRUE(enthaelt(voll.ausgabe, endzeile_ok(1, 1))) << voll.ausgabe;
    EXPECT_TRUE(enthaelt(voll.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << voll.ausgabe;

    // (d) MODUS 100755 (Lens C LCT-12, 2026-09-18): ein ausfuehrbares Blob ist ein regulaeres Blob und ankert.
    //     ROT ZUERST am Mutanten M4 (nur 100644 zugelassen): dort ist diese Stufe Exit 1 -- Beweisort FIX-r2.md.
    Lauf const chmod = im_repo(fall.repo(), "git update-index --chmod=+x -- " + zitiert(anker));
    ASSERT_EQ(chmod.code, 0) << "git update-index --chmod=+x fehlgeschlagen:\n" << chmod.ausgabe;
    Lauf const modus_x = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(enthaelt(modus_x.ausgabe, "100755 ")) << "Das Arrangement ist falsch: kein 100755 im Index:\n"
                                                      << modus_x.ausgabe;
    Lauf const ausfuehrbar = fall.fahren();
    berichten("ArchivAnkerOhneInhaltOderFormAnkertNicht/Modus-100755", ausfuehrbar, marke);
    EXPECT_EQ(ausfuehrbar.code, 0) << "Ein Blob mit Modus 100755 ist regulaer und muss ankern.\n"
                                   << ausfuehrbar.ausgabe;
    EXPECT_TRUE(enthaelt(ausfuehrbar.ausgabe, endzeile_ok(1, 1))) << ausfuehrbar.ausgabe;
    EXPECT_TRUE(enthaelt(ausfuehrbar.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << ausfuehrbar.ausgabe;
}

// =============================================================================
// (12b) DER ANKER IM MERGE-KONFLIKT (Lens C LCW-01, 2026-09-18). Im Konflikt fuehrt der Index einen Pfad
//       auf den Stufen 1 (Basis), 2 (ours) und 3 (theirs) -- ohne Stufe 0, also ohne aufgeloeste Fassung.
//       'git ls-files -s' listet ihn dann bis zu dreimal, jede Zeile mit gueltigem Modus und lesbarem
//       Blob; die Fassung d8e8f53d nahm jede davon als Anker (Exit 0, "1 archiviert" -- Lens A Probe Z1).
//       Ein Anker ohne aufgeloeste Fassung traegt keine Begruendung: UNPRUEFBAR, einmal gemeldet (nicht
//       dreimal), die Datei bleibt im SOLL. Gegenrichtung: derselbe Pfad per 'git add' aufgeloest traegt.
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivAnkerImMergeKonfliktAnkertNicht) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/konflikt_" + marke;
    std::string const anker  = ordner + "/VERMERK.md";
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());

    // (a) aufgeloest (Stufe 0): traegt -- das Arrangement vor dem Konflikt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "# Anker vor dem Konflikt " + marke + "\n"));
    Lauf const vorher = fall.fahren();
    berichten("ArchivAnkerImMergeKonfliktAnkertNicht/Stufe-0", vorher, marke);
    EXPECT_EQ(vorher.code, 0) << vorher.ausgabe;
    EXPECT_TRUE(enthaelt(vorher.ausgabe, endzeile_ok(1, 1))) << vorher.ausgabe;

    // (b) der Konflikt: das Blob bleibt in der Objektdatenbank, Stufe 0 wird entfernt, Stufen 1-3 gesetzt
    //     (git update-index --index-info, das Rezept der git-Dokumentation).
    Lauf const sha = im_repo(fall.repo(), "git hash-object -w -- " + zitiert(anker));
    ASSERT_EQ(sha.code, 0) << "git hash-object fehlgeschlagen:\n" << sha.ausgabe;
    ASSERT_EQ(sha.ausgabe.size(), 40U) << "kein SHA-1: '" << sha.ausgabe << "'";
    std::string const rezept = "0 0000000000000000000000000000000000000000\t" + anker + "\n" + "100644 " + sha.ausgabe +
                               " 1\t" + anker + "\n" + "100644 " + sha.ausgabe + " 2\t" + anker + "\n" + "100644 " +
                               sha.ausgabe + " 3\t" + anker + "\n";
    ASSERT_TRUE(fall.repo().schreibe("konflikt_" + marke + ".txt", rezept));
    Lauf const konflikt = im_repo(fall.repo(), "git update-index --index-info < " +
                                                   zitiert(fall.repo().pfad() / ("konflikt_" + marke + ".txt")));
    ASSERT_EQ(konflikt.code, 0) << "git update-index --index-info fehlgeschlagen:\n" << konflikt.ausgabe;
    Lauf const stufen = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(enthaelt(stufen.ausgabe, " 1\t" + anker)) << "Arrangement: keine Stufe 1:\n" << stufen.ausgabe;
    ASSERT_TRUE(enthaelt(stufen.ausgabe, " 3\t" + anker)) << "Arrangement: keine Stufe 3:\n" << stufen.ausgabe;
    ASSERT_FALSE(enthaelt(stufen.ausgabe, " 0\t" + anker)) << "Arrangement: Stufe 0 steht noch:\n" << stufen.ausgabe;

    Lauf const im_konflikt = fall.fahren();
    berichten("ArchivAnkerImMergeKonfliktAnkertNicht/Stufen-1-2-3", im_konflikt, marke);
    EXPECT_EQ(im_konflikt.code, 1) << "Ein Anker im Merge-Konflikt hat keine aufgeloeste Fassung -- ROT.\n"
                                   << im_konflikt.ausgabe;
    EXPECT_TRUE(enthaelt(im_konflikt.ausgabe,
                         anker + " -- UNPRUEFBARER ANKER: Index-Stufe 1 statt 0 (Merge-Konflikt, keine aufgeloeste"))
        << "Der Anker muss namentlich, mit Stufe und Klasse gemeldet werden.\n"
        << im_konflikt.ausgabe;
    EXPECT_TRUE(enthaelt(im_konflikt.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << im_konflikt.ausgabe;
    EXPECT_TRUE(enthaelt(im_konflikt.ausgabe, fall.waise())) << im_konflikt.ausgabe;
    // EINMAL gezaehlt, obwohl der Index den Pfad dreimal fuehrt.
    EXPECT_TRUE(enthaelt(im_konflikt.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0)))
        << "Drei Stufen sind EIN unpruefbarer Anker, nicht drei.\n"
        << im_konflikt.ausgabe;
    EXPECT_TRUE(enthaelt(im_konflikt.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << im_konflikt.ausgabe;

    // (c) GEGENRICHTUNG: 'git add' loest den Konflikt (Stufe 0, Stufen 1-3 weg) -- derselbe Pfad traegt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "# Anker nach dem Konflikt " + marke + "\n"));
    Lauf const geloest_probe = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(enthaelt(geloest_probe.ausgabe, " 0\t" + anker)) << geloest_probe.ausgabe;
    ASSERT_FALSE(enthaelt(geloest_probe.ausgabe, " 1\t" + anker)) << geloest_probe.ausgabe;
    Lauf const geloest = fall.fahren();
    berichten("ArchivAnkerImMergeKonfliktAnkertNicht/aufgeloest", geloest, marke);
    EXPECT_EQ(geloest.code, 0) << "Der aufgeloeste Anker muss wieder tragen.\n" << geloest.ausgabe;
    EXPECT_TRUE(enthaelt(geloest.ausgabe, endzeile_ok(1, 1))) << geloest.ausgabe;
}

// =============================================================================
// (13) DIE SCHWESTERKLASSE ZU (10d) (Lens A LA-08, 2026-09-18): eine Allowlist-Zeile, deren Feld 1
//      KEINE Datei des SOLL-Bestands nennt (getrackte Test-Quelldatei ausserhalb ext/: geloescht,
//      umbenannt, unter ext/, anders geschrieben), wird genauso nie ausgewertet -- und erwacht mit dem
//      naechsten Namensgleichen als Freibrief. Der Fall hiess bis Fix-r2 "...ImIndex..."; das Etikett
//      war falsch (Lens A LA3-02, Lens C LCW-07): eine Datei unter ext/ steht im Index, aber nicht im
//      SOLL-Bestand -- die Referenzmenge ist soll_roh.txt, nicht der Index.
//      Aufbau: eine BEGRUENDETE Waise (Zeile wie in Fall (2), sie traegt) plus eine zweite Zeile fuer
//      einen Geist. ROT darf dann NUR die Geist-Zeile sein: "davon 1 begruendet" bleibt stehen.
//      Gegenrichtung: dieselbe Allowlist ohne die Geist-Zeile -> GRUEN.
//        (c) Zeilen NUR aus Leerraum sind Leerzeilen, keine Datenzeilen (Lens A LA3-03, Fix-r2 F3):
//            gruen, so wie test_mt_l4_registrierungs_wache_isa Fall (8) sie liest. Gegen d8e8f53d ROT.
//        (d) ein EINGERUECKTER Kommentar ist fuer beide Leser eine Datenzeile ohne Gegenstand (Lens A
//            LA3-04, Lens B LB2-02): rot, und die Meldung nennt die naheliegende Ursache (M1).
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistZeileOhneGegenstandImSollBestandIstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const geist    = "tests/unit/test_geist_" + marke + ".cpp";
    std::string const tragend  = fall.waise() + " | datei:" + lebendig + " | Koeder " + marke + "\n";
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend + geist +
                                         " | frist:2999-12-31 | Geist " + marke + "\n"));

    Lauf const mit = fall.fahren();
    berichten("AllowlistZeileOhneGegenstandImSollBestandIstUnpruefbar/mit-Geist-Zeile", mit, marke);
    EXPECT_EQ(mit.code, 1) << "Eine Zeile ohne Gegenstand im SOLL-Bestand ist ein schlafender Freibrief -- ROT.\n"
                           << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, geist + " -- UNPRUEFBAR: Allowlist-Zeile ohne Gegenstand -- Feld 1 steht"
                                              " nicht im SOLL-Bestand"))
        << mit.ausgabe;
    // GANZE ZEILEN (Lens C LCT-10/LCT-11): "davon 1 begruendet" mit allen Feldern (die tragende Zeile darf
    // nicht mit rot werden), die Klassen-Zeile mit "1 ohne Gegenstand im SOLL-Bestand", die Endzeile mit
    // "0 erloschen" (die Frist der Geist-Zeile darf nicht bewertet worden sein) und "1 unpruefbar".
    EXPECT_TRUE(enthaelt(mit.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, nenner_ohne_bauweg(0, 0, 0, 1, 0))) << mit.ausgabe;
    EXPECT_TRUE(enthaelt(mit.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << mit.ausgabe;

    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend));
    Lauf const ohne = fall.fahren();
    berichten("AllowlistZeileOhneGegenstandImSollBestandIstUnpruefbar/ohne-Geist-Zeile", ohne, marke);
    EXPECT_EQ(ohne.code, 0) << "Ohne die Geist-Zeile muss die tragende Zeile allein gruen sein.\n" << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << ohne.ausgabe;
    EXPECT_TRUE(enthaelt(ohne.ausgabe, endzeile_ok(2, 0))) << ohne.ausgabe;

    // (c) Zeilen nur aus Leerraum (drei Leerzeichen; ein Tab) zwischen Kopf und tragender Zeile: Leerzeilen.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n   \n\t\n" + tragend));
    Lauf const leerraum = fall.fahren();
    berichten("AllowlistZeileOhneGegenstandImSollBestandIstUnpruefbar/Leerraum-Zeilen", leerraum, marke);
    EXPECT_EQ(leerraum.code, 0) << "Eine Zeile nur aus Leerraum ist eine Leerzeile, kein Befund.\n" << leerraum.ausgabe;
    EXPECT_TRUE(enthaelt(leerraum.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << leerraum.ausgabe;
    EXPECT_TRUE(enthaelt(leerraum.ausgabe, endzeile_ok(2, 0))) << leerraum.ausgabe;

    // (d) ein eingerueckter Kommentar ist eine Datenzeile ohne Gegenstand -- rot, mit der Ursache im Text.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n  # eingerueckt " + marke + "\n" + tragend));
    Lauf const eingerueckt = fall.fahren();
    berichten("AllowlistZeileOhneGegenstandImSollBestandIstUnpruefbar/eingerueckter-Kommentar", eingerueckt, marke);
    EXPECT_EQ(eingerueckt.code, 1) << "Ein eingerueckter Kommentar ist fuer die Wache eine Datenzeile -- ROT.\n"
                                   << eingerueckt.ausgabe;
    EXPECT_TRUE(enthaelt(eingerueckt.ausgabe, "# eingerueckt " + marke +
                                                  " -- UNPRUEFBAR: Allowlist-Zeile ohne Gegenstand -- Feld 1 beginnt"
                                                  " mit '#': ein EINGERUECKTER Kommentar?"))
        << "Die Meldung muss die naheliegende Ursache nennen.\n"
        << eingerueckt.ausgabe;
    EXPECT_TRUE(enthaelt(eingerueckt.ausgabe, nenner_ohne_bauweg(0, 0, 0, 1, 0))) << eingerueckt.ausgabe;
    EXPECT_TRUE(enthaelt(eingerueckt.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << eingerueckt.ausgabe;
}

// =============================================================================
// (14) DIE LETZTE ZEILE OHNE ZEILENUMBRUCH (Lens C LCW-04 = Lens A LA3-05, hochgestuft; 2026-09-18).
//      'read' liefert am Dateiende nach gelesenen Zeichen einen Status ungleich 0 -- die Schleife lief
//      dort ohne Rumpf, in BEIDEN Lesern der Wache. Eine Geist- oder ARCHIV-Zeile als letzte Zeile ohne
//      Zeilenumbruch blieb ungezaehlt (Exit 0), eine tragende Zeile dort blieb ungelesen (Exit 1, OHNE
//      BEGRUENDUNG). Beide Richtungen; beide gegen d8e8f53d falsch herum.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistLetzteZeileOhneZeilenumbruchWirdGelesen) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const geist    = "tests/unit/test_geist_" + marke + ".cpp";
    std::string const tragend  = fall.waise() + " | datei:" + lebendig + " | Koeder " + marke;

    // (a) die Geist-Zeile als LETZTE Zeile ohne Zeilenumbruch: muss gezaehlt werden -> ROT.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend + "\n" + geist +
                                         " | frist:2999-12-31 | Geist " + marke));
    Lauf const geist_ohne_lf = fall.fahren();
    berichten("AllowlistLetzteZeileOhneZeilenumbruchWirdGelesen/Geist-Zeile-ohne-LF", geist_ohne_lf, marke);
    EXPECT_EQ(geist_ohne_lf.code, 1) << "Die letzte Zeile ohne Zeilenumbruch ist eine Zeile -- der Nachscan muss"
                                        " sie sehen.\n"
                                     << geist_ohne_lf.ausgabe;
    EXPECT_TRUE(enthaelt(geist_ohne_lf.ausgabe, geist + " -- UNPRUEFBAR: Allowlist-Zeile ohne Gegenstand"))
        << geist_ohne_lf.ausgabe;
    EXPECT_TRUE(enthaelt(geist_ohne_lf.ausgabe, nenner_ohne_bauweg(0, 0, 0, 1, 0))) << geist_ohne_lf.ausgabe;
    EXPECT_TRUE(enthaelt(geist_ohne_lf.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << geist_ohne_lf.ausgabe;

    // (b) die TRAGENDE Zeile als letzte Zeile ohne Zeilenumbruch: muss gelesen werden -> GRUEN.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend));
    Lauf const tragend_ohne_lf = fall.fahren();
    berichten("AllowlistLetzteZeileOhneZeilenumbruchWirdGelesen/tragende-Zeile-ohne-LF", tragend_ohne_lf, marke);
    EXPECT_EQ(tragend_ohne_lf.code, 0) << "Die tragende Zeile ohne Zeilenumbruch muss tragen.\n"
                                       << tragend_ohne_lf.ausgabe;
    EXPECT_TRUE(enthaelt(tragend_ohne_lf.ausgabe, "Koeder " + marke))
        << "Die Wache muss GENAU diese Zeile gelesen haben.\n"
        << tragend_ohne_lf.ausgabe;
    EXPECT_TRUE(enthaelt(tragend_ohne_lf.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << tragend_ohne_lf.ausgabe;
    EXPECT_TRUE(enthaelt(tragend_ohne_lf.ausgabe, endzeile_ok(2, 0))) << tragend_ohne_lf.ausgabe;
}

// =============================================================================
// (15) EINE ALLOWLIST-ZEILE FUER EINEN PFAD UNTER tests/deprecated/ (Lens C LCW-05, 2026-09-18). Ohne
//      Anker steht die Datei im SOLL, und eine 'frist:'-Zeile truege sie regulaer als begruendet: das
//      Archiv waere durch die Hintertuer wieder eine Frist -- gegen den Geist der Owner-Order 206
//      ("frist Zeilen entfernen"). Gegen d8e8f53d war das GRUEN. Jetzt: die Zeile ist UNPRUEFBAR (ORT),
//      die Datei bleibt OHNE BEGRUENDUNG; mit Anker ist die Zeile weiter rot (ARCHIV-Klasse, Fall 10d);
//      gruen ist allein der Anker OHNE Zeile.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistZeileFuerPfadUnterDeprecatedIstUnpruefbar) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/ort_" + marke;
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("frist:2999-12-31"));

    // (a) ohne Anker, mit Zeile: die Zeile ist UNPRUEFBAR, die Datei OHNE BEGRUENDUNG -- nicht begruendet.
    Lauf const ohne_anker = fall.fahren();
    berichten("AllowlistZeileFuerPfadUnterDeprecatedIstUnpruefbar/ohne-Anker-mit-Zeile", ohne_anker, marke);
    EXPECT_EQ(ohne_anker.code, 1) << "Eine Allowlist-Zeile darf einen Archiv-Pfad nie tragen -- ROT.\n"
                                  << ohne_anker.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_anker.ausgabe,
                         fall.waise() + " -- UNPRUEFBAR: Allowlist-Zeile fuer einen Pfad unter tests/deprecated/"))
        << "Die Zeile muss namentlich und mit ihrer Klasse gemeldet werden.\n"
        << ohne_anker.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_anker.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << ohne_anker.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_anker.ausgabe, nenner_davon(0, 0, 0, 0, 1)))
        << "Die Frist darf die Datei NICHT begruendet haben.\n"
        << ohne_anker.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_anker.ausgabe, nenner_ohne_bauweg(0, 0, 1, 0, 0))) << ohne_anker.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_anker.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << ohne_anker.ausgabe;

    // (b) mit Anker, mit Zeile: die Zeile bleibt rot -- jetzt als ARCHIV-Zeile (Fall 10d), der Ort traegt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Anker " + marke + "\n"));
    Lauf const mit_anker = fall.fahren();
    berichten("AllowlistZeileFuerPfadUnterDeprecatedIstUnpruefbar/mit-Anker-mit-Zeile", mit_anker, marke);
    EXPECT_EQ(mit_anker.code, 1) << mit_anker.ausgabe;
    EXPECT_TRUE(enthaelt(mit_anker.ausgabe, nenner_ohne_bauweg(0, 1, 0, 0, 0))) << mit_anker.ausgabe;
    EXPECT_TRUE(enthaelt(mit_anker.ausgabe, endzeile_rot(0, 1, 0, 0, 1, 1))) << mit_anker.ausgabe;

    // (c) mit Anker, ohne Zeile: der einzige gruene Weg fuer einen Archiv-Pfad.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt", "# ohne Zeile " + marke + "\n"));
    Lauf const nur_anker = fall.fahren();
    berichten("AllowlistZeileFuerPfadUnterDeprecatedIstUnpruefbar/mit-Anker-ohne-Zeile", nur_anker, marke);
    EXPECT_EQ(nur_anker.code, 0) << "Der Anker allein muss tragen.\n" << nur_anker.ausgabe;
    EXPECT_TRUE(enthaelt(nur_anker.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << nur_anker.ausgabe;
    EXPECT_TRUE(enthaelt(nur_anker.ausgabe, endzeile_ok(1, 1))) << nur_anker.ausgabe;
}

// =============================================================================
// (16) DOPPELTE ALLOWLIST-ZEILEN JE PFAD (Lens C LCW-08, 2026-09-18). allow_zeile() nimmt die erste
//      Zeile; jede weitere schlief und erwachte allein durch die Reihenfolge -- eine abgelaufene oder
//      unpruefbare zweite Zeile blieb still (Lens A Probe X11: Exit 0). Jetzt: der Pfad wird als
//      DOPPELT gemeldet (einmal, mit Haeufigkeit), die erste Zeile traegt weiterhin sichtbar.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistDoppelteZeileJePfadIstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const erste    = fall.waise() + " | datei:" + lebendig + " | erste " + marke + "\n";
    std::string const zweite   = fall.waise() + " | datei:" + lebendig + " | zweite " + marke + "\n";
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + erste + zweite));

    Lauf const doppelt = fall.fahren();
    berichten("AllowlistDoppelteZeileJePfadIstUnpruefbar/zwei-Zeilen", doppelt, marke);
    EXPECT_EQ(doppelt.code, 1) << "Zwei Zeilen fuer einen Pfad: die zweite schlaeft -- ROT.\n" << doppelt.ausgabe;
    EXPECT_TRUE(enthaelt(doppelt.ausgabe, fall.waise() + " -- UNPRUEFBAR: DOPPELTE ALLOWLIST-ZEILE -- Feld 1 steht"
                                                         " 2-mal in der Allowlist"))
        << "Der Pfad muss namentlich, mit Klasse und Haeufigkeit gemeldet werden.\n"
        << doppelt.ausgabe;
    EXPECT_TRUE(enthaelt(doppelt.ausgabe, "erste " + marke)) << "Die erste Zeile muss weiter sichtbar tragen.\n"
                                                             << doppelt.ausgabe;
    EXPECT_FALSE(enthaelt(doppelt.ausgabe, "zweite " + marke)) << "Die zweite Zeile darf nie gelesen worden sein.\n"
                                                               << doppelt.ausgabe;
    EXPECT_TRUE(enthaelt(doppelt.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << doppelt.ausgabe;
    EXPECT_TRUE(enthaelt(doppelt.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 1))) << doppelt.ausgabe;
    EXPECT_TRUE(enthaelt(doppelt.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << doppelt.ausgabe;

    // GEGENRICHTUNG: dieselbe Allowlist mit genau EINER Zeile fuer den Pfad -> GRUEN.
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + erste));
    Lauf const einfach = fall.fahren();
    berichten("AllowlistDoppelteZeileJePfadIstUnpruefbar/eine-Zeile", einfach, marke);
    EXPECT_EQ(einfach.code, 0) << "Eine Zeile je Pfad muss tragen.\n" << einfach.ausgabe;
    EXPECT_TRUE(enthaelt(einfach.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0))) << einfach.ausgabe;
    EXPECT_TRUE(enthaelt(einfach.ausgabe, endzeile_ok(2, 0))) << einfach.ausgabe;
}

// =============================================================================
// (17) EIN LEERES FELD 3 (Lens C LCW-09, 2026-09-18). Der Drei-Feld-Vertrag der Allowlist verlangt die
//      Begruendung; gegen d8e8f53d lief eine Zeile mit leerem oder fehlendem dritten Feld als BEGRUENDET
//      durch (Exit 0). Jetzt: UNPRUEFBAR, in derselben Liste wie eine unbekannte Art. Gegenrichtung: mit
//      Text traegt dieselbe Zeile.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistLeeresFeld3IstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const klasse   = fall.waise() + " -- UNPRUEFBAR: Feld 3 (Begruendung) ist leer";

    // (a) drittes Feld vorhanden, aber leer.
    ASSERT_TRUE(
        fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                             "# Allowlist des Falls " + marke + "\n" + fall.waise() + " | datei:" + lebendig + " |\n"));
    Lauf const leer = fall.fahren();
    berichten("AllowlistLeeresFeld3IstUnpruefbar/Feld-3-leer", leer, marke);
    EXPECT_EQ(leer.code, 1) << "Eine Ausnahme ohne Begruendungstext ist keine -- ROT.\n" << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, klasse)) << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << leer.ausgabe;
    EXPECT_TRUE(enthaelt(leer.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << leer.ausgabe;

    // (b) nur zwei Felder, kein dritter Trenner.
    ASSERT_TRUE(
        fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                             "# Allowlist des Falls " + marke + "\n" + fall.waise() + " | datei:" + lebendig + "\n"));
    Lauf const zwei = fall.fahren();
    berichten("AllowlistLeeresFeld3IstUnpruefbar/nur-zwei-Felder", zwei, marke);
    EXPECT_EQ(zwei.code, 1) << "Zwei Felder sind kein Drei-Feld-Vertrag -- ROT.\n" << zwei.ausgabe;
    EXPECT_TRUE(enthaelt(zwei.ausgabe, klasse)) << zwei.ausgabe;
    EXPECT_TRUE(enthaelt(zwei.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << zwei.ausgabe;

    // (c) GEGENRICHTUNG: dieselbe Zeile mit Begruendungstext traegt.
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + lebendig));
    Lauf const mit_text = fall.fahren();
    berichten("AllowlistLeeresFeld3IstUnpruefbar/mit-Text", mit_text, marke);
    EXPECT_EQ(mit_text.code, 0) << "Mit Begruendungstext muss dieselbe Zeile tragen.\n" << mit_text.ausgabe;
    EXPECT_TRUE(enthaelt(mit_text.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << mit_text.ausgabe;
    EXPECT_TRUE(enthaelt(mit_text.ausgabe, endzeile_ok(2, 0))) << mit_text.ausgabe;
}

// =============================================================================
// (18) WERKZEUG-AUSFALL IN DER NENNER-PIPELINE IST EXIT 2 (Lens C LCW-02/LCW-03, 2026-09-18). POSIX-sh
//      kennt kein 'pipefail': in 'git ls-files -s | grep' zaehlte nur der Status von grep, und ein 'wc',
//      das scheitert, hinterliess leere Zaehler. Gegen d8e8f53d meldete die Wache in beiden Faellen
//      "OK ( Quelldateien, ...)" mit Exit 0 -- fail-open an der Stelle, die den Nenner erhebt. Aufbau:
//      ein PATH-Koeder gleichen Namens VOR dem echten Werkzeug; (a) 'git' liefert bei 'ls-files -s' die
//      erste Zeile und stirbt (Teilausgabe = der Anker), (b) 'wc' stirbt ohne Ausgabe. Beide: Exit 2 mit
//      ABBRUCH-Zeile, nie ein OK. (c) ohne Koeder wieder Exit 0.
// =============================================================================
TEST(Pa1ToteAusnahme, WerkzeugAusfallInDerNennerPipelineIstExit2) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/werkzeug_" + marke;
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(ordner + "/VERMERK.md", "# Anker " + marke + "\n"));
    Lauf const gesund = fall.fahren();
    berichten("WerkzeugAusfallInDerNennerPipelineIstExit2/ohne-Koeder", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: der gesunde Baum ist nicht gruen.\n" << gesund.ausgabe;
    ASSERT_TRUE(enthaelt(gesund.ausgabe, endzeile_ok(1, 1))) << gesund.ausgabe;

    Lauf const wo = im_repo(fall.repo(), "command -v git");
    ASSERT_EQ(wo.code, 0) << wo.ausgabe;
    std::string const echtes_git = wo.ausgabe;
    ASSERT_FALSE(echtes_git.empty());
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";

    // (a) 'git ls-files -s' mit Teilausgabe und Exit 1.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "git",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'git ls-files -s' liefert die erste Zeile und scheitert dann.\n"
                                       "if [ \"$1\" = \"ls-files\" ] && [ \"$2\" = \"-s\" ]; then\n"
                                       "    " +
                                       echtes_git + " \"$@\" | head -n 1\n    exit 1\nfi\nexec " + echtes_git +
                                       " \"$@\"\n"));
    Lauf const probe_git = im_repo(fall.repo(), pfad + " git ls-files -s");
    ASSERT_EQ(probe_git.code, 1) << "Arrangement: der git-Koeder scheitert nicht:\n" << probe_git.ausgabe;
    ASSERT_TRUE(enthaelt(probe_git.ausgabe, "VERMERK.md")) << "Arrangement: die Teilausgabe ist nicht der Anker:\n"
                                                           << probe_git.ausgabe;
    Lauf const teilausgabe = fall.fahren("", pfad);
    berichten("WerkzeugAusfallInDerNennerPipelineIstExit2/git-ls-files-s-Teilausgabe", teilausgabe, marke);
    EXPECT_EQ(teilausgabe.code, 2) << "Ein Werkzeug-Ausfall ist 'konnte nicht pruefen' -- Exit 2, nie gruen.\n"
                                   << teilausgabe.ausgabe;
    EXPECT_TRUE(enthaelt(teilausgabe.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'git ls-files -s'"))
        << "Der Abbruch muss das Werkzeug nennen.\n"
        << teilausgabe.ausgabe;
    EXPECT_FALSE(enthaelt(teilausgabe.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << teilausgabe.ausgabe;
    std::error_code ec;
    fs::remove(bin / "git", ec);
    ASSERT_FALSE(fs::exists(bin / "git"));

    // (b) 'wc' scheitert ohne Ausgabe -- der erste Zaehler der Wache.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "wc",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'wc' scheitert ohne Ausgabe.\n"
                                       "exit 1\n"));
    Lauf const probe_wc = im_repo(fall.repo(), pfad + " wc -l < /dev/null");
    ASSERT_EQ(probe_wc.code, 1) << "Arrangement: der wc-Koeder scheitert nicht:\n" << probe_wc.ausgabe;
    Lauf const zaehler = fall.fahren("", pfad);
    berichten("WerkzeugAusfallInDerNennerPipelineIstExit2/wc-Ausfall", zaehler, marke);
    EXPECT_EQ(zaehler.code, 2) << "Ein Zaehler ohne Zahl ist kein Nenner -- Exit 2, nie gruen.\n" << zaehler.ausgabe;
    EXPECT_TRUE(enthaelt(zaehler.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'wc -l'"))
        << "Der Abbruch muss das Werkzeug nennen.\n"
        << zaehler.ausgabe;
    EXPECT_FALSE(enthaelt(zaehler.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << zaehler.ausgabe;
    fs::remove(bin / "wc", ec);
    ASSERT_FALSE(fs::exists(bin / "wc"));

    // (c) GEGENRICHTUNG: ohne Koeder wieder gruen -- die Abbrueche stammten aus den Koedern.
    Lauf const wieder = fall.fahren("", pfad);
    berichten("WerkzeugAusfallInDerNennerPipelineIstExit2/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(enthaelt(wieder.ausgabe, endzeile_ok(1, 1))) << wieder.ausgabe;
}

// =============================================================================
// (19) LEERE ISA-TEILMERKMALE SIND UNPRUEFBAR (Lens C LC3W-07, Fix-r3). Bis 806629ca wurde ein leeres
//      Teilmerkmal still uebersprungen: 'isa:+avx512f' galt wie 'isa:avx512f' (Exit 0). Die Form ist
//      isa:<merkmal>[+<merkmal>] -- kein '+' am Rand, kein '++'. Geprueft wird die FORM vor dem Cache:
//      der Nenner sagt "ISA-Gegenprobe: nicht gefragt". Gegenrichtung (a): dieselbe Zeile wohlgeformt
//      traegt, weil avx512f dem Bau-Host des Falls fehlt (kIsaCacheAvx2DaAvx512fFehlt).
// =============================================================================
TEST(Pa1ToteAusnahme, IsaLeeresTeilmerkmalIstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.isa_cache_schreiben(kIsaCacheAvx2DaAvx512fFehlt));

    // (a) Arrangement und Gegenrichtung: wohlgeformt, avx512f fehlt -> begruendet.
    ASSERT_TRUE(fall.allowlist_setzen("isa:avx512f"));
    Lauf const gesund = fall.fahren();
    berichten("IsaLeeresTeilmerkmalIstUnpruefbar/wohlgeformt", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: 'isa:avx512f' traegt nicht.\n" << gesund.ausgabe;
    EXPECT_TRUE(enthaelt(gesund.ausgabe, "avx512f=nein")) << gesund.ausgabe;
    EXPECT_TRUE(enthaelt(gesund.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << gesund.ausgabe;
    EXPECT_TRUE(enthaelt(gesund.ausgabe, endzeile_ok(2, 0))) << gesund.ausgabe;

    // (b)-(d) ein leeres Teilmerkmal am Anfang, am Ende, in der Mitte.
    for (char const* const merkmal : {"+avx512f", "avx512f+", "avx2++avx512f"}) {
        ASSERT_TRUE(fall.allowlist_setzen(std::string{"isa:"} + merkmal));
        Lauf const leer = fall.fahren();
        berichten((std::string{"IsaLeeresTeilmerkmalIstUnpruefbar/isa:"} + merkmal).c_str(), leer, marke);
        EXPECT_EQ(leer.code, 1) << "'isa:" << merkmal << "' hat ein leeres Teilmerkmal -- ROT.\n" << leer.ausgabe;
        EXPECT_TRUE(
            enthaelt(leer.ausgabe, fall.waise() + " -- UNPRUEFBAR: \"isa:" + merkmal + "\" hat ein leeres Teilmerkmal"))
            << leer.ausgabe;
        EXPECT_TRUE(enthaelt(leer.ausgabe, "ISA-Gegenprobe: nicht gefragt"))
            << "Die Form wird VOR dem Cache geprueft -- der Cache darf nicht gelesen worden sein.\n"
            << leer.ausgabe;
        EXPECT_TRUE(enthaelt(leer.ausgabe, nenner_davon(0, 0, 0, 1, 0))) << leer.ausgabe;
        EXPECT_TRUE(enthaelt(leer.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << leer.ausgabe;
    }
}

// =============================================================================
// (20) ISA-BELEGE MUESSEN EINDEUTIG SEIN (Lens C LC3W-08, Fix-r3). Die Wertzeile musste schon GENAU
//      EINMAL im Cache stehen; _COMPILED und _EXITCODE nicht -- eine zweite Zeile schlief oder gewann
//      je nach Reihenfolge (gegen 806629ca je Exit 0). Und bei leerem Wert ('nein') liess ein LEERER
//      _EXITCODE durch, obwohl try_run beide Zeilen gemeinsam schreibt. Jede Stufe ist Exit 2 (die
//      ISA-Frage ist unbeantwortbar), nie ein Befund; Gegenrichtung: der ehrliche Cache traegt.
// =============================================================================
TEST(Pa1ToteAusnahme, IsaBelegeMuessenEindeutigSein) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("isa:avx512f"));
    std::string const gesund_cache{kIsaCacheAvx2DaAvx512fFehlt};

    ASSERT_TRUE(fall.isa_cache_schreiben(gesund_cache));
    Lauf const gesund = fall.fahren();
    berichten("IsaBelegeMuessenEindeutigSein/ehrlicher-Cache", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: der ehrliche Cache traegt nicht.\n" << gesund.ausgabe;

    struct Stufe {
        char const* name;
        std::string cache;
        char const* meldung;
    };
    std::vector<Stufe> const stufen{
        {"_COMPILED-doppelt", gesund_cache + "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=FALSE\n",
         "'COMDARE_HOST_RUNS_AVX512F_COMPILED' steht 2-mal in"},
        {"_EXITCODE-doppelt", gesund_cache + "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=0\n",
         "'COMDARE_HOST_RUNS_AVX512F_EXITCODE' steht 2-mal in"},
        {"nein-mit-leerem-_EXITCODE",
         "COMDARE_HOST_RUNS_AVX512F:INTERNAL=\n"
         "COMDARE_HOST_RUNS_AVX512F_COMPILED:INTERNAL=TRUE\n"
         "COMDARE_HOST_RUNS_AVX512F_EXITCODE:INTERNAL=\n",
         "'COMDARE_HOST_RUNS_AVX512F' ist leer und 'COMDARE_HOST_RUNS_AVX512F_EXITCODE' ist es auch"},
    };
    for (auto const& s : stufen) {
        ASSERT_TRUE(fall.isa_cache_schreiben(s.cache));
        Lauf const lauf = fall.fahren();
        berichten((std::string{"IsaBelegeMuessenEindeutigSein/"} + s.name).c_str(), lauf, marke);
        EXPECT_EQ(lauf.code, 2) << s.name << ": ein nicht eindeutiger Beleg ist UNBEANTWORTBAR -- Exit 2.\n"
                                << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, "ABBRUCH: ")) << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, s.meldung)) << s.name << ": die Meldung muss den Beleg nennen.\n"
                                                       << lauf.ausgabe;
        EXPECT_FALSE(enthaelt(lauf.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << lauf.ausgabe;
    }

    // Gegenrichtung: der ehrliche Cache traegt wieder.
    ASSERT_TRUE(fall.isa_cache_schreiben(gesund_cache));
    Lauf const wieder = fall.fahren();
    berichten("IsaBelegeMuessenEindeutigSein/ehrlicher-Cache-wieder", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(enthaelt(wieder.ausgabe, endzeile_ok(2, 0))) << wieder.ausgabe;
}

// =============================================================================
// (21) WERKZEUG-AUSFALL IM ISA-PFAD IST EXIT 2 (Lens C LC3W-01, Fix-r3). Die Fassung 806629ca zaehlte die
//      Cache-Zeilen mit 'sed | wc -l | tr': ein Ausfall vor dem letzten Glied war unsichtbar, und ein
//      ausgefallenes wc hinterliess einen leeren Zaehler, den jedes '-eq 0' still uebersprang -- Exit 0
//      (Shell-Probe im Beweisort FIX-r3.md, stdin-lesender wc-Koeder). Jetzt laeuft der ISA-Teil ueber
//      grep_in_datei und zeilen_zaehlen; zeilen_zaehlen reicht den DATEINAMEN an wc, deshalb kann ein
//      PATH-Koeder gezielt an der ISA-Zwischendatei scheitern. Gegenrichtung: ohne Koeder Exit 0.
// =============================================================================
TEST(Pa1ToteAusnahme, WerkzeugAusfallImIsaPfadIstExit2) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.isa_cache_schreiben(kIsaCacheAvx2DaAvx512fFehlt));
    ASSERT_TRUE(fall.allowlist_setzen("isa:avx512f"));
    Lauf const gesund = fall.fahren();
    berichten("WerkzeugAusfallImIsaPfadIstExit2/ohne-Koeder", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: der gesunde Baum ist nicht gruen.\n" << gesund.ausgabe;

    Lauf const wo = im_repo(fall.repo(), "command -v wc");
    ASSERT_EQ(wo.code, 0) << wo.ausgabe;
    std::string const echtes_wc = wo.ausgabe;
    ASSERT_FALSE(echtes_wc.empty());
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";
    // Der Koeder scheitert NUR, wenn seine Argumente die ISA-Zwischendatei nennen; alle anderen
    // Zaehlungen der Wache laufen durch das echte wc.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "wc",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'wc' scheitert an der ISA-Zwischendatei, sonst echtes wc.\n"
                                       "case \"$*\" in *isa_*) exit 1 ;; esac\nexec " +
                                       echtes_wc + " \"$@\"\n"));
    ASSERT_TRUE(fall.repo().schreibe("isa_wert_probe.txt", "x\n"));
    Lauf const probe_isa = im_repo(fall.repo(), pfad + " wc -l isa_wert_probe.txt");
    ASSERT_EQ(probe_isa.code, 1) << "Arrangement: der wc-Koeder scheitert nicht an 'isa_':\n" << probe_isa.ausgabe;
    Lauf const probe_echt = im_repo(fall.repo(), "wc -l isa_wert_probe.txt");
    ASSERT_EQ(probe_echt.code, 0) << "Arrangement: das echte wc zaehlt die Probedatei nicht:\n" << probe_echt.ausgabe;
    Lauf const probe_rest = im_repo(fall.repo(), pfad + " wc -l /dev/null");
    ASSERT_EQ(probe_rest.code, 0) << "Arrangement: der wc-Koeder reicht andere Aufrufe nicht durch:\n"
                                  << probe_rest.ausgabe;

    Lauf const ausfall = fall.fahren("", pfad);
    berichten("WerkzeugAusfallImIsaPfadIstExit2/wc-Koeder-an-isa_wert", ausfall, marke);
    EXPECT_EQ(ausfall.code, 2) << "Ein Werkzeug-Ausfall im ISA-Pfad ist 'konnte nicht pruefen' -- Exit 2.\n"
                               << ausfall.ausgabe;
    EXPECT_TRUE(enthaelt(ausfall.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'wc -l' ueber")) << ausfall.ausgabe;
    EXPECT_TRUE(enthaelt(ausfall.ausgabe, "isa_wert.txt")) << "Der Abbruch muss die ISA-Zwischendatei nennen.\n"
                                                           << ausfall.ausgabe;
    EXPECT_FALSE(enthaelt(ausfall.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ausfall.ausgabe;

    std::error_code ec;
    fs::remove(bin / "wc", ec);
    ASSERT_FALSE(fs::exists(bin / "wc"));
    Lauf const wieder = fall.fahren("", pfad);
    berichten("WerkzeugAusfallImIsaPfadIstExit2/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(enthaelt(wieder.ausgabe, endzeile_ok(2, 0))) << wieder.ausgabe;
}

// =============================================================================
// (22) GREP-STATUS 2 IST EXIT 2, KEIN NICHTTREFFER (Lens C LC3W-02, Fix-r3). 'if ! grep -q' nahm einen
//      Status 2 (Datei unlesbar) als "nicht gefunden": an der Gegenprobe wurde daraus "steht NICHT in
//      compile_commands.json" (Exit 2 mit falscher Diagnose), an der Zuordnung im Nachscan ein
//      Datenbefund. Jetzt laeuft jeder grep durch grep_in_datei: 0/1 sind Antworten, ab 2 ist ein
//      Werkzeug-Ausfall. Arrangement: die IST-Datei des Bau-Baums wird unlesbar gemacht -- per chmod 000;
//      liest sie der Nutzer trotzdem (root, CAP_DAC_OVERRIDE), ersatzweise als Symlink auf /proc/self/mem
//      (eine regulaere Datei, deren Lesen mit EIO scheitert); ist auch das nicht herstellbar, wird der
//      Fall LAUT uebersprungen, nie still gruen. Gegenrichtung: wieder lesbar -> Exit 0.
// =============================================================================
TEST(Pa1ToteAusnahme, GrepStatusZweiIstExit2) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("datei:tests/unit/kommt_vielleicht_" + marke + ".hpp"));
    Lauf const gesund = fall.fahren();
    berichten("GrepStatusZweiIstExit2/lesbar", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << gesund.ausgabe;

    fs::path const  ist = fall.baum() / "compile_commands.json";
    std::error_code ec;
    fs::permissions(ist, fs::perms::none, fs::perm_options::replace, ec);
    ASSERT_FALSE(ec) << "chmod 000 fehlgeschlagen: " << ec.message();
    bool procfs = false;
    Lauf lesbar = fahre("cat " + zitiert(ist) + " > /dev/null");
    if (lesbar.code == 0) {
        // root liest trotz chmod 000: zweite Herstellung ueber procfs.
        fs::permissions(ist, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace, ec);
        fs::remove(ist, ec);
        fs::create_symlink("/proc/self/mem", ist, ec);
        procfs = true;
        lesbar = fahre("cat " + zitiert(ist) + " > /dev/null");
        if (ec || lesbar.code == 0 || !fs::is_regular_file(ist)) {
            GTEST_SKIP() << "Die IST-Datei laesst sich auf diesem Host nicht unlesbar machen (root ohne "
                            "procfs?) -- ohne dieses Arrangement waere die Stufe kein Beweis.";
        }
    }

    Lauf const ausfall = fall.fahren();
    berichten(procfs ? "GrepStatusZweiIstExit2/IST-Datei-EIO" : "GrepStatusZweiIstExit2/IST-Datei-chmod-000", ausfall,
              marke);
    EXPECT_EQ(ausfall.code, 2) << "Eine unlesbare IST-Datei ist ein Werkzeug-Ausfall -- Exit 2.\n" << ausfall.ausgabe;
    EXPECT_TRUE(enthaelt(ausfall.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'grep' Gegenprobe")) << ausfall.ausgabe;
    EXPECT_FALSE(enthaelt(ausfall.ausgabe, "steht NICHT in"))
        << "Die alte Fehldiagnose (Nichttreffer statt Werkzeug-Ausfall) darf nicht mehr erscheinen.\n"
        << ausfall.ausgabe;
    EXPECT_FALSE(enthaelt(ausfall.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ausfall.ausgabe;

    // Gegenrichtung: wieder lesbar.
    if (procfs) {
        fs::remove(ist, ec);
        ASSERT_TRUE(fall.bauweg_schreiben({kGegenprobe}));
    } else {
        fs::permissions(ist,
                        fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read | fs::perms::others_read,
                        fs::perm_options::replace, ec);
        ASSERT_FALSE(ec) << ec.message();
    }
    Lauf const wieder = fall.fahren();
    berichten("GrepStatusZweiIstExit2/wieder-lesbar", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(enthaelt(wieder.ausgabe, endzeile_ok(2, 0))) << wieder.ausgabe;
}

// =============================================================================
// (23) GIT-FEHLER IN DER ERREICHBARKEITS-PROBE SIND EXIT 2 (Lens C LC3W-03, Fix-r3). 'git check-ignore -q'
//      kennt drei Antworten: 0 ignoriert, 1 nicht ignoriert, 128 git scheitert. Die Fassung 806629ca
//      nahm 128 als "nicht ignoriert" und erklaerte ein Bauprodukt unter .gitignore fuer TOT (Exit 1,
//      ein Datenurteil aus einem Werkzeugfehler); ein 'git ls-files' mit 128 galt als "kein Inhalt"
//      (Shell-Proben im Beweisort FIX-r3.md). Aufbau wie Fall (3), dazu PATH-Koeder fuer git, die genau
//      EINEN Unterbefehl mit 128 beenden. Gegenrichtung: ohne Koeder Exit 0.
// =============================================================================
TEST(Pa1ToteAusnahme, GitFehlerInDerErreichbarkeitsProbeIstExit2) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(".gitignore", "erzeugt_" + marke + "/\n"));
    std::string const bauprodukt = "erzeugt_" + marke + "/artefakt.hpp";
    ASSERT_TRUE(fall.allowlist_setzen("datei:" + bauprodukt));
    Lauf const gesund = fall.fahren();
    berichten("GitFehlerInDerErreichbarkeitsProbeIstExit2/ohne-Koeder", gesund, marke);
    ASSERT_EQ(gesund.code, 0) << "Das Arrangement ist falsch: das Bauprodukt ist nicht erreichbar.\n" << gesund.ausgabe;

    Lauf const wo = im_repo(fall.repo(), "command -v git");
    ASSERT_EQ(wo.code, 0) << wo.ausgabe;
    std::string const echtes_git = wo.ausgabe;
    ASSERT_FALSE(echtes_git.empty());
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";
    std::error_code   ec;

    // (a) 'git check-ignore' endet mit 128.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "git",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'check-ignore' scheitert mit 128.\n"
                                       "if [ \"$1\" = check-ignore ]; then echo 'fatal: Koeder' >&2; exit 128; fi\n"
                                       "exec " +
                                       echtes_git + " \"$@\"\n"));
    Lauf const probe_ci = im_repo(fall.repo(), pfad + " git check-ignore -q -- x");
    ASSERT_EQ(probe_ci.code, 128) << "Arrangement: der git-Koeder liefert keine 128:\n" << probe_ci.ausgabe;
    Lauf const ci = fall.fahren("", pfad);
    berichten("GitFehlerInDerErreichbarkeitsProbeIstExit2/check-ignore-128", ci, marke);
    EXPECT_EQ(ci.code, 2) << "Ein git-Fehler ist keine Antwort auf 'ignoriert?' -- Exit 2.\n" << ci.ausgabe;
    EXPECT_TRUE(enthaelt(ci.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'git check-ignore' fuer " + bauprodukt))
        << ci.ausgabe;
    EXPECT_FALSE(enthaelt(ci.ausgabe, "TOTE AUSNAHME -- der Gegenstand"))
        << "Aus einem Werkzeugfehler darf kein Datenurteil werden.\n"
        << ci.ausgabe;
    EXPECT_FALSE(enthaelt(ci.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ci.ausgabe;
    fs::remove(bin / "git", ec);
    ASSERT_FALSE(fs::exists(bin / "git"));

    // (b) 'git ls-files -- <pfad>' (die Index-Frage der Probe) endet mit 128; 'ls-files' ohne '--' (SOLL)
    //     und 'ls-files -s' (Anker) laufen echt.
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "git",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'ls-files --' scheitert mit 128.\n"
                                       "if [ \"$1\" = ls-files ] && [ \"$2\" = -- ]; then echo 'fatal: Koeder' >&2; "
                                       "exit 128; fi\nexec " +
                                       echtes_git + " \"$@\"\n"));
    Lauf const probe_lf = im_repo(fall.repo(), pfad + " git ls-files -- x");
    ASSERT_EQ(probe_lf.code, 128) << probe_lf.ausgabe;
    Lauf const probe_soll = im_repo(fall.repo(), pfad + " git ls-files");
    ASSERT_EQ(probe_soll.code, 0) << "Arrangement: der Koeder trifft auch das SOLL:\n" << probe_soll.ausgabe;
    Lauf const lf = fall.fahren("", pfad);
    berichten("GitFehlerInDerErreichbarkeitsProbeIstExit2/ls-files-128", lf, marke);
    EXPECT_EQ(lf.code, 2) << lf.ausgabe;
    EXPECT_TRUE(enthaelt(lf.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'git ls-files' fuer " + bauprodukt)) << lf.ausgabe;
    EXPECT_FALSE(enthaelt(lf.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << lf.ausgabe;
    fs::remove(bin / "git", ec);
    ASSERT_FALSE(fs::exists(bin / "git"));

    // (c) Gegenrichtung.
    Lauf const wieder = fall.fahren("", pfad);
    berichten("GitFehlerInDerErreichbarkeitsProbeIstExit2/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(enthaelt(wieder.ausgabe, endzeile_ok(2, 0))) << wieder.ausgabe;
}

// =============================================================================
// (24) DER ANKER MUSS EIN BLOB IN DER OBJEKTDATENBANK SEIN (Lens C LC3W-06 / LC3W-03, Fix-r3). Der
//      Index-Modus 100644 verspricht ein Blob, prueft es aber nicht: 'git update-index --cacheinfo' legt
//      jedes Objekt unter jedem Modus ab. Ein TREE unter 100644 ankerte gegen 806629ca (Exit 0, sein
//      'cat-file -p' hat Nicht-Leerraum-Zeichen). Jetzt: 'cat-file -e' (Objekt da? 1 = fehlt -> UNPRUEFBAR,
//      ein Datenbefund) und 'cat-file -t' == blob (sonst UNPRUEFBAR); scheitert git selbst, ist es Exit 2.
//      Stufen: (a) Tree, (b) Fantasie-SHA, (c) echtes Blob (Gegenrichtung), (d) git-Koeder 'cat-file -t' 128.
// =============================================================================
TEST(Pa1ToteAusnahme, ArchivAnkerMussBlobInDerObjektdatenbankSein) {
    std::string const marke  = koeder();
    std::string const ordner = "tests/deprecated/objekt_" + marke;
    std::string const anker  = ordner + "/VERMERK.md";
    Fall              fall{marke, ordner + "/test_waise_" + marke + ".cpp"};
    ASSERT_TRUE(fall.init());

    // (a) ein Tree-Objekt unter 100644.
    Lauf const tree = im_repo(fall.repo(), "git write-tree");
    ASSERT_EQ(tree.code, 0) << tree.ausgabe;
    ASSERT_EQ(tree.ausgabe.size(), 40U) << "kein SHA-1: '" << tree.ausgabe << "'";
    Lauf const typ = im_repo(fall.repo(), "git cat-file -t " + tree.ausgabe);
    ASSERT_EQ(typ.ausgabe, "tree") << "Arrangement: das Objekt ist kein Tree:\n" << typ.ausgabe;
    Lauf const cacheinfo =
        im_repo(fall.repo(), "git update-index --add --cacheinfo 100644," + tree.ausgabe + "," + anker);
    ASSERT_EQ(cacheinfo.code, 0) << cacheinfo.ausgabe;
    Lauf const eintrag = im_repo(fall.repo(), "git ls-files -s -- " + zitiert(anker));
    ASSERT_TRUE(enthaelt(eintrag.ausgabe, "100644 " + tree.ausgabe + " 0\t" + anker)) << "Arrangement:\n"
                                                                                      << eintrag.ausgabe;
    Lauf const als_tree = fall.fahren();
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/Tree-unter-100644", als_tree, marke);
    EXPECT_EQ(als_tree.code, 1) << "Ein Tree ist kein Anker -- die Datei bleibt im SOLL, ROT.\n" << als_tree.ausgabe;
    EXPECT_TRUE(enthaelt(als_tree.ausgabe, anker + " -- UNPRUEFBARER ANKER: Objekttyp tree ist kein Blob (Index-Modus"
                                                   " 100644 verspricht eines)"))
        << als_tree.ausgabe;
    EXPECT_TRUE(enthaelt(als_tree.ausgabe, "OHNE BEGRUENDUNG AUSSERHALB DES BAUWEGS")) << als_tree.ausgabe;
    EXPECT_TRUE(enthaelt(als_tree.ausgabe, fall.waise())) << als_tree.ausgabe;
    EXPECT_TRUE(enthaelt(als_tree.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << als_tree.ausgabe;
    EXPECT_TRUE(enthaelt(als_tree.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << als_tree.ausgabe;

    // (b) ein SHA ohne Objekt.
    std::string const fantasie = "0123456789012345678901234567890123456789";
    Lauf const cacheinfo2 = im_repo(fall.repo(), "git update-index --add --cacheinfo 100644," + fantasie + "," + anker);
    ASSERT_EQ(cacheinfo2.code, 0) << cacheinfo2.ausgabe;
    Lauf const fehlt_probe = im_repo(fall.repo(), "git cat-file -e " + fantasie);
    ASSERT_EQ(fehlt_probe.code, 1) << "Arrangement: das Objekt existiert doch:\n" << fehlt_probe.ausgabe;
    Lauf const ohne_objekt = fall.fahren();
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/Objekt-fehlt", ohne_objekt, marke);
    EXPECT_EQ(ohne_objekt.code, 1) << ohne_objekt.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_objekt.ausgabe,
                         anker + " -- UNPRUEFBARER ANKER: Blob " + fantasie + " fehlt in der Objektdatenbank"))
        << ohne_objekt.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_objekt.ausgabe, nenner_ohne_bauweg(1, 0, 0, 0, 0))) << ohne_objekt.ausgabe;
    EXPECT_TRUE(enthaelt(ohne_objekt.ausgabe, endzeile_rot(1, 2, 0, 0, 1, 0))) << ohne_objekt.ausgabe;

    // (c) Gegenrichtung: ein echtes Blob traegt.
    ASSERT_TRUE(fall.repo().schreibe_und_verfolge(anker, "# Anker mit Inhalt " + marke + "\n"));
    Lauf const blob = fall.fahren();
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/echtes-Blob", blob, marke);
    EXPECT_EQ(blob.code, 0) << blob.ausgabe;
    EXPECT_TRUE(enthaelt(blob.ausgabe, endzeile_ok(1, 1))) << blob.ausgabe;

    // (d) git selbst scheitert an 'cat-file -t': Exit 2, kein Befund.
    Lauf const wo = im_repo(fall.repo(), "command -v git");
    ASSERT_EQ(wo.code, 0) << wo.ausgabe;
    Lauf const blob_sha = im_repo(fall.repo(), "git hash-object -- " + zitiert(anker));
    ASSERT_EQ(blob_sha.ausgabe.size(), 40U) << blob_sha.ausgabe;
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "git",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke +
                                       ": 'cat-file -t' scheitert mit 128.\n"
                                       "if [ \"$1\" = cat-file ] && [ \"$2\" = -t ]; then echo 'fatal: Koeder' >&2; "
                                       "exit 128; fi\nexec " +
                                       wo.ausgabe + " \"$@\"\n"));
    Lauf const probe = im_repo(fall.repo(), pfad + " git cat-file -t " + blob_sha.ausgabe);
    ASSERT_EQ(probe.code, 128) << "Arrangement: der git-Koeder liefert keine 128:\n" << probe.ausgabe;
    Lauf const probe_echt = im_repo(fall.repo(), "git cat-file -t " + blob_sha.ausgabe);
    ASSERT_EQ(probe_echt.ausgabe, "blob") << "Arrangement: das echte git kennt das Blob nicht:\n" << probe_echt.ausgabe;
    Lauf const ausfall = fall.fahren("", pfad);
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/cat-file-t-128", ausfall, marke);
    EXPECT_EQ(ausfall.code, 2) << ausfall.ausgabe;
    EXPECT_TRUE(enthaelt(ausfall.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'git cat-file -t' fuer den Anker " + anker))
        << ausfall.ausgabe;
    EXPECT_FALSE(enthaelt(ausfall.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ausfall.ausgabe;
    std::error_code ec;
    fs::remove(bin / "git", ec);
    ASSERT_FALSE(fs::exists(bin / "git"));
    Lauf const wieder = fall.fahren("", pfad);
    berichten("ArchivAnkerMussBlobInDerObjektdatenbankSein/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(enthaelt(wieder.ausgabe, endzeile_ok(1, 1))) << wieder.ausgabe;
}

// =============================================================================
// (25) EIN AUSFALL VON 'date' IST EXIT 2 (Lens C LC3W-04, Fix-r3). HEUTE=$(date ...) ohne Statuspruefung
//      endete unter 'set -e' mit dem Rohstatus von date -- Exit 1, von einem Befund nicht zu unterscheiden
//      (Shell-Probe im Beweisort FIX-r3.md). Jetzt: werkzeug_abbruch, Exit 2. Mit COMDARE_WACHE_HEUTE
//      wird date nicht gefragt -- derselbe Koeder ist dann wirkungslos (Gegenrichtung 1); ohne Koeder
//      Exit 0 (Gegenrichtung 2). Die uebrigen direkten Werkzeuge der Fassung 806629ca (sed fuer die
//      eingerueckte Ausgabe und die Ordnernamen, cut/sed/tr fuer die Felder) sind aus der Wache entfernt
//      -- ohne Werkzeug kein Ausfall (Kopf der Wache, Folge (6)).
// =============================================================================
TEST(Pa1ToteAusnahme, DateAusfallIstExit2) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    ASSERT_TRUE(fall.allowlist_setzen("datei:tests/unit/kommt_vielleicht_" + marke + ".hpp"));
    fs::path const    bin  = fall.repo().pfad() / "koeder_bin";
    std::string const pfad = "PATH=\"" + bin.string() + ":$PATH\"";
    ASSERT_TRUE(koeder_bin_anlegen(fall.repo(), "date",
                                   "#!/bin/sh\n# PATH-Koeder des Falls " + marke + ": 'date' scheitert.\nexit 1\n"));
    Lauf const probe = im_repo(fall.repo(), pfad + " date +%Y-%m-%d");
    ASSERT_EQ(probe.code, 1) << "Arrangement: der date-Koeder scheitert nicht:\n" << probe.ausgabe;

    Lauf const ausfall = fall.fahren("", pfad);
    berichten("DateAusfallIstExit2/date-Koeder", ausfall, marke);
    EXPECT_EQ(ausfall.code, 2) << "Ohne belastbares Heute kann die Wache nicht pruefen -- Exit 2, nie Rohstatus.\n"
                               << ausfall.ausgabe;
    EXPECT_TRUE(enthaelt(ausfall.ausgabe, "ABBRUCH: Werkzeug-Ausfall -- 'date +%Y-%m-%d'")) << ausfall.ausgabe;
    EXPECT_FALSE(enthaelt(ausfall.ausgabe, "TEST-REGISTRIERUNGS-WACHE: OK")) << ausfall.ausgabe;

    // Gegenrichtung 1: das Heute vorgegeben -> date wird nicht gefragt, der Koeder bleibt wirkungslos.
    Lauf const vorgegeben = fall.fahren("2026-09-18", pfad);
    berichten("DateAusfallIstExit2/Heute-vorgegeben", vorgegeben, marke);
    EXPECT_EQ(vorgegeben.code, 0) << vorgegeben.ausgabe;
    EXPECT_TRUE(enthaelt(vorgegeben.ausgabe, "2026-09-18 -- Herkunft: COMDARE_WACHE_HEUTE (ueberschrieben)"))
        << vorgegeben.ausgabe;

    // Gegenrichtung 2: ohne Koeder.
    std::error_code ec;
    fs::remove(bin / "date", ec);
    ASSERT_FALSE(fs::exists(bin / "date"));
    Lauf const wieder = fall.fahren("", pfad);
    berichten("DateAusfallIstExit2/Koeder-entfernt", wieder, marke);
    EXPECT_EQ(wieder.code, 0) << wieder.ausgabe;
    EXPECT_TRUE(enthaelt(wieder.ausgabe, "Herkunft: Systemuhr")) << wieder.ausgabe;
}

// =============================================================================
// (26) DIE FORM GILT AUCH FUER DIE STUMME ZEILE EINER DATEI IM BAUWEG (Lens C LC3W-05, Fix-r3). Die
//      Schleife der Wache prueft Feld 2 und 3 nur an Zeilen, die sie auswertet (Dateien, die dem Bauweg
//      fehlen). Eine Zeile fuer eine Datei IM Bauweg ist stumm -- das ist Design (Lens A LA3-08: die
//      'isa:'-Zeile ist auf dem AVX-512-Host genau so stumm) -- aber ein Formfehler darin lag in Reserve:
//      gegen 806629ca war jede der Stufen (a)-(d) Exit 0 (Shell-Proben im Beweisort FIX-r3.md; die
//      Lead-Triage hatte den Fund als widerlegt gefuehrt, die Test-Auflage widerlegte die Widerlegung).
//      Jetzt prueft der Nachscan die Form ALLER Datenzeilen mit demselben Helfer wie die Schleife und
//      zaehlt Formfehler stummer Zeilen als eigene Klasse (sechstes Nenner-Feld). Gegenstand: die
//      Gegenprobe-Datei des Falls (sie steht immer im Bauweg); die Waise traegt daneben eine gueltige
//      Zeile und muss begruendet bleiben. Die ISA-Stufe hat KEINEN CMakeCache.txt und ist trotzdem Exit 1,
//      nicht Exit 2: Form vor Cache. (e) Gegenrichtung: eine WOHLGEFORMTE stumme Zeile bleibt stumm --
//      Exit 0, ihr Feld 3 erscheint nie in der Ausgabe.
// =============================================================================
TEST(Pa1ToteAusnahme, AllowlistFormfehlerFuerDateiImBauwegIstUnpruefbar) {
    std::string const marke = koeder();
    Fall              fall{marke};
    ASSERT_TRUE(fall.init());
    std::string const lebendig = "tests/unit/kommt_vielleicht_" + marke + ".hpp";
    std::string const tragend  = fall.waise() + " | datei:" + lebendig + " | Koeder " + marke + "\n";
    std::string const gebaut   = kGegenprobe; // steht im Bauweg des Falls

    struct Stufe {
        char const* name;
        std::string zeile;
        std::string meldung;
    };
    std::vector<Stufe> const rot{
        {"Feld-3-leer", gebaut + " | datei:tests/unit/x_" + marke + ".hpp |\n",
         gebaut + " -- UNPRUEFBAR: Feld 3 (Begruendung) ist leer"},
        {"Art-unbekannt", gebaut + " | foo:x | Text " + marke + "\n",
         gebaut + " -- UNPRUEFBAR: Feld 2 ist \"foo:x\" -- keine bekannte Art"},
        {"ISA-Merkmal-unbekannt", gebaut + " | isa:sse9 | Text " + marke + "\n",
         gebaut + " -- UNPRUEFBAR: unbekannte(s) ISA-Merkmal(e): sse9 (bekannt: avx2, avx512f)"},
        {"kein-Datum", gebaut + " | frist:gestern | Text " + marke + "\n",
         gebaut + " -- UNPRUEFBAR: \"frist:gestern\" ist kein Datum JJJJ-MM-TT"},
    };
    for (auto const& s : rot) {
        ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                         "# Allowlist des Falls " + marke + "\n" + tragend + s.zeile));
        Lauf const lauf = fall.fahren();
        berichten((std::string{"AllowlistFormfehlerFuerDateiImBauwegIstUnpruefbar/"} + s.name).c_str(), lauf, marke);
        EXPECT_EQ(lauf.code, 1) << s.name << ": ein Formfehler in einer stummen Zeile ist ein Freibrief in Reserve"
                                << " -- ROT.\n"
                                << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, s.meldung)) << s.name << "\n" << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, "die Zeile gilt einer Datei IM Bauweg und ist stumm")) << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, nenner_davon(1, 0, 0, 0, 0)))
            << "Die tragende Zeile der Waise darf nicht mit rot werden.\n"
            << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0, 1))) << lauf.ausgabe;
        EXPECT_TRUE(enthaelt(lauf.ausgabe, endzeile_rot(0, 2, 0, 0, 1, 0))) << lauf.ausgabe;
    }

    // (e) Gegenrichtung: wohlgeformt und stumm.
    std::string const stumm = gebaut + " | datei:tests/unit/y_" + marke + ".hpp | stumm " + marke + "\n";
    ASSERT_TRUE(fall.repo().schreibe("scripts/ci_test_registrierungs_allowlist.txt",
                                     "# Allowlist des Falls " + marke + "\n" + tragend + stumm));
    Lauf const gruen = fall.fahren();
    berichten("AllowlistFormfehlerFuerDateiImBauwegIstUnpruefbar/wohlgeformt-stumm", gruen, marke);
    EXPECT_EQ(gruen.code, 0) << "Eine wohlgeformte Zeile fuer eine Datei im Bauweg ist stumm -- kein Befund.\n"
                             << gruen.ausgabe;
    EXPECT_FALSE(enthaelt(gruen.ausgabe, "stumm " + marke))
        << "Die stumme Zeile darf nie ausgewertet worden sein -- ihr Feld 3 gehoert nicht in die Ausgabe.\n"
        << gruen.ausgabe;
    EXPECT_TRUE(enthaelt(gruen.ausgabe, nenner_davon(1, 0, 0, 0, 0))) << gruen.ausgabe;
    EXPECT_TRUE(enthaelt(gruen.ausgabe, nenner_ohne_bauweg(0, 0, 0, 0, 0, 0))) << gruen.ausgabe;
    EXPECT_TRUE(enthaelt(gruen.ausgabe, endzeile_ok(2, 0))) << gruen.ausgabe;
}

#endif // _WIN32
