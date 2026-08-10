// tests/unit/test_d3_6_realm_selektor_produktionsablage.cpp -- D3-6 (Schwesterstelle in ce):
// der --realm-root-Selektor von tools/mess_report/realm_scan.hpp trifft die PRODUKTIONS-Ablage NIE.
//
// DER BEFUND, am Objekt gemessen (2026-08-10, ce origin/development c1791714):
//   Der Erzeuger schreibt die Mess-CSV als bin_dir/"result.csv" -- ein Verzeichnis je Binary-Stamm,
//   darin eine Datei mit dem FESTEN Namen "result.csv"
//   (libs/cache_engine/builder/experiment_tree/cache_engine_builder_iterator.hpp:2950,
//    `std::ofstream pf{bin_dir / "result.csv", std::ios::trunc};`).
//   Der Selektor sucht dagegen ausschliesslich den SUFFIX ".result.csv"
//   (realm_scan.hpp:35 kResultCsvSuffix + :86 endet_mit(name, kResultCsvSuffix)).
//   "result.csv" ist 10 Zeichen, ".result.csv" ist 11 -- der Suffix-Vergleich kann rechnerisch NIE
//   halten. Die beiden Muster sind DISJUNKT: der Bericht findet auf einem frischen Produktionslauf
//   null Dateien und meldet das als "enthaelt keine *.result.csv" -- ohne jeden Nenner, also ohne
//   Unterschied zwischen "nichts da" und "falsch gesucht".
//
// WARUM DAS BISHER NIEMAND SAH -- der abgeschriebene Koeder: die vorhandene Deckung fuettert dem
// Werkzeug ausschliesslich die Form, die es ohnehin erwartet. tests/unit/test_a9s4_realm_scan_stale.cpp
// legt "a_0_1111.result.csv" an, und der CI-Rauchtest .gitlab-ci.yml:680 schreibt sich seinen Koeder
// mit `SMOKE_CSV=".../smoke_0_deadbeef.result.csv"` selbst. Kein einziger Fall der Grundgesamtheit
// benutzt die Form, die der Erzeuger wirklich schreibt. Genau diese Luecke schliesst diese Datei.
//
// T-3 NENNER, FREMD: die Grundgesamtheit ("welchen Dateinamen schreibt die Produktion?") stammt NICHT
// aus dem Pruefling (realm_scan.hpp), sondern aus dem ERZEUGER -- dem Literal an
// cache_engine_builder_iterator.hpp:2950, hier als kErzeugerDateiname abgeschrieben (Abschrift mit
// Fundstelle, nicht Verweis auf die Konstante des Prueflings). Zusaetzlich wird die dritte Stelle,
// die denselben Namen fuehrt (planner_status_types.hpp::kResultCsvName), GEGEN diese Abschrift
// geprueft -- driftet eine der drei Stellen, beisst dieser Test.
//
// K13 KOEDER BEIDSEITIG: die Marken stammen aus /dev/urandom (gezogen 2026-08-10), NICHT abgeschrieben.
//   Produktionsform  ba95ba8aaeaa824b1aefa06e3ddf5016   muss ANKOMMEN
//   Archivform       6fe9084c75829ec71367013234cf3668   muss ANKOMMEN
//   Gegenkoeder      df239b8eb72268027f5b279d79d88960   liegt in .stale und darf NIE ankommen

#include "../../libs/cache_engine/profile_facade/planner/planner_status_types.hpp"
#include "../../tools/mess_report/realm_scan.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace mr  = ::comdare::cache_engine::tools::mess_report;
namespace pln = ::comdare::cache_engine::planner;

namespace {

/// ABSCHRIFT des Erzeuger-Literals, mit Fundstelle -- die fremde Grundgesamtheit dieses Tests.
/// Quelle: libs/cache_engine/builder/experiment_tree/cache_engine_builder_iterator.hpp:2950
///         `std::ofstream pf{bin_dir / "result.csv", std::ios::trunc};`
/// Bewusst KEIN Verweis auf mr::-Konstanten: der Pruefling darf seinen eigenen Nenner nicht stellen.
constexpr std::string_view kErzeugerDateiname = "result.csv";

/// Die drei /dev/urandom-Marken dieses Pakets (2026-08-10 gezogen, s. Kopf).
constexpr std::string_view kKoederProduktion = "ba95ba8aaeaa824b1aefa06e3ddf5016";
constexpr std::string_view kKoederArchiv     = "6fe9084c75829ec71367013234cf3668";
constexpr std::string_view kGegenkoederStale = "df239b8eb72268027f5b279d79d88960";

/// RAII-Temp-Verzeichnis -- dasselbe Muster wie tests/unit/test_a9s4_realm_scan_stale.cpp::TempDir.
struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("d3_6_realm_selektor_" + std::to_string(counter.fetch_add(1)) + "_" +
                std::to_string(::time(nullptr)));
        std::filesystem::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
    TempDir(TempDir const&)            = delete;
    TempDir& operator=(TempDir const&) = delete;
};

void schreibe(std::filesystem::path const& p, std::string const& inhalt) {
    std::filesystem::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    f << inhalt;
}

/// Eine Mess-CSV in der Form, die der Erzeuger schreibt: Kopfzeile mit Pflichtspalte binary_id,
/// eine Datenzeile, die die Marke traegt (die Marke muss den ganzen Weg ueberstehen, nicht nur den Namen).
[[nodiscard]] std::string mess_csv(std::string_view binary_id, std::string_view marke) {
    return "binary_id;setting;ns_per_op\n" + std::string{binary_id} + ";" + std::string{marke} + ";42\n";
}

[[nodiscard]] std::string lies(std::filesystem::path const& p) {
    std::ifstream     f(p, std::ios::binary);
    std::string const inhalt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return inhalt;
}

} // namespace

// -- Fall 1: der Koeder gegen das WERKZEUG -- die Rechnung, aus der der Defekt folgt --------------
// Erst der Beweis am nackten Vergleich, dann am Scan (dieselbe Reihenfolge wie
// test_a9s4_realm_scan_stale.cpp: "erst der Koeder gegen das Werkzeug, dann gegen die Wache").
TEST(D36RealmSelektor, SuffixVergleichKannDenProduktionsnamenRechnerischNieTreffen) {
    // Die drei Stellen, die denselben Namen fuehren, muessen uebereinstimmen -- sonst ist der Nenner
    // dieses Tests schon falsch, bevor der Scan ueberhaupt laeuft (FAIL-CLOSED).
    EXPECT_EQ(std::string_view{pln::kResultCsvName}, kErzeugerDateiname)
        << "planner_status_types.hpp::kResultCsvName ist vom Erzeuger-Literal abgedriftet";

    // Der Defekt selbst: der Produktionsname ist KUERZER als der gesuchte Suffix.
    EXPECT_LT(kErzeugerDateiname.size(), mr::kResultCsvSuffix.size())
        << "'" << kErzeugerDateiname << "' vs '" << mr::kResultCsvSuffix << "'";
    EXPECT_FALSE(mr::endet_mit(kErzeugerDateiname, mr::kResultCsvSuffix))
        << "der Suffix-Vergleich allein kann die Produktions-Ablage strukturell nie treffen";
}

// -- Fall 2: die Wache selbst, gegen die ECHTE Produktions-Ablage (ROT vor der Heilung) -----------
// Baum wie ihn ein frischer Lauf hinterlaesst: <stamm>/result.csv, ein Verzeichnis je Binary.
TEST(D36RealmSelektor, ProduktionsAblageWirdGEFUNDEN_FremderNennerAusDemErzeuger) {
    TempDir const tmp;
    auto const    a = tmp.path / "search_algo_eytzinger_0_1e789132991acf07" / std::string{kErzeugerDateiname};
    auto const    b = tmp.path / "search_algo_k_ary_1_45e8ae58614b7ce7" / std::string{kErzeugerDateiname};
    schreibe(a, mess_csv("eytzinger_0", kKoederProduktion));
    schreibe(b, mess_csv("k_ary_1", kKoederProduktion));

    auto const erg = mr::scanne_realm_root(tmp.path);

    ASSERT_TRUE(erg.wurzel_vorhanden);
    // NENNER IN DER AUSSAGE: 2 angelegt, 2 erwartet -- eine nackte ">0"-Pruefung waere hier blind.
    ASSERT_EQ(erg.gefundene_csvs.size(), 2u)
        << "2 Produktions-CSVs angelegt, gefunden: " << erg.gefundene_csvs.size()
        << " -- besuchte Eintraege: " << erg.besuchte_eintraege;
    // Der Koeder muss ANKOMMEN, nicht nur der Dateiname passen (V-8: Gegenstand statt Ankuendigung).
    for (auto const& p : erg.gefundene_csvs)
        EXPECT_NE(lies(p).find(std::string{kKoederProduktion}), std::string::npos)
            << p.string() << " traegt die Marke nicht";
}

// -- Fall 3: BEIDE Ablageformen in EINEM Baum, EIN Selektor (Kernforderung 1) ---------------------
TEST(D36RealmSelektor, BeideAblageformenWerdenVonEinemSelektorGetroffen) {
    TempDir const tmp;
    // Produktionsform: <stamm>/result.csv
    schreibe(tmp.path / "eytzinger_0_1e789132991acf07" / std::string{kErzeugerDateiname},
             mess_csv("eytzinger_0", kKoederProduktion));
    // Archivform: per_binary/<binary_id>.result.csv (real belegt im Erstbeleg-Archiv vom 26.07.2026,
    // super/docs/architektur/measurement/erstbeleg-d03-20260726/.../per_binary/*.result.csv, 8 Dateien).
    schreibe(tmp.path / "per_binary" / "search_algo_k_ary_1_45e8ae58614b7ce7.result.csv",
             mess_csv("k_ary_1", kKoederArchiv));

    auto const erg = mr::scanne_realm_root(tmp.path);

    ASSERT_EQ(erg.gefundene_csvs.size(), 2u)
        << "eine Datei je Ablageform angelegt, gefunden: " << erg.gefundene_csvs.size();
    bool produktion_da = false;
    bool archiv_da     = false;
    for (auto const& p : erg.gefundene_csvs) {
        std::string const inhalt = lies(p);
        if (inhalt.find(std::string{kKoederProduktion}) != std::string::npos) produktion_da = true;
        if (inhalt.find(std::string{kKoederArchiv}) != std::string::npos) archiv_da = true;
    }
    EXPECT_TRUE(produktion_da) << "die Produktions-Marke ist nicht angekommen";
    EXPECT_TRUE(archiv_da) << "die Archiv-Marke ist nicht angekommen";
}

// -- Fall 4: T-4 GEGENEINGANG -- die Zusicherung gilt hier NICHT ----------------------------------
// Ein weiter gefasster Selektor darf die Ausschluesse nicht mitreissen. Vier Eingaenge, bei denen
// "wird gefunden" FALSCH sein muss -- zwei davon in der Produktionsform, die es vorher gar nicht gab.
TEST(D36RealmSelektor, GegeneingangStaleStampUndAggregatBleibenDraussen) {
    TempDir const tmp;
    auto const    stamm = tmp.path / "eytzinger_0_1e789132991acf07";
    // (1) Produktions-.stale: DIE neue Gefahr -- "result.csv.stale" enthaelt "result.csv" als PRAEFIX.
    schreibe(stamm / std::string{pln::kResultStaleName}, mess_csv("eytzinger_0", kGegenkoederStale));
    // (2) Archiv-.stale (der bereits gedeckte Fall, hier als Nachbar mitgemessen).
    schreibe(tmp.path / "per_binary" / "k_ary_1_45e8ae58614b7ce7.result.csv.stale",
             mess_csv("k_ary_1", kGegenkoederStale));
    // (3) Der Resume-Stempel -- kein Messdatum.
    schreibe(stamm / std::string{pln::kResultStampName}, "rows=3\n");
    // (4) Die Aggregat-Datei -- bewusst NICHT mitnehmen (sonst zaehlte jede Zeile doppelt,
    //     realm_scan.hpp:5-9).
    schreibe(tmp.path / "measurements.csv", mess_csv("eytzinger_0", kGegenkoederStale));

    auto const erg = mr::scanne_realm_root(tmp.path);

    EXPECT_EQ(erg.gefundene_csvs.size(), 0u)
        << "4 Gegeneingaenge angelegt, 0 duerfen durch -- durchgekommen: " << erg.gefundene_csvs.size();
    for (auto const& p : erg.gefundene_csvs)
        EXPECT_EQ(lies(p).find(std::string{kGegenkoederStale}), std::string::npos)
            << p.string() << " hat den Gegenkoeder durchgelassen";
    EXPECT_EQ(erg.uebersprungen_stale, 2u) << "beide .stale-Formen muessen GEZAEHLT werden, nicht stumm fallen";
}

// -- Fall 5: V-1 -- der NENNER muss die Ausgabe verlassen, und zwar AUFGESCHLUESSELT --------------
// V-8 gefragt: "Was waere der Zustand, in dem diese Ausgabe erscheint und die Sache trotzdem nicht
// existiert?" -- Antwort: eine blosse Gesamtzahl. "gefunden=1" sieht auf einem Baum, der EINE
// Archivdatei und daneben unerkannte Produktionsdaten traegt, exakt so aus wie auf einem gesunden
// Baum. Erst die Aufschluesselung je Ablageform macht den Unterschied sichtbar. Genau deshalb prueft
// dieser Fall die ZAHLEN JE FORM und nicht nur ihre Summe.
TEST(D36RealmSelektor, BilanzTraegtDenNennerJeAblageformNichtNurDieSumme) {
    TempDir const tmp;
    schreibe(tmp.path / "eytzinger_0_1e789132991acf07" / std::string{kErzeugerDateiname},
             mess_csv("eytzinger_0", kKoederProduktion));
    schreibe(tmp.path / "k_ary_1_45e8ae58614b7ce7" / std::string{kErzeugerDateiname},
             mess_csv("k_ary_1", kKoederProduktion));
    schreibe(tmp.path / "per_binary" / "interpolation_1_c1d41b7e2653bff5.result.csv",
             mess_csv("interpolation_1", kKoederArchiv));
    schreibe(tmp.path / "per_binary" / "verworfen_2_2222222222222222.result.csv.stale",
             mess_csv("verworfen_2", kGegenkoederStale));

    auto const erg = mr::scanne_realm_root(tmp.path);

    ASSERT_EQ(erg.gefundene_csvs.size(), 3u);
    EXPECT_EQ(erg.gefunden_produktionsform, 2u);
    EXPECT_EQ(erg.gefunden_archivform, 1u);
    EXPECT_EQ(erg.gefunden_produktionsform + erg.gefunden_archivform, erg.gefundene_csvs.size())
        << "die Aufschluesselung muss die Fundmenge vollstaendig erklaeren -- sonst ist sie Zierde";

    // Der Text, den das Werkzeug wirklich ausgibt (main.cpp reicht ihn im Erfolgs- UND im Fehlerfall
    // durch). Er muss die Zahlen UND den Selektor tragen, sonst ist eine Null wieder unbelegt.
    std::string const bilanz = mr::scan_bilanz(erg);
    EXPECT_NE(bilanz.find("gefunden=3"), std::string::npos) << bilanz;
    EXPECT_NE(bilanz.find("produktionsform=2"), std::string::npos) << bilanz;
    EXPECT_NE(bilanz.find("archivform=1"), std::string::npos) << bilanz;
    EXPECT_NE(bilanz.find("uebersprungen_stale=1"), std::string::npos) << bilanz;
    EXPECT_NE(bilanz.find("besuchte_eintraege="), std::string::npos) << bilanz;
    // Wonach wurde gesucht? Ohne diese Angabe ist "0 gefunden" nicht interpretierbar.
    EXPECT_NE(bilanz.find(std::string{kErzeugerDateiname}), std::string::npos) << bilanz;
    EXPECT_NE(bilanz.find("*.result.csv"), std::string::npos) << bilanz;
}

// -- Fall 6: die Null MIT Nenner -- der Zustand, der ein halbes Jahr unbemerkt blieb --------------
// Der leere Baum und der "voll, aber falsch gesucht"-Baum muessen sich in der AUSGABE unterscheiden.
TEST(D36RealmSelektor, NullMitNenner_LeererBaumUndUnbeteiligterBaumSindUnterscheidbar) {
    TempDir const leer;
    auto const    erg_leer = mr::scanne_realm_root(leer.path);
    ASSERT_TRUE(erg_leer.wurzel_vorhanden);
    EXPECT_EQ(erg_leer.gefundene_csvs.size(), 0u);
    EXPECT_EQ(erg_leer.besuchte_eintraege, 0u) << "ein leerer Baum hat einen Nenner von 0 -- das ist die Aussage";

    TempDir const voll;
    for (int i = 0; i < 5; ++i)
        schreibe(voll.path / ("stamm_" + std::to_string(i)) / "notiz.txt", "kein Mess-Ergebnis");
    auto const erg_voll = mr::scanne_realm_root(voll.path);
    EXPECT_EQ(erg_voll.gefundene_csvs.size(), 0u);
    EXPECT_GT(erg_voll.besuchte_eintraege, erg_leer.besuchte_eintraege)
        << "zwei verschiedene Wirklichkeiten muessen zwei verschiedene Nenner liefern";
    EXPECT_NE(mr::scan_bilanz(erg_leer), mr::scan_bilanz(erg_voll))
        << "beide Nullen erzeugen denselben Text -- genau das war der Defekt";
}
