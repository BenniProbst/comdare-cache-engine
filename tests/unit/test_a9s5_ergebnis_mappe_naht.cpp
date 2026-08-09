// test_a9s5_ergebnis_mappe_naht -- A9-S5: die Naht, an der aus einem Mess-Lauf eine Mappe entsteht.
//
// DIE ZWEI ZUSICHERUNGEN, die dieses Paket schuldet:
//   (1) LEERES <writeback_methods> erzeugt WIRKLICH eine xlsx-Mappe -- nicht "einen Aufruf, der nicht
//       wirft", sondern eine Datei, die mit der ZIP-Signatur beginnt (eine xlsx IST ein ZIP).
//   (2) GEGENEINGANG: csv gewaehlt => ein Ordner mit flachen Sheet-Dateien und KEINE xlsx.
//
// T-2 (Aussage statt Anwesenheit): keine Zusicherung dieses Tests lautet "existiert" oder "wirft
// nicht". Geprueft werden Werte (ZIP-Signatur, exakter Zellinhalt), Positionen (Spalten-Index einer
// leeren Zelle), Mengen (Feldzahl, Sheet-Dateien im Ordner) und Klassen (FormatLage).
//
// T-5 (Orakel unabhaengig): jeder Sollwert unten ist von Hand eingefroren -- keiner wird aus der
// geprueften Funktion abgeleitet.
//
// T-3 (Nenner fremd): die Grundgesamtheit "3 Eintraege" im golden-Fall stammt NICHT aus dem Prueflung,
// sondern aus libs/cache_engine/algorithm_profiles/thesis_profiles/all_axes_golden.profile.xml:216-220
// (dort woertlich csv + latex_table + comparison_metrics). Der Test zitiert diese Datei als Orakel.

#include "ergebnis_mappe_naht.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace naht = comdare::cache_engine::lager_naht;
namespace lab  = comdare::cache_engine::builder::lager_ablage;

namespace {

struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("a9s5_naht_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
        std::filesystem::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

std::string read_file(std::filesystem::path const& p) {
    std::ifstream is(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
}

/// Alle regulaeren Dateien mit der gegebenen Endung im Ordner -- die MENGE ist die Zusicherung,
/// nicht die blosse Anwesenheit einer einzelnen Datei.
std::vector<std::string> dateien_mit_endung(std::filesystem::path const& dir, std::string const& endung) {
    std::vector<std::string> namen;
    for (auto const& e : std::filesystem::directory_iterator(dir)) {
        if (!e.is_regular_file()) continue;
        auto const name = e.path().filename().string();
        if (name.size() >= endung.size() && name.compare(name.size() - endung.size(), endung.size(), endung) == 0)
            namen.push_back(name);
    }
    return namen;
}

} // namespace

// =================================================================================================
// A. FORMAT-ENTSCHEIDUNG
// =================================================================================================

TEST(A9S5FormatWahl, LeerErgibtXlsxUndNenntDieGrundgesamtheit) {
    auto const w = naht::waehle_ergebnis_format({});
    EXPECT_EQ(w.format, lab::ErgebnisFormat::xlsx) << "LEER muss der Owner-KERN-Default xlsx sein.";
    EXPECT_EQ(w.lage, naht::FormatLage::default_leer);
    EXPECT_EQ(w.token_gesamt, 0u);
    EXPECT_EQ(w.format_token, 0u);
    EXPECT_TRUE(w.eindeutig());
    // Der Nenner darf nicht nur im Kopf stehen: er muss in der AUSGABE erscheinen.
    EXPECT_NE(w.diagnose().find("format_token=0/0"), std::string::npos)
        << "diagnose() muss Teilmenge UND Grundgesamtheit tragen, war: " << w.diagnose();
}

// T-3: der Nenner 3 kommt aus all_axes_golden.profile.xml:216-220, nicht aus dem Prueflung.
TEST(A9S5FormatWahl, GoldenTripelWaehltCsvUeberDreiEintraegen) {
    std::vector<std::string> const golden{"csv", "latex_table", "comparison_metrics"};
    auto const                     w = naht::waehle_ergebnis_format(golden);
    EXPECT_EQ(w.format, lab::ErgebnisFormat::csv);
    EXPECT_EQ(w.lage, naht::FormatLage::gewaehlt);
    EXPECT_EQ(w.token_gesamt, 3u) << "Grundgesamtheit = die 3 Eintraege des golden-Profils.";
    EXPECT_EQ(w.format_token, 1u) << "Nur 'csv' ist ein FORMAT-Token; die anderen zwei sind Kanaele.";
    EXPECT_EQ(w.kanal_token, 2u);
    EXPECT_EQ(w.unbekannt_token, 0u);
    // Die eigentliche Regressions-Wache: eine wortwoertlich uebernommene run_methodology-Regel
    // (size() > 1 => Fehler) wuerde hier mehrdeutig/abgelehnt liefern und die Abgabe-Messung brechen.
    EXPECT_TRUE(w.eindeutig()) << "3 Eintraege duerfen die Abgabe-Messung NICHT mehrdeutig machen.";
}

TEST(A9S5FormatWahl, NurKanaeleFallenAufXlsxZurueck) {
    auto const w = naht::waehle_ergebnis_format({"latex_table", "comparison_metrics"});
    EXPECT_EQ(w.format, lab::ErgebnisFormat::xlsx);
    EXPECT_EQ(w.lage, naht::FormatLage::default_leer) << "Kein FORMAT-Token => Default-Lage, nicht 'gewaehlt'.";
    EXPECT_EQ(w.token_gesamt, 2u);
    EXPECT_EQ(w.format_token, 0u);
}

TEST(A9S5FormatWahl, XlsxWirdGewaehlt) {
    auto const w = naht::waehle_ergebnis_format({"xlsx"});
    EXPECT_EQ(w.format, lab::ErgebnisFormat::xlsx);
    EXPECT_EQ(w.lage, naht::FormatLage::gewaehlt) << "Explizites xlsx ist 'gewaehlt', nicht 'default_leer'.";
    EXPECT_EQ(w.format_token, 1u);
}

// T-4 GEGENEINGANG zur exactly-one-Zusicherung: der Eingang, bei dem sie NICHT gilt.
TEST(A9S5FormatWahl, CsvUndXlsxGleichzeitigIstMehrdeutigUndRaetNicht) {
    auto const w = naht::waehle_ergebnis_format({"csv", "xlsx"});
    EXPECT_EQ(w.lage, naht::FormatLage::mehrdeutig);
    EXPECT_FALSE(w.eindeutig()) << "Zwei konkurrierende FORMAT-Token duerfen NICHT still aufgeloest werden.";
    EXPECT_EQ(w.format_token, 2u);
    EXPECT_EQ(w.token_gesamt, 2u);
}

// Grenzfall daneben: zweimal DASSELBE Format ist kein Konflikt.
TEST(A9S5FormatWahl, DoppeltesCsvIstKeineMehrdeutigkeit) {
    auto const w = naht::waehle_ergebnis_format({"csv", "csv"});
    EXPECT_EQ(w.lage, naht::FormatLage::gewaehlt);
    EXPECT_EQ(w.format, lab::ErgebnisFormat::csv);
    EXPECT_EQ(w.format_token, 2u) << "Beide Nennungen zaehlen in die Teilmenge -- sie kollidieren nur nicht.";
}

// validate_profile() laeuft auf dem Mess-Pfad NICHT (profile_run_facade.cpp:776 vs. :809): ein
// unbekanntes Token erreicht die Wahl ungeprueft. Es darf weder den Lauf brechen noch still
// verschwinden -- es muss BENANNT werden.
TEST(A9S5FormatWahl, UnbekanntesTokenWirdBenanntNichtGeschluckt) {
    auto const w = naht::waehle_ergebnis_format({"parquet"});
    EXPECT_EQ(w.format, lab::ErgebnisFormat::xlsx) << "Unbekanntes Token ist kein Format-Token => Default bleibt.";
    EXPECT_EQ(w.unbekannt_token, 1u);
    EXPECT_EQ(w.unbekannt_liste, "parquet") << "Die id selbst muss erhalten bleiben, nicht nur gezaehlt.";
    EXPECT_NE(w.diagnose().find("parquet"), std::string::npos)
        << "Das unbekannte Token muss in der AUSGABE stehen, war: " << w.diagnose();
}

// =================================================================================================
// B. FELD-ADAPTER -- die Wache gegen die stille Spalten-Verschiebung
// =================================================================================================

// DIE zentrale Bissprobe dieses Pakets. split_on() aus profile_run_facade.cpp:99-108 verwirft leere
// Felder; uebernaehme man es hier, lieferte dieser Eingang {"a","b"} statt {"a","","b"} -- und JEDER
// Messwert rechts der leeren Zelle stuende ab da unter der falschen Ueberschrift.
TEST(A9S5FeldAdapter, LeeresFeldBleibtErhaltenUndHaeltDieSpaltenPosition) {
    auto const felder = naht::csv_zeile_in_felder("a;;b\n");
    ASSERT_EQ(felder.size(), 3u) << "Ein Split, der leere Felder verwirft, liefert hier 2 -- das ist der Defekt.";
    EXPECT_EQ(felder[0], "a");
    EXPECT_EQ(felder[1], "") << "Position 1 MUSS die leere Zelle sein.";
    EXPECT_EQ(felder[2], "b") << "'b' MUSS auf Spalte 2 bleiben, nicht auf 1 rutschen.";
}

TEST(A9S5FeldAdapter, NurTrennerErgibtDreiLeereFelder) {
    auto const felder = naht::csv_zeile_in_felder(";;\n");
    ASSERT_EQ(felder.size(), 3u);
    EXPECT_EQ(felder[0], "");
    EXPECT_EQ(felder[1], "");
    EXPECT_EQ(felder[2], "");
}

TEST(A9S5FeldAdapter, ZeileOhneUmbruchUndMitCrLf) {
    auto const ohne = naht::csv_zeile_in_felder("a;b");
    ASSERT_EQ(ohne.size(), 2u);
    EXPECT_EQ(ohne[1], "b");

    auto const crlf = naht::csv_zeile_in_felder("a;b\r\n");
    ASSERT_EQ(crlf.size(), 2u);
    EXPECT_EQ(crlf[1], "b") << "Ein zurueckbleibendes '\\r' wuerde die letzte Zelle verfaelschen.";
}

TEST(A9S5FeldAdapter, LeereZeileErgibtKeinFeld) {
    EXPECT_EQ(naht::csv_zeile_in_felder("\n").size(), 0u);
    EXPECT_EQ(naht::csv_zeile_in_felder("").size(), 0u);
}

TEST(A9S5FeldAdapter, BlobZerfaelltInZeilenOhneLeereSchlusszeile) {
    auto const zeilen = naht::csv_blob_in_zeilen("r1;x\nr2;y\n");
    ASSERT_EQ(zeilen.size(), 2u) << "Das abschliessende '\\n' darf keine dritte, leere Zeile erzeugen.";
    EXPECT_EQ(zeilen[0], "r1;x");
    EXPECT_EQ(zeilen[1], "r2;y");
}

// =================================================================================================
// C. ENDE-ZU-ENDE -- die beiden geschuldeten Zusicherungen
// =================================================================================================

// Von Hand eingefrorenes Orakel (T-5): Kopf + zwei Zeilen, die zweite mit einer LEEREN Zelle in der
// Mitte -- damit die Spaltentreue nicht nur im Adapter, sondern bis in die Datei geprueft ist.
constexpr char const* kKopf   = "binary_id;setting;n_ops\n";
constexpr char const* kZeile1 = "b1;s1;100\n";
constexpr char const* kZeile2 = "b2;;200\n";

// ZUSICHERUNG (1): leeres writeback_methods => WIRKLICH eine xlsx.
TEST(A9S5Naht, LeeresWritebackErzeugtEchteXlsxMappe) {
    TempDir const   tmp;
    auto const      out_csv = tmp.path / "measurements.csv";
    naht::MappenNaht n;
    n.oeffnen(out_csv, {}); // LEER

    ASSERT_TRUE(n.scharf()) << "Naht nicht scharf: " << n.diagnose();
    EXPECT_EQ(n.wahl().format, lab::ErgebnisFormat::xlsx);

    n.kopf_aus_csv(kKopf);
    n.zeile_aus_csv(kZeile1);
    n.zeile_aus_csv(kZeile2);
    EXPECT_EQ(n.zeilen(), 2u);
    EXPECT_EQ(n.kopf_spalten(), 3u) << "Grundgesamtheit der Spalten-Wache.";
    EXPECT_EQ(n.feld_abweichungen(), 0u) << "Beide Zeilen haben 3 Felder wie der Kopf.";

    ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << n.diagnose();

    // MENGE: genau EINE xlsx im Ordner -- und KEINE flachen Sheet-CSVs.
    auto const xlsx_dateien = dateien_mit_endung(tmp.path, ".xlsx");
    ASSERT_EQ(xlsx_dateien.size(), 1u) << "Es muss genau eine .xlsx entstehen.";
    EXPECT_TRUE(dateien_mit_endung(tmp.path, "__S001.csv").empty())
        << "Im xlsx-Modus darf KEIN flaches Sheet-CSV danebenliegen.";
    EXPECT_TRUE(dateien_mit_endung(tmp.path, ".xlsx.tmp").empty()) << "Die tmp-Datei muss umbenannt worden sein.";

    // WERT statt Anwesenheit: eine xlsx IST ein ZIP -- die ersten vier Bytes sind die lokale
    // Datei-Kopf-Signatur 'PK\x03\x04'. Eine leere oder abgebrochene Datei faellt hier durch.
    auto const inhalt = read_file(tmp.path / xlsx_dateien.front());
    ASSERT_GE(inhalt.size(), 4u) << "xlsx ist leer/abgeschnitten.";
    EXPECT_EQ(static_cast<unsigned char>(inhalt[0]), 0x50u);
    EXPECT_EQ(static_cast<unsigned char>(inhalt[1]), 0x4Bu);
    EXPECT_EQ(static_cast<unsigned char>(inhalt[2]), 0x03u);
    EXPECT_EQ(static_cast<unsigned char>(inhalt[3]), 0x04u);

    // Der Stamm folgt der Lagerbaum-Grammatik <datum>-<zeit>.xlsx: 8 Ziffern, '-', 6 Ziffern.
    auto const stamm = xlsx_dateien.front().substr(0, xlsx_dateien.front().size() - 5);
    ASSERT_EQ(stamm.size(), 15u) << "Erwartete Grammatik <8 Ziffern>-<6 Ziffern>, war: " << stamm;
    EXPECT_EQ(stamm[8], '-');
    for (std::size_t i = 0; i < stamm.size(); ++i) {
        if (i == 8) continue;
        EXPECT_TRUE(stamm[i] >= '0' && stamm[i] <= '9') << "Stamm-Zeichen " << i << " ist keine Ziffer: " << stamm;
    }
}

// ZUSICHERUNG (2) / T-4 GEGENEINGANG: csv gewaehlt => flache Sheets, KEINE xlsx.
TEST(A9S5Naht, CsvGewaehltErzeugtFlacheSheetsUndKeineXlsx) {
    TempDir const   tmp;
    auto const      out_csv = tmp.path / "measurements.csv";
    naht::MappenNaht n;
    n.oeffnen(out_csv, {"csv"});

    ASSERT_TRUE(n.scharf()) << "Naht nicht scharf: " << n.diagnose();
    EXPECT_EQ(n.wahl().format, lab::ErgebnisFormat::csv);

    n.kopf_aus_csv(kKopf);
    n.zeile_aus_csv(kZeile1);
    n.zeile_aus_csv(kZeile2);
    ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << n.diagnose();

    // KEINE xlsx -- das ist die halbe Aussage dieses Gegeneingangs.
    EXPECT_TRUE(dateien_mit_endung(tmp.path, ".xlsx").empty()) << "Im csv-Modus darf KEINE xlsx entstehen.";

    // Flache Sheets in EINEM Ordner: genau ein S001-Blatt + genau ein INFO-Blatt.
    auto const sheets = dateien_mit_endung(tmp.path, "__S001.csv");
    ASSERT_EQ(sheets.size(), 1u) << "Genau ein flaches Sheet-CSV erwartet.";
    ASSERT_EQ(dateien_mit_endung(tmp.path, "__INFO.csv").size(), 1u) << "Das INFO-Blatt gehoert dazu.";
    EXPECT_TRUE(dateien_mit_endung(tmp.path, ".csv.tmp").empty()) << "Kein tmp-Rest nach schliessen().";

    // WERT: der Inhalt ist byte-genau der eingefrorene Erwartungswert -- inklusive der LEEREN Zelle
    // an Position 1 der zweiten Zeile. Ein Split, der leere Felder verwirft, liefert hier "b2;200".
    std::string const erwartet = "binary_id;setting;n_ops\nb1;s1;100\nb2;;200\n";
    EXPECT_EQ(read_file(tmp.path / sheets.front()), erwartet);
}

// T-4 zur Spalten-Wache: eine Zeile mit zu wenigen Feldern MUSS auffallen -- und eine mit zu
// VIELEN ebenso. (Lens A9-S5, F1: '!=' -> '<' ueberlebte 16/16, weil nur die kurze Richtung
// einen Gegeneingang hatte. Die lange Richtung ist die realistische: eine neue Spalte in
// format_csv_row() ohne die passende in lazy_csv_header() macht JEDE Zeile ein Feld zu lang.)
TEST(A9S5Naht, ZeileMitFalscherFeldzahlWirdGezaehlt) {
    TempDir const   tmp;
    naht::MappenNaht n;
    n.oeffnen(tmp.path / "measurements.csv", {"csv"});
    ASSERT_TRUE(n.scharf()) << n.diagnose();

    n.kopf_aus_csv(kKopf);      // 3 Spalten
    n.zeile_aus_csv(kZeile1);   // 3 Felder -- passt
    n.zeile_aus_csv("b3;s3\n"); // 2 Felder -- passt NICHT (zu KURZ)

    EXPECT_EQ(n.kopf_spalten(), 3u) << "Die Grundgesamtheit der Wache.";
    EXPECT_EQ(n.feld_abweichungen(), 1u) << "Genau die eine zu kurze Zeile muss gezaehlt werden.";

    n.zeile_aus_csv("b4;s4;400;u4\n"); // 4 Felder -- passt NICHT (zu LANG)
    EXPECT_EQ(n.feld_abweichungen(), 2u)
        << "Auch die zu LANGE Zeile (4 Felder gegen 3 Kopfspalten) muss zaehlen -- eine Wache, "
           "die nur nach unten schaut, laesst '!=' -> '<' ueberleben.";

    EXPECT_EQ(n.zeilen(), 3u) << "Geschrieben werden alle drei -- die Wache meldet, sie unterschlaegt nicht.";
    EXPECT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {}));
}

// Mehrdeutigkeit => die Naht geht gar nicht erst scharf, und der Grund steht in der Ausgabe.
TEST(A9S5Naht, MehrdeutigesFormatOeffnetKeineMappe) {
    TempDir const   tmp;
    naht::MappenNaht n;
    n.oeffnen(tmp.path / "measurements.csv", {"csv", "xlsx"});

    EXPECT_FALSE(n.scharf());
    EXPECT_NE(n.diagnose().find("mehrdeutig"), std::string::npos) << "Grund fehlt in der Ausgabe: " << n.diagnose();
    EXPECT_TRUE(dateien_mit_endung(tmp.path, ".xlsx").empty()) << "Bei Mehrdeutigkeit darf NICHTS geraten werden.";
    EXPECT_TRUE(dateien_mit_endung(tmp.path, "__S001.csv").empty());
    EXPECT_FALSE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << "schliessen() ohne geoeffnete Mappe ist false.";
}
