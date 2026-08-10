// test_a9s5_ergebnis_mappe_naht -- A9-S5: die Naht, an der aus einem Mess-Lauf eine Mappe entsteht.
//
// DIE DREI ZUSICHERUNGEN, die dieses Paket schuldet:
//   (1) LEERES <writeback_methods> erzeugt WIRKLICH eine xlsx-Mappe -- nicht "einen Aufruf, der nicht
//       wirft", sondern eine Datei, die mit der ZIP-Signatur beginnt (eine xlsx IST ein ZIP).
//   (2) GEGENEINGANG: csv gewaehlt => ein Ordner mit flachen Sheet-Dateien und KEINE xlsx.
//   (3) Owner-Entscheid 09.08.: csv UND xlsx zugleich => BEIDE Ausgaben, und zwar aus EINER Mappe.
//       Der Beleg dafuer ist nicht "beide Dateien liegen da", sondern ihr GEMEINSAMER Dateiname-
//       Stamm -- zwei Schreibwege oder zwei Laeufe haetten zwei Zeitstempel und zwei Staemme.
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

#include <zlib.h>

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
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
    EXPECT_TRUE(w.xlsx) << "LEER muss der Owner-KERN-Default xlsx sein.";
    EXPECT_FALSE(w.csv) << "LEER darf KEINE csv-Ausgabe hinzuerfinden.";
    EXPECT_EQ(w.lage, naht::FormatLage::default_leer);
    EXPECT_EQ(w.token_gesamt, 0u);
    EXPECT_EQ(w.format_token, 0u);
    EXPECT_EQ(w.ausgaben(), 1u);
    // Der Nenner darf nicht nur im Kopf stehen: er muss in der AUSGABE erscheinen.
    EXPECT_NE(w.diagnose().find("format_token=0/0"), std::string::npos)
        << "diagnose() muss Teilmenge UND Grundgesamtheit tragen, war: " << w.diagnose();
}

// T-3: der Nenner 3 kommt aus all_axes_golden.profile.xml:216-220, nicht aus dem Prueflung.
TEST(A9S5FormatWahl, GoldenTripelWaehltCsvUeberDreiEintraegen) {
    std::vector<std::string> const golden{"csv", "latex_table", "comparison_metrics"};
    auto const                     w = naht::waehle_ergebnis_format(golden);
    EXPECT_TRUE(w.csv);
    EXPECT_FALSE(w.xlsx) << "Ohne xlsx-Token darf die Abgabe-Messung KEINE xlsx danebenlegen.";
    EXPECT_EQ(w.lage, naht::FormatLage::gewaehlt);
    EXPECT_EQ(w.token_gesamt, 3u) << "Grundgesamtheit = die 3 Eintraege des golden-Profils.";
    EXPECT_EQ(w.format_token, 1u) << "Nur 'csv' ist ein FORMAT-Token; die anderen zwei sind Kanaele.";
    EXPECT_EQ(w.kanal_token, 2u);
    EXPECT_EQ(w.unbekannt_token, 0u);
    // Die eigentliche Regressions-Wache -- durch den Entscheid 09.08. NICHT entschaerft: eine
    // wortwoertlich uebernommene run_methodology-Regel (size() > 1 => Fehler) wuerde diesen Eingang
    // ablehnen und damit exakt die Abgabe-Messung brechen. 3 Eintraege sind EINE Ausgabe.
    EXPECT_EQ(w.ausgaben(), 1u) << "3 Eintraege duerfen die Abgabe-Messung NICHT ablehnen.";
}

TEST(A9S5FormatWahl, NurKanaeleFallenAufXlsxZurueck) {
    auto const w = naht::waehle_ergebnis_format({"latex_table", "comparison_metrics"});
    EXPECT_TRUE(w.xlsx);
    EXPECT_FALSE(w.csv);
    EXPECT_EQ(w.lage, naht::FormatLage::default_leer) << "Kein FORMAT-Token => Default-Lage, nicht 'gewaehlt'.";
    EXPECT_EQ(w.token_gesamt, 2u);
    EXPECT_EQ(w.format_token, 0u);
}

TEST(A9S5FormatWahl, XlsxWirdGewaehlt) {
    auto const w = naht::waehle_ergebnis_format({"xlsx"});
    EXPECT_TRUE(w.xlsx);
    EXPECT_FALSE(w.csv);
    EXPECT_EQ(w.lage, naht::FormatLage::gewaehlt) << "Explizites xlsx ist 'gewaehlt', nicht 'default_leer'.";
    EXPECT_EQ(w.format_token, 1u);
}

// OWNER-ENTSCHEID 09.08.: der Eingang, den die erste Fassung dieser Datei noch ABLEHNTE. Er ist
// jetzt der VOLLE Eingang -- beide Formate, eine Mappe. (Bis 09.08. stand hier das Gegenteil:
// "mehrdeutig ... es wird NICHT geraten". Die Ausschliesslichkeit ist aufgehoben, sonst nichts.)
TEST(A9S5FormatWahl, CsvUndXlsxGleichzeitigErgibtBEIDEAusgaben) {
    auto const w = naht::waehle_ergebnis_format({"csv", "xlsx"});
    EXPECT_EQ(w.lage, naht::FormatLage::beide);
    EXPECT_TRUE(w.xlsx);
    EXPECT_TRUE(w.csv);
    EXPECT_EQ(w.ausgaben(), 2u) << "Beide Formate zugleich sind ein GUELTIGER Eingang, kein Fehler.";
    EXPECT_EQ(w.format_token, 2u);
    EXPECT_EQ(w.token_gesamt, 2u);
    // Die Lauf-Ausgabe muss BEIDE nennen -- sonst bliebe die zweite Datei unbeobachtet.
    EXPECT_NE(w.diagnose().find("format=xlsx+csv"), std::string::npos)
        << "diagnose() muss beide Formate nennen, war: " << w.diagnose();
}

// Grenzfall daneben, der die Zaehlung von der AUSGABE trennt: zweimal DASSELBE Format sind zwei
// Nennungen, aber EINE Ausgabe -- nicht zwei. Ein `format_token`-basierter Kurzschluss faellt hier.
TEST(A9S5FormatWahl, DoppeltesCsvBleibtEineAusgabe) {
    auto const w = naht::waehle_ergebnis_format({"csv", "csv"});
    EXPECT_EQ(w.lage, naht::FormatLage::gewaehlt);
    EXPECT_TRUE(w.csv);
    EXPECT_FALSE(w.xlsx);
    EXPECT_EQ(w.ausgaben(), 1u) << "Zwei Nennungen DESSELBEN Formats sind EINE Ausgabe.";
    EXPECT_EQ(w.format_token, 2u) << "Beide Nennungen zaehlen in die Teilmenge -- sie sind nur dasselbe.";
}

// validate_profile() laeuft auf dem Mess-Pfad NICHT (profile_run_facade.cpp:776 vs. :809): ein
// unbekanntes Token erreicht die Wahl ungeprueft. Es darf weder den Lauf brechen noch still
// verschwinden -- es muss BENANNT werden.
TEST(A9S5FormatWahl, UnbekanntesTokenWirdBenanntNichtGeschluckt) {
    auto const w = naht::waehle_ergebnis_format({"parquet"});
    EXPECT_TRUE(w.xlsx) << "Unbekanntes Token ist kein Format-Token => Default bleibt.";
    EXPECT_FALSE(w.csv);
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
    TempDir const    tmp;
    auto const       out_csv = tmp.path / "measurements.csv";
    naht::MappenNaht n;
    n.oeffnen(out_csv, {}); // LEER

    ASSERT_TRUE(n.scharf()) << "Naht nicht scharf: " << n.diagnose();
    EXPECT_TRUE(n.wahl().xlsx);
    EXPECT_FALSE(n.wahl().csv);

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
    TempDir const    tmp;
    auto const       out_csv = tmp.path / "measurements.csv";
    naht::MappenNaht n;
    n.oeffnen(out_csv, {"csv"});

    ASSERT_TRUE(n.scharf()) << "Naht nicht scharf: " << n.diagnose();
    EXPECT_TRUE(n.wahl().csv);
    EXPECT_FALSE(n.wahl().xlsx) << "Ohne xlsx-Token darf KEINE xlsx-Ausgabe entstehen.";

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
    TempDir const    tmp;
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

// ZUSICHERUNG (3) / OWNER-ENTSCHEID 09.08.: beide Formate => BEIDE Ausgaben, aus EINER Mappe.
//
// Bis zum 09.08. stand hier das Gegenteil (MehrdeutigesFormatOeffnetKeineMappe: "scharf()==false,
// es wird NICHT geraten"). Die Ausschliesslichkeit ist aufgehoben -- der Rest des 05.08.-Kanons
// nicht: es bleibt EINE Mappe. Die schaerfste pruefbare Aussage dafuer ist der GEMEINSAME
// Dateiname-Stamm: zwei unabhaengige Schreibwege (oder zwei Laeufe) haetten zwei Zeitstempel
// genommen und damit zwei verschiedene Staemme erzeugt. Genau daran wuerde ein Rueckfall auf
// "zweimal dasselbe bauen" auffliegen -- ohne dass eine Datei fehlen muesste.
TEST(A9S5Naht, BeideFormateErzeugenBeideAusgabenAusEinerMappe) {
    TempDir const    tmp;
    naht::MappenNaht n;
    n.oeffnen(tmp.path / "measurements.csv", {"csv", "xlsx"});

    ASSERT_TRUE(n.scharf()) << "Naht nicht scharf: " << n.diagnose();
    EXPECT_TRUE(n.wahl().xlsx);
    EXPECT_TRUE(n.wahl().csv);
    EXPECT_EQ(n.wahl().lage, naht::FormatLage::beide);
    EXPECT_EQ(n.wahl().ausgaben(), 2u);

    // BEIDE Ziele muessen in der Lauf-Ausgabe stehen -- eine ungenannte Datei ist eine
    // unbeobachtete Datei. (Nach schliessen() ist die Liste leer, deshalb HIER lesen.)
    std::string const ziele = n.ziele();
    EXPECT_NE(ziele.find(".xlsx"), std::string::npos) << "ziele() nennt die xlsx nicht: " << ziele;
    EXPECT_NE(ziele.find("__S001.csv"), std::string::npos) << "ziele() nennt die csv nicht: " << ziele;

    n.kopf_aus_csv(kKopf);
    n.zeile_aus_csv(kZeile1);
    n.zeile_aus_csv(kZeile2);
    EXPECT_EQ(n.zeilen(), 2u) << "Eine Zeile bleibt EINE Zeile der Mappe, auch wenn sie in zwei Dateien faellt.";
    EXPECT_EQ(n.feld_abweichungen(), 0u);
    ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << n.diagnose();

    // MENGE: genau eine xlsx UND genau ein flaches Sheet-CSV -- nicht das eine STATT des anderen.
    auto const xlsx_dateien = dateien_mit_endung(tmp.path, ".xlsx");
    auto const sheets       = dateien_mit_endung(tmp.path, "__S001.csv");
    ASSERT_EQ(xlsx_dateien.size(), 1u) << "Die xlsx-Ausgabe fehlt.";
    ASSERT_EQ(sheets.size(), 1u) << "Die csv-Ausgabe fehlt.";
    ASSERT_EQ(dateien_mit_endung(tmp.path, "__INFO.csv").size(), 1u) << "Das INFO-Blatt der csv-Ausgabe fehlt.";
    EXPECT_TRUE(dateien_mit_endung(tmp.path, ".xlsx.tmp").empty()) << "Kein tmp-Rest der xlsx-Ausgabe.";
    EXPECT_TRUE(dateien_mit_endung(tmp.path, ".csv.tmp").empty()) << "Kein tmp-Rest der csv-Ausgabe.";

    // DIE EIGENTLICHE AUSSAGE: EIN Stamm traegt beide Ausgaben.
    std::string const kXlsxSuffix = ".xlsx";
    std::string const kCsvSuffix  = "__S001.csv";
    std::string const stamm_xlsx  = xlsx_dateien.front().substr(0, xlsx_dateien.front().size() - kXlsxSuffix.size());
    std::string const stamm_csv   = sheets.front().substr(0, sheets.front().size() - kCsvSuffix.size());
    EXPECT_EQ(stamm_xlsx, stamm_csv) << "Verschiedene Staemme heissen: ZWEI Mappen statt einer. xlsx="
                                     << xlsx_dateien.front() << " csv=" << sheets.front();

    // WERT statt Anwesenheit, Seite xlsx: die Datei IST ein ZIP (Signatur 'PK\x03\x04').
    auto const inhalt = read_file(tmp.path / xlsx_dateien.front());
    ASSERT_GE(inhalt.size(), 4u) << "xlsx ist leer/abgeschnitten.";
    EXPECT_EQ(static_cast<unsigned char>(inhalt[0]), 0x50u);
    EXPECT_EQ(static_cast<unsigned char>(inhalt[1]), 0x4Bu);
    EXPECT_EQ(static_cast<unsigned char>(inhalt[2]), 0x03u);
    EXPECT_EQ(static_cast<unsigned char>(inhalt[3]), 0x04u);

    // WERT statt Anwesenheit, Seite csv: byte-genau dieselben Zeilen -- inklusive der LEEREN Zelle.
    std::string const erwartet = "binary_id;setting;n_ops\nb1;s1;100\nb2;;200\n";
    EXPECT_EQ(read_file(tmp.path / sheets.front()), erwartet);
}

// GEGENPROBE zur Ausgabe-Zahl: schliessen() ohne je geoeffnete Mappe bleibt false. Ohne diesen Fall
// koennte scharf() konstant true zurueckgeben und alle Zusicherungen oben blieben gruen.
TEST(A9S5Naht, SchliessenOhneGeoeffneteMappeIstFalse) {
    naht::MappenNaht n; // nie geoeffnet
    EXPECT_FALSE(n.scharf());
    EXPECT_FALSE(n.mappe_lebt());
    EXPECT_EQ(n.ziele(), "") << "Ohne Ausgabe gibt es keinen Zielnamen.";
    EXPECT_FALSE(n.schliessen(lab::MaschinenSysinfo{}, {}, {}));
}

// =================================================================================================
// D. DIE ERZEUGUNG IST BEDINGUNGSLOS -- W0b (2026-08-10)
// =================================================================================================
//
// DER OWNER-KERN, den dieser Abschnitt pruefen soll (09.08. 16:31, woertlich): "die csv wird doch
// aus der xlsx gebildet, IMMER. es wird nur entweder xlsx oder csv oder BEIDE auf Platte gesichert,
// was auf dem RAM liegt ist etwas voellig anderes."
//
// Bis W0b steuerte EIN Bit (wahl_.xlsx) beides: bei einem reinen csv-Profil wurde das
// xlsx-Mappen-Objekt GAR NICHT gebaut -- die Erzeugung hing an der Persistenz-Wahl. Die Tests unten
// trennen die beiden Fragen und pruefen die Erzeugung dort, wo sie stattfindet: IM SPEICHER, vor
// und unabhaengig von jedem Datei-Schreibvorgang.
//
// T-3 (Nenner FREMD): die Grundgesamtheit N ist NICHT die Zahl, die der Prueflung meldet. Sie wird
// unten aus dem Eingangs-CSV-Blob gezaehlt -- mit einer eigenen Schleife ueber '\n', die von
// MappenNaht nichts weiss. Weichen beide voneinander ab, faellt der Test, nicht der Nenner.

namespace {

/// Der Eingang, gegen den gerechnet wird: EIN Kopf + `n` Datenzeilen, jede mit einer LEEREN Zelle
/// in der Mitte (damit die Spaltentreue mitgeprueft ist). Von Hand gebaut, nicht vom Prueflung.
std::string eingangs_csv(std::size_t n) {
    std::string s;
    for (std::size_t i = 0; i < n; ++i) s += "b" + std::to_string(i) + ";;" + std::to_string(100 + i) + "\n";
    return s;
}

/// UNABHAENGIGES ORAKEL fuer die Grundgesamtheit: zaehlt Datenzeilen im Eingang, ohne den Prueflung
/// zu fragen. Genau das verlangt T-3 -- die Zahl darf nicht aus derselben Quelle kommen wie die
/// gepruefte Aussage.
std::size_t datenzeilen_im_eingang(std::string const& blob) {
    std::size_t n = 0;
    for (char const c : blob)
        if (c == '\n') ++n;
    return n;
}

/// Zeilen einer Datei auf der PLATTE (Gegenstand, nicht Ankuendigung).
std::size_t zeilen_in_datei(std::filesystem::path const& p) { return datenzeilen_im_eingang(read_file(p)); }

} // namespace

// DIE ABNAHME. Ein Lauf, der NUR csv persistiert, muss die xlsx-Erzeugung vollstaendig durchlaufen
// haben. Geprueft wird die Mappe IM SPEICHER -- vor schliessen(), also bevor irgendetwas auf der
// Platte steht -- und zwar mit dem Nenner, den der Auftrag verlangt: wieviele Blaetter, wieviele
// Zeilen je Blatt, gegen die Eingangs-CSV gerechnet.
//
// ROT VOR W0b: oeffnen() rief oeffne_eine(..., xlsx) nur `if (wahl_.xlsx)`; bei {"csv"} entstand gar
// keine Mappe. blatt_zahl() waere 0, zeilen_im_blatt(0) waere 0 -- gegen 7 erwartete.
TEST(A9S5Bestand, CsvOnlyLaeuftDieXlsxErzeugungVollstaendigDurch) {
    TempDir const     tmp;
    std::string const eingang = eingangs_csv(7);
    std::size_t const N       = datenzeilen_im_eingang(eingang); // FREMDER Nenner: 7
    ASSERT_EQ(N, 7u) << "Das Orakel selbst muss stimmen, sonst misst der Test nichts.";

    naht::MappenNaht n;
    n.oeffnen(tmp.path / "measurements.csv", {"csv"});

    // Die Mappe steht -- OBWOHL nur csv persistiert wird. Das ist der Kern des Owner-KERNs.
    ASSERT_TRUE(n.mappe_lebt()) << "Die xlsx-Mappe entsteht IMMER im Speicher. diagnose: " << n.diagnose();
    ASSERT_TRUE(n.scharf()) << n.diagnose();
    EXPECT_FALSE(n.wahl().xlsx) << "PERSISTENZ: keine xlsx auf die Platte -- das ist die andere Frage.";
    EXPECT_TRUE(n.wahl().csv);

    n.kopf_aus_csv(kKopf);
    n.blob_aus_csv(eingang);

    // ===== DER BESTAND IM SPEICHER, VOR jedem Datei-Schreibvorgang =====
    EXPECT_EQ(n.blatt_zahl(), 1u) << "Die Mappe fuehrt EIN Blatt (ein konstanter SheetSchluessel).";
    EXPECT_EQ(n.zeilen_im_blatt(0), N) << "Die Mappe muss ALLE " << N << " Eingangszeilen angenommen haben.";
    EXPECT_EQ(n.angeboten(), N) << "Grundgesamtheit: was hereingereicht wurde.";
    EXPECT_EQ(n.verworfen(), 0u) << "Keine Zeile darf unterwegs verlorengehen.";
    EXPECT_EQ(n.zeilen(), N);
    EXPECT_EQ(n.kopf_spalten(), 3u);
    EXPECT_EQ(n.feld_abweichungen(), 0u);

    // Der Nenner gehoert in die AUSGABE, nicht nur in den Kopf des Lesers (V-1).
    std::string const b = n.bestand();
    EXPECT_NE(b.find("mappe=xlsx-im-speicher"), std::string::npos) << "bestand() war: " << b;
    EXPECT_NE(b.find("blaetter=1"), std::string::npos) << "bestand() war: " << b;
    EXPECT_NE(b.find("S1:7/7 angeboten"), std::string::npos)
        << "bestand() muss angenommene GEGEN angebotene Zeilen nennen, war: " << b;
    EXPECT_NE(b.find("persistenz=csv"), std::string::npos)
        << "bestand() muss die Persistenz-Wahl von der Erzeugung TRENNEN, war: " << b;

    ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << n.diagnose();

    // ===== DIE PLATTE: die Mappe LIEF, sie LANDET nur nicht =====
    EXPECT_TRUE(dateien_mit_endung(tmp.path, ".xlsx").empty()) << "csv-only darf KEINE xlsx hinterlassen.";
    EXPECT_TRUE(dateien_mit_endung(tmp.path, ".xlsx.tmp").empty())
        << "Eine nicht persistierte Mappe darf auch keinen tmp-Rest hinterlassen -- sonst waere die "
           "Entkopplung nur halb.";
    auto const sheets = dateien_mit_endung(tmp.path, "__S001.csv");
    ASSERT_EQ(sheets.size(), 1u);
    // Der Gegenstand statt der Ankuendigung: die csv traegt genau die Zeilen, die die Mappe annahm.
    EXPECT_EQ(zeilen_in_datei(tmp.path / sheets.front()), n.zeilen_im_blatt(0) + 1)
        << "Die csv ist das KIND der Mappe -- sie kann nicht mehr und nicht weniger Zeilen tragen "
           "als die Mappe angenommen hat (+1 fuer den Kopf).";
    EXPECT_EQ(zeilen_in_datei(tmp.path / sheets.front()), N + 1) << "Und dieselbe Zahl gegen den FREMDEN Nenner.";
}

// T-4 GEGENEINGANG zur Abnahme oben: DERSELBE Eingang, die ANDERE Persistenz-Wahl. Der Bestand im
// Speicher muss IDENTISCH sein -- daran faellt jede Fassung, in der ein Bit beide Fragen steuert.
TEST(A9S5Bestand, DerBestandHaengtNICHTAnDerPersistenzWahl) {
    std::string const eingang = eingangs_csv(7);
    std::size_t const N       = datenzeilen_im_eingang(eingang);

    // Drei Persistenz-Faelle, EIN erwarteter Bestand. Nenner: 3 von 3 Faellen (xlsx allein, csv
    // allein, BEIDE) -- die vollstaendige Menge, die der Owner-Entscheid 09.08. zulaesst.
    std::vector<std::vector<std::string>> const faelle{{}, {"csv"}, {"csv", "xlsx"}};
    std::size_t                                 geprueft = 0;
    for (auto const& methoden : faelle) {
        TempDir const    tmp;
        naht::MappenNaht n;
        n.oeffnen(tmp.path / "measurements.csv", methoden);
        ASSERT_TRUE(n.mappe_lebt()) << "Fall " << geprueft << ": die Mappe fehlt. " << n.diagnose();

        n.kopf_aus_csv(kKopf);
        n.blob_aus_csv(eingang);

        EXPECT_EQ(n.blatt_zahl(), 1u) << "Fall " << geprueft;
        EXPECT_EQ(n.zeilen_im_blatt(0), N)
            << "Fall " << geprueft << ": der Bestand im Speicher darf sich mit der Persistenz-Wahl NICHT aendern.";
        EXPECT_EQ(n.verworfen(), 0u) << "Fall " << geprueft;
        ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << "Fall " << geprueft << ": " << n.diagnose();

        // Und die Platte trennt die Faelle sehr wohl -- sonst waere die Trennung folgenlos.
        std::size_t const xlsx_auf_platte = dateien_mit_endung(tmp.path, ".xlsx").size();
        std::size_t const csv_auf_platte  = dateien_mit_endung(tmp.path, "__S001.csv").size();
        EXPECT_EQ(xlsx_auf_platte, n.wahl().xlsx ? 1u : 0u) << "Fall " << geprueft;
        EXPECT_EQ(csv_auf_platte, n.wahl().csv ? 1u : 0u) << "Fall " << geprueft;
        ++geprueft;
    }
    EXPECT_EQ(geprueft, 3u) << "Alle drei Persistenz-Faelle muessen gelaufen sein, nicht nur der erste.";
}

// T-4 zur ZAHL: ein Lauf ohne Datenzeilen ergibt EIN Blatt und NULL Zeilen. Ohne diesen Gegeneingang
// duerfte zeilen_im_blatt() konstant 7 liefern und die Abnahme oben bliebe gruen.
TEST(A9S5Bestand, NullDatenzeilenErgebenEinBlattUndNullZeilen) {
    TempDir const    tmp;
    naht::MappenNaht n;
    n.oeffnen(tmp.path / "measurements.csv", {"csv"});
    ASSERT_TRUE(n.mappe_lebt()) << n.diagnose();

    n.kopf_aus_csv(kKopf); // NUR der Kopf, keine Datenzeile

    EXPECT_EQ(n.blatt_zahl(), 1u) << "Das Blatt entsteht mit der Mappe, nicht mit der ersten Zeile.";
    EXPECT_EQ(n.zeilen_im_blatt(0), 0u) << "Der Kopf ist keine Datenzeile.";
    EXPECT_EQ(n.angeboten(), 0u);
    EXPECT_NE(n.bestand().find("S1:0/0 angeboten"), std::string::npos) << "bestand() war: " << n.bestand();
    EXPECT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << n.diagnose();
}

// FAIL-CLOSED / die Kind-Regel an ihrer haertesten Stelle: laesst sich das Ziel nicht herstellen,
// entsteht KEINE Mappe -- und dann entsteht auch KEINE csv, obwohl csv verlangt war. Ein
// csv-Ergebnis ohne funktionierenden xlsx-Weg waere ein Widerspruch, kein Sparmodus.
//
// Der Eingang ist real und nicht gestellt: das Elternverzeichnis der Ziel-CSV ist eine DATEI.
// create_directories() scheitert daran; frueher wurde sein error_code verworfen.
TEST(A9S5Naht, OhneHerstellbaresZielEntstehtWederMappeNochCsv) {
    std::vector<std::vector<std::string>> const faelle{{}, {"csv"}, {"csv", "xlsx"}};
    std::size_t                                 geprueft = 0;
    for (auto const& methoden : faelle) {
        TempDir const tmp;
        auto const    blocker = tmp.path / "blocker"; // eine DATEI, kein Verzeichnis
        { std::ofstream(blocker) << "x"; }
        ASSERT_TRUE(std::filesystem::is_regular_file(blocker));

        naht::MappenNaht n;
        n.oeffnen(blocker / "measurements.csv", methoden);

        EXPECT_FALSE(n.mappe_lebt()) << "Fall " << geprueft << ": ohne herstellbares Ziel darf keine Mappe entstehen.";
        EXPECT_FALSE(n.scharf()) << "Fall " << geprueft;
        EXPECT_EQ(n.ziele(), "") << "Fall " << geprueft
                                 << ": ein genanntes Ziel, das nie entsteht, ist ein Stellvertreter.";
        EXPECT_NE(n.diagnose().find("Zielverzeichnis nicht herstellbar"), std::string::npos)
            << "Fall " << geprueft << ": der Grund muss BENANNT sein, war: " << n.diagnose();
        EXPECT_FALSE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << "Fall " << geprueft;

        // Der Gegenstand: im Ordner liegt NUR der Blocker -- keine Mappe, kein Kind, kein tmp.
        std::size_t dateien = 0;
        for (auto const& e : std::filesystem::directory_iterator(tmp.path))
            if (e.is_regular_file()) ++dateien;
        EXPECT_EQ(dateien, 1u) << "Fall " << geprueft << ": es darf NICHTS ausser dem Blocker entstanden sein.";
        ++geprueft;
    }
    EXPECT_EQ(geprueft, 3u);
}

// =================================================================================================
// E. DAS ORAKEL AM INHALT DER MAPPE -- W0b-Nachbesserung (2026-08-10)
// =================================================================================================
//
// WARUM DIESER ABSCHNITT EXISTIERT. Ein adversarischer Pruefer hat am 10.08. den Aufruf
// `stamm_blatt_->zeile(felder)` in MappenNaht::schreibe() zu einem No-op gemacht -- ohne dass er
// wirft (Koeder M3s). Ergebnis: sieben von sieben Messzeilen verschwanden aus der Mappe, und KEINE
// Schicht klapperte. Alle 21 Tests der Abschnitte A-D blieben gruen.
//
// WARUM SIE ALLE GRUEN BLIEBEN -- und das ist der Kern, nicht ein Versehen:
//   * bestand()/zeilen_im_blatt() lesen zeilen_je_blatt_. Dieser Zaehler waechst in schreibe() im
//     Erfolgspfad UNTER dem Aufruf, und "Erfolg" heisst dort nur "es wurde nicht geworfen". Ein
//     No-op wirft nicht. Der Zaehler bewegt sich also MIT dem Defekt, nicht gegen ihn.
//   * Die csv rettet NICHT. Sie ist an dieser Stelle kein unabhaengiger Zeuge, sondern haengt am
//     KIND-Zweig DERSELBEN Funktion: schreibe() reicht dieselben Felder weiter. Eine Mutation auf
//     dem Weg Naht->Mappe bewegt Zaehler UND csv gemeinsam. Das Messgeraet stand beim Nachbarn.
//   * A9S5Bestand.CsvOnlyLaeuftDieXlsxErzeugungVollstaendigDurch prueft zwar csv-Zeilen gegen
//     zeilen_im_blatt(0)+1 -- aber BEIDE Seiten dieses Vergleichs liegen auf derselben Seite der
//     Mutation. Unter M3s bleiben sie miteinander konsistent (beide 7) und der Test bleibt gruen.
//     Ein fremder Nenner ist erst dann fremd, wenn er die MUTIERTE STELLE nicht durchlaufen hat.
//
// WAS HIER ANDERS IST (T-3/V-7 ernst genommen): die Grundgesamtheit kommt aus der ERZEUGTEN DATEI,
// nicht aus dem Prueflung. Eine .xlsx IST ein ZIP; das erste angelegte Blatt liegt als
// xl/worksheets/sheet1.xml. Darin fuehrt libxlsxwriter selbst zwei Zahlen, die MappenNaht nie zu
// Gesicht bekommt:
//   (a) <dimension ref="A1:C8"/> -- die Ausdehnung, die die Bibliothek aus den tatsaechlich
//       erfolgten worksheet_write_*-Aufrufen mitschreibt (worksheet.c:2050 _worksheet_write_dimension,
//       gespeist aus _check_dimensions, worksheet.c:1260).
//   (b) die Zahl der <row>-Elemente -- die tatsaechlich materialisierten Zeilen.
// Unter M3s erreicht KEIN einziger Datenzeilen-Aufruf die Bibliothek: (a) bleibt bei "A1:C1" und
// (b) bei 1 (nur der Kopf). Beide Zahlen fallen, der Zaehler nicht -- genau das ist der Biss.
//
// (a) UND (b) SIND NICHT DASSELBE, und das ist am Objekt nachgemessen: _check_dimensions() laeuft in
// worksheet_write_string() VOR der Laengenpruefung (worksheet.c:7953 gegen :7957) und speichert die
// Ausdehnung auch dann, wenn der Schreibvorgang danach mit einem Fehler zurueckkommt. `dimension`
// ist deshalb eine OBERGRENZE, kein Zeilenzaehler -- A9S5Ablehnung unten pinnt genau diesen
// Unterschied fest. Wer nur (a) pruefte, haette ein Orakel, das eine fehlgeschlagene Zeile mitzaehlt.
//
// EINE WEITERE STILLE VENDOR-EIGENSCHAFT, hier nur benannt, weil sie die Sollwerte unten erklaert:
// worksheet_write_string() behandelt eine LEERE Zeichenkette ohne Format als "nichts zu tun" und
// schreibt gar keine Zelle (worksheet.c:7944-7951). Die leere Mittelspalte der Eingangszeilen taucht
// in sheet1.xml also nicht als Zelle auf. Auf `dimension` wirkt sich das nicht aus, weil Spalte C
// belegt ist -- die Ausdehnung bleibt A..C.

namespace {

// -----------------------------------------------------------------------------
// Minimaler ZIP-Leser -- identisches Muster zu test_a9s3_xlsx_ergebnis_writer.cpp und
// test_a9_xlsx_vendor_rauchtest.cpp: Zentral-Directory statt lokaler Kopfsaetze, vendoriertes
// zlib-Inflate. Kein Python, kein externes Werkzeug.
//
// T-6-VERMERK: das ist die VIERTE woertliche Kopie dieses Lesers im Testbaum (die anderen drei siehe
// oben). Ein gemeinsamer Test-Hilfsheader waere die richtige Aufraeumarbeit -- sie beruehrt aber drei
// fremde Testdateien und gehoert damit nicht in dieses Paket. Ausdruecklich als offen benannt, nicht
// stillschweigend uebergangen.
// -----------------------------------------------------------------------------

std::uint16_t le16(std::string const& b, std::size_t off) {
    return static_cast<std::uint16_t>(static_cast<unsigned char>(b[off]) |
                                      (static_cast<unsigned char>(b[off + 1]) << 8));
}
std::uint32_t le32(std::string const& b, std::size_t off) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(b[off])) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b[off + 1])) << 8) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b[off + 2])) << 16) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b[off + 3])) << 24);
}
std::string inflate_raw(char const* data, std::size_t csize, std::size_t usize) {
    std::string out(usize, '\0');
    if (usize == 0) return out;
    z_stream s{};
    if (inflateInit2(&s, -MAX_WBITS) != Z_OK) return {};
    s.next_in    = reinterpret_cast<Bytef*>(const_cast<char*>(data));
    s.avail_in   = static_cast<uInt>(csize);
    s.next_out   = reinterpret_cast<Bytef*>(out.data());
    s.avail_out  = static_cast<uInt>(usize);
    int const rc = inflate(&s, Z_FINISH);
    inflateEnd(&s);
    if (rc != Z_STREAM_END) return {};
    out.resize(s.total_out);
    return out;
}
std::string zip_entry(std::string const& zip, std::string const& name) {
    constexpr std::size_t kEocdMin = 22;
    if (zip.size() < kEocdMin) return {};
    std::size_t eocd = std::string::npos;
    for (std::size_t i = zip.size() - kEocdMin + 1; i-- > 0;) {
        if (le32(zip, i) == 0x06054b50u) {
            eocd = i;
            break;
        }
    }
    if (eocd == std::string::npos) return {};
    std::uint16_t const anzahl   = le16(zip, eocd + 10);
    std::uint32_t const cd_start = le32(zip, eocd + 16);
    if (cd_start >= zip.size()) return {};
    std::size_t pos = cd_start;
    for (std::uint16_t i = 0; i < anzahl; ++i) {
        if (pos + 46 > zip.size() || le32(zip, pos) != 0x02014b50u) return {};
        std::uint16_t const methode   = le16(zip, pos + 10);
        std::uint32_t const csize     = le32(zip, pos + 20);
        std::uint32_t const usize     = le32(zip, pos + 24);
        std::uint16_t const namelen   = le16(zip, pos + 28);
        std::uint16_t const extralen  = le16(zip, pos + 30);
        std::uint16_t const kommentar = le16(zip, pos + 32);
        std::uint32_t const lokal_off = le32(zip, pos + 42);
        std::string const   eintrag   = zip.substr(pos + 46, namelen);
        if (eintrag == name) {
            if (csize == 0xffffffffu || usize == 0xffffffffu) return {};
            if (lokal_off + 30 > zip.size() || le32(zip, lokal_off) != 0x04034b50u) return {};
            std::size_t const daten = lokal_off + 30 + le16(zip, lokal_off + 26) + le16(zip, lokal_off + 28);
            if (daten + csize > zip.size()) return {};
            if (methode == 0) return zip.substr(daten, usize);
            if (methode != 8) return {};
            return inflate_raw(zip.data() + daten, csize, usize);
        }
        pos += 46 + namelen + extralen + kommentar;
    }
    return {};
}

/// Das Datenblatt der von MappenNaht erzeugten Mappe, als XML.
///
/// WARUM sheet1.xml: MappenNaht::oeffnen() legt das Datenblatt per stamm_->blatt(SheetSchluessel{})
/// an, BEVOR schliessen() das INFO-Blatt anhaengt. Das Datenblatt ist damit das ZUERST angelegte
/// Sheet -- dieselbe Zuordnung, die test_a9s3_xlsx_ergebnis_writer.cpp bereits nutzt ("sheet1.xml =
/// S001, erste angelegte"). Der Test unten pruefte das nicht nur, er BELEGT es: er sucht in
/// sheet1.xml nach Mess-Zellen, und das INFO-Blatt traegt andere.
std::string datenblatt_xml(std::filesystem::path const& xlsx) {
    return zip_entry(read_file(xlsx), "xl/worksheets/sheet1.xml");
}

/// Die Ausdehnung, die libxlsxwriter selbst mitgeschrieben hat: <dimension ref="A1:C8"/> -> "A1:C8".
/// Leerer String = kein dimension-Element gefunden (dann faellt der Test, statt still 0 zu melden --
/// FAIL-CLOSED).
std::string dimension_ref(std::string const& sheet_xml) {
    constexpr std::string_view kMarke = "<dimension ref=\"";
    auto const                 start  = sheet_xml.find(kMarke);
    if (start == std::string::npos) return {};
    auto const von = start + kMarke.size();
    auto const bis = sheet_xml.find('"', von);
    if (bis == std::string::npos) return {};
    return sheet_xml.substr(von, bis - von);
}

/// Die Zahl der tatsaechlich materialisierten <row>-Elemente im Blatt -- die haertere der beiden
/// Vendor-Zahlen (s. Abschnitts-Kopf: dimension ist nur eine Obergrenze).
std::size_t zeilen_im_blatt_xml(std::string const& sheet_xml) {
    constexpr std::string_view kMarke = "<row ";
    std::size_t                n      = 0;
    std::size_t                pos    = 0;
    while ((pos = sheet_xml.find(kMarke, pos)) != std::string::npos) {
        ++n;
        pos += kMarke.size();
    }
    return n;
}

/// Der Sollwert fuer dimension bei `datenzeilen` Datenzeilen ueber 3 Spalten -- aus dem FREMDEN
/// Nenner gerechnet, nicht vom Prueflung erfragt. +1 fuer die Kopfzeile.
std::string soll_dimension(std::size_t datenzeilen) { return "A1:C" + std::to_string(datenzeilen + 1); }

} // namespace

// DIE ABNAHME DIESES POSTENS. Der Zaehler der Naht wird gegen den INHALT der Mappe gehalten -- gegen
// eine Quelle, die die mutierte Stelle nicht durchlaufen hat.
//
// BISS BELEGT (K13, Wegwerf-Mutation vom 10.08., protokolliert): mit `stamm_blatt_->zeile(felder);`
// -> `(void) felder;` in ergebnis_mappe_naht.hpp faellt dieser Test mit
//   dimension ist "A1:C1", soll "A1:C8"   und   <row>-Zahl ist 1, soll 8
// waehrend n.zeilen_im_blatt(0) unveraendert 7 meldet. Ohne die Mutation ist er gruen.
TEST(A9S5MappenInhalt, DieMappeTraegtDieZeilenWirklichNichtNurImZaehler) {
    TempDir const     tmp;
    std::string const eingang = eingangs_csv(7);
    std::size_t const N       = datenzeilen_im_eingang(eingang); // FREMDER Nenner: 7, aus dem Eingang
    ASSERT_EQ(N, 7u) << "Das Orakel selbst muss stimmen, sonst misst der Test nichts.";

    naht::MappenNaht n;
    // BEIDE Persistenzen: so liegt die Mappe als Datei vor UND ihr Kind daneben -- zwei Ausgaben
    // derselben Mappe, die sich gegenseitig bezeugen koennen.
    n.oeffnen(tmp.path / "measurements.csv", {"csv", "xlsx"});
    ASSERT_TRUE(n.scharf()) << n.diagnose();

    n.kopf_aus_csv(kKopf);
    n.blob_aus_csv(eingang);

    // WAS DIE NAHT BEHAUPTET -- vor dem Schliessen gelesen, danach ist das Objekt freigegeben.
    std::size_t const behauptet = n.zeilen_im_blatt(0);
    EXPECT_EQ(behauptet, N) << "Die Buchhaltung der Naht selbst.";
    // Die Herkunft der Zahl muss in der AUSGABE stehen -- sonst liest ein Mensch sie als
    // Inhalts-Aussage, die sie nicht ist (V-8).
    EXPECT_NE(n.bestand().find("quelle=naht-buchhaltung"), std::string::npos)
        << "bestand() muss seine Herkunft selbst ansagen, war: " << n.bestand();

    ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << n.diagnose();

    auto const xlsx_dateien = dateien_mit_endung(tmp.path, ".xlsx");
    ASSERT_EQ(xlsx_dateien.size(), 1u) << "Ohne Mappe auf der Platte gibt es nichts zu pruefen.";
    std::string const sheet = datenblatt_xml(tmp.path / xlsx_dateien.front());
    ASSERT_FALSE(sheet.empty()) << "xl/worksheets/sheet1.xml nicht lesbar -- FAIL-CLOSED, nicht gruen.";

    // ===== DAS ORAKEL: zwei Zahlen aus der DATEI, keine davon aus der Naht =====
    EXPECT_EQ(dimension_ref(sheet), soll_dimension(N))
        << "libxlsxwriter fuehrt die Ausdehnung selbst mit. Steht hier A1:C1, hat die Mappe nur den "
           "Kopf gesehen -- egal was der Zaehler meldet. sheet1.xml war: "
        << sheet;
    EXPECT_EQ(zeilen_im_blatt_xml(sheet), N + 1) << "Materialisierte <row>-Elemente: " << N << " Datenzeilen + 1 Kopf.";

    // ===== DIE EIGENTLICHE AUSSAGE DES POSTENS: Zaehler GEGEN Inhalt =====
    EXPECT_EQ(zeilen_im_blatt_xml(sheet), behauptet + 1)
        << "Die Naht meldete " << behauptet << " angenommene Zeilen. Die Mappe traegt sie nicht. "
        << "Genau diese Luecke liess Koeder M3s ueberleben.";

    // WERT statt Menge: die LETZTE Datenzeile muss mit ihrem Wert an ihrer Position stehen. Zeile 8
    // (Kopf + 7), Spalte C, Wert 100+6 -- als echte Zahlzelle, nicht als String-Verweis. Eine
    // Mutation, die nur die letzte Zeile verliert, faellt hier und nicht erst an der Menge.
    EXPECT_NE(sheet.find("<c r=\"C8\"><v>106</v></c>"), std::string::npos)
        << "Die letzte Datenzeile fehlt oder steht an der falschen Stelle. sheet1.xml war: " << sheet;

    // ===== STAMM GEGEN KIND: zwei Ausgaben DERSELBEN Mappe muessen sich decken =====
    // Das ist die zweite unabhaengige Achse: waeren es zwei Schreibwege statt Stamm und Kind,
    // koennten sie hier auseinanderlaufen, ohne dass eine Datei fehlt.
    auto const sheets = dateien_mit_endung(tmp.path, "__S001.csv");
    ASSERT_EQ(sheets.size(), 1u);
    EXPECT_EQ(zeilen_in_datei(tmp.path / sheets.front()), zeilen_im_blatt_xml(sheet))
        << "Die csv ist das KIND der Mappe -- sie kann nicht mehr Zeilen tragen als der Stamm.";
}

// T-4 GEGENEINGANG zum Orakel oben: das Orakel darf nicht konstant sein. Drei verschiedene
// Eingangsgroessen muessen DREI verschiedene Ausdehnungen erzeugen. Ohne diesen Fall duerfte
// dimension_ref() konstant "A1:C8" liefern und die Abnahme oben bliebe gruen.
TEST(A9S5MappenInhalt, DieAusdehnungFolgtDerEingangsgroesse) {
    std::vector<std::size_t> const groessen{0, 2, 7};
    std::vector<std::string>       gesehen;
    std::size_t                    geprueft = 0;
    for (std::size_t const g : groessen) {
        TempDir const     tmp;
        std::string const eingang = eingangs_csv(g);
        std::size_t const N       = datenzeilen_im_eingang(eingang); // FREMDER Nenner
        ASSERT_EQ(N, g) << "Fall " << geprueft << ": das Orakel selbst muss stimmen.";

        naht::MappenNaht n;
        n.oeffnen(tmp.path / "measurements.csv", {}); // Default = nur xlsx
        ASSERT_TRUE(n.scharf()) << "Fall " << geprueft << ": " << n.diagnose();
        n.kopf_aus_csv(kKopf);
        n.blob_aus_csv(eingang);
        ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << "Fall " << geprueft << ": " << n.diagnose();

        auto const xlsx_dateien = dateien_mit_endung(tmp.path, ".xlsx");
        ASSERT_EQ(xlsx_dateien.size(), 1u) << "Fall " << geprueft;
        std::string const sheet = datenblatt_xml(tmp.path / xlsx_dateien.front());
        ASSERT_FALSE(sheet.empty()) << "Fall " << geprueft << ": sheet1.xml nicht lesbar.";

        EXPECT_EQ(dimension_ref(sheet), soll_dimension(N)) << "Fall " << geprueft << " (N=" << N << ")";
        EXPECT_EQ(zeilen_im_blatt_xml(sheet), N + 1) << "Fall " << geprueft << " (N=" << N << ")";
        gesehen.push_back(dimension_ref(sheet));
        ++geprueft;
    }
    ASSERT_EQ(geprueft, 3u) << "Alle drei Groessen muessen gelaufen sein.";
    // Der eigentliche Gegeneingang: die drei Ausdehnungen sind PAARWEISE VERSCHIEDEN. Ein Orakel,
    // das immer dasselbe liefert, faellt genau hier.
    EXPECT_NE(gesehen[0], gesehen[1]) << "N=0 und N=2 duerfen nicht dieselbe Ausdehnung haben.";
    EXPECT_NE(gesehen[1], gesehen[2]) << "N=2 und N=7 duerfen nicht dieselbe Ausdehnung haben.";
    EXPECT_EQ(gesehen[0], "A1:C1") << "Nur der Kopf -- genau die Lage, die Koeder M3s vortaeuscht.";
}

// DER ABLEHNUNGSPFAD -- bis heute unbefahren. verworfen() stand im ganzen Testfile nur als
// EXPECT_EQ(..., 0u) (zwei Fundstellen: A9S5Bestand oben); kein Test trieb ihn je ueber null. Damit
// war der gesamte catch-Zweig von schreibe() ungedeckt, inklusive der Kernsemantik
// "legt der Stamm die Annahme nieder, faellt das Kind im selben Zug mit".
//
// DER AUSLOESER IST ECHT und stammt aus dem Vendor-Vertrag, nicht aus einem Test-Seam:
// worksheet.c:28 setzt LXW_STR_MAX auf 32767; worksheet.c:7957 lehnt laengere Zeichenketten mit
// LXW_ERROR_MAX_STRING_LENGTH_EXCEEDED ab; xlsx_ergebnis_writer.cpp:95-99 macht daraus einen
// ErgebnisSchreibFehler{ZipFehler}, der in schreibe() im catch landet. Der andere denkbare Ausloeser
// -- das Zeilenlimit kXlsxZeilenlimit -- braeuchte eine Million echte Zeilen und waere kein
// staerkerer Beleg, nur ein langsamerer Test (dieselbe Begruendung fuehrt test_a9s3_xlsx_*).
TEST(A9S5Ablehnung, LegtDerStammDieAnnahmeNiederFaelltDasKindMitUndVerworfeneZeilenWerdenGezaehlt) {
    TempDir const    tmp;
    naht::MappenNaht n;
    n.oeffnen(tmp.path / "measurements.csv", {"csv", "xlsx"});
    ASSERT_TRUE(n.scharf()) << n.diagnose();

    n.kopf_aus_csv(kKopf);
    n.zeile_aus_csv("b0;;100\n"); // wird angenommen

    // Spalte A, damit der Fehlschlag VOR jeder anderen Zelle dieser Zeile faellt.
    std::string const zu_lang(32768, 'a'); // 32768 > LXW_STR_MAX (32767)
    n.zeile_aus_csv(zu_lang + ";;200\n");  // DIESE Zeile bringt den Stamm zu Fall
    n.zeile_aus_csv("b2;;300\n");          // und DIESE trifft auf eine Mappe, die nicht mehr annimmt

    // ===== DIE BUCHHALTUNG: der Zaehler, den bisher kein Test ueber null getrieben hat =====
    EXPECT_FALSE(n.scharf()) << "Nach dem Schreibfehler darf die Naht nicht mehr scharf sein.";
    EXPECT_TRUE(n.mappe_lebt()) << "Das Mappen-OBJEKT lebt weiter -- nur die Annahme ist niedergelegt.";
    EXPECT_EQ(n.angeboten(), 3u) << "Grundgesamtheit: drei Datenzeilen wurden hereingereicht.";
    EXPECT_EQ(n.zeilen_im_blatt(0), 1u) << "Genau die erste Zeile wurde angenommen.";
    EXPECT_EQ(n.verworfen(), 2u) << "BEIDE muessen zaehlen: die Zeile, die den Fehler ausloeste, UND jede danach. Eine "
                                    "verworfene Zeile, die still verschwindet, macht die csv unvollstaendig OHNE Spur.";
    EXPECT_NE(n.bestand().find("S1:1/3 angeboten verworfen=2"), std::string::npos)
        << "Der Nenner gehoert in die AUSGABE, auch im Fehlerfall. bestand() war: " << n.bestand();
    EXPECT_NE(n.diagnose().find("die MAPPE nahm nicht mehr an"), std::string::npos)
        << "Der Grund muss BENANNT sein, war: " << n.diagnose();

    ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << n.diagnose();

    // ===== DER GEGENSTAND: das Kind auf der Platte =====
    // Das ist die pruefbare Fassung von "das Kind faellt MIT". Die csv darf NUR Kopf + b0 tragen:
    // weder die ausloesende Zeile noch die danach duerfen sie erreicht haben.
    auto const sheets = dateien_mit_endung(tmp.path, "__S001.csv");
    ASSERT_EQ(sheets.size(), 1u) << "Die csv-Ausgabe fehlt.";
    std::string const csv_inhalt = read_file(tmp.path / sheets.front());
    EXPECT_EQ(zeilen_in_datei(tmp.path / sheets.front()), 2u)
        << "Kopf + genau die eine angenommene Zeile. csv war: " << csv_inhalt;
    EXPECT_NE(csv_inhalt.find("b0;;100"), std::string::npos) << "Die angenommene Zeile fehlt: " << csv_inhalt;
    EXPECT_EQ(csv_inhalt.find("b2;;300"), std::string::npos)
        << "Eine Zeile, die der Stamm ABGELEHNT hat, darf die csv NIE erreichen -- sonst waeren es "
           "Geschwister statt Stamm und Kind. csv war: "
        << csv_inhalt;
    EXPECT_EQ(csv_inhalt.find(zu_lang), std::string::npos)
        << "Auch die ausloesende Zeile selbst darf nicht im Kind stehen.";

    // ===== UND DIE MAPPE SELBST: dimension ist eine OBERGRENZE, kein Zeilenzaehler =====
    // Nachgemessen am Vendor: _check_dimensions() laeuft in worksheet_write_string() VOR der
    // Laengenpruefung (worksheet.c:7953 gegen :7957) und speichert die Ausdehnung auch dann, wenn
    // der Aufruf danach mit einem Fehler zurueckkommt. Die abgelehnte Zeile 3 weitet `dimension`
    // also auf A1:C3 -- obwohl nur ZWEI <row>-Elemente entstehen. Dieser Unterschied wird hier
    // festgenagelt, damit niemand `dimension` spaeter fuer eine Zeilenzahl haelt.
    auto const xlsx_dateien = dateien_mit_endung(tmp.path, ".xlsx");
    ASSERT_EQ(xlsx_dateien.size(), 1u);
    std::string const sheet = datenblatt_xml(tmp.path / xlsx_dateien.front());
    ASSERT_FALSE(sheet.empty()) << "sheet1.xml nicht lesbar -- FAIL-CLOSED.";
    EXPECT_EQ(zeilen_im_blatt_xml(sheet), 2u) << "Kopf + die eine angenommene Zeile. sheet1.xml war: " << sheet;
    EXPECT_EQ(dimension_ref(sheet), "A1:C3")
        << "Erwartet ist die OBERGRENZE inklusive der abgelehnten Zeile -- weicht das ab, hat der "
           "Vendor sein Verhalten geaendert und der Kommentar darueber ist falsch geworden.";
}
