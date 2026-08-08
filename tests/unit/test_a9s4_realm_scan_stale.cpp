// tests/unit/test_a9s4_realm_scan_stale.cpp -- A9-S4: die .stale-Bissprobe fuer den
// --realm-root-Verzeichnis-Scan (tools/mess_report/realm_scan.hpp).
//
// K13-DOKTRIN ("der Koeder muss erst beissen"): Test 1 prueft den Codepfad, den eine
// verbreitete, VERSUCHERISCHE Vereinfachung brechen wuerde -- ein Ersetzen der ECHTEN
// Endungs-Pruefung (`str.ends_with(".stale")`) durch eine bequeme SUBSTRING-Pruefung
// (`str.find(".result.csv") != npos`) fuer die Einschluss-Entscheidung. Fuer eine Datei namens
// "<stamm>.result.csv.stale" liefert die Substring-Pruefung TRUE (der Teilstring ".result.csv"
// KOMMT VOR), obwohl die Datei NICHT auf ".result.csv" ENDET -- Test 1 belegt diesen Unterschied
// direkt am Objekt, bevor Test 2 die reale Scan-Funktion gegen dieselbe Datei prueft. Erst der
// Koeder gegen das Werkzeug, dann gegen die Wache.

#include "../../tools/mess_report/realm_scan.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>

namespace mr = ::comdare::cache_engine::tools::mess_report;

namespace {

/// RAII-Temp-Verzeichnis -- dasselbe Muster wie tests/unit/test_a9s3_ergebnis_mappe_csv.cpp::TempDir
/// (Zaehler + Zeitstempel statt PID, automatisches remove_all, kein Test-Rueckstand im Arbeitsbaum).
struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("a9s4_realm_scan_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
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

} // namespace

// -- Test 1: der Koeder gegen das WERKZEUG (die nackte Zeichenketten-Pruefung, nicht der Scan) --
TEST(A9S4RealmScanStale, EndsWithVsContains_DerUnterschiedDerDenBissAusmacht) {
    std::string const stale_name = "eytzinger_0_deadbeef12345678.result.csv.stale";

    // Die ECHTE Wache: endet die Datei WIRKLICH auf ".stale"?
    EXPECT_TRUE(mr::endet_mit(stale_name, mr::kStaleSuffix));

    // Der VERSUCHERISCHE Fehlgriff, gegen den Test 1 beisst: eine Substring-Pruefung auf
    // ".result.csv" haette die Datei FAELSCHLICH als "Ergebnis-CSV" durchgewunken (der Teilstring
    // steckt drin), obwohl die Datei NICHT auf ".result.csv" ENDET. Waere die Einschluss-Logik in
    // realm_scan.hpp jemals auf `find(...) != npos` vereinfacht, wuerde GENAU DIESE Zeile den
    // Unterschied zwischen "koennte theoretisch matchen" und "matcht wirklich" beweisen.
    bool const naive_substring_matcht = stale_name.find(mr::kResultCsvSuffix) != std::string::npos;
    EXPECT_TRUE(naive_substring_matcht) << "die Faessern-Falle: der Teilstring ist da";
    bool const echte_endung_matcht = mr::endet_mit(stale_name, mr::kResultCsvSuffix);
    EXPECT_FALSE(echte_endung_matcht) << "die Datei endet NICHT auf .result.csv -- sie endet auf .stale";
}

// -- Test 2: die Wache selbst (die reale scanne_realm_root()-Funktion, Integrationsebene) --
TEST(A9S4RealmScanStale, ScanSchliesstStaleAusUndZaehltEs) {
    TempDir const tmp;
    schreibe(tmp.path / "per_binary" / "a_0_1111.result.csv", "binary_id;x\na;1\n");
    schreibe(tmp.path / "per_binary" / "b_0_2222.result.csv", "binary_id;x\nb;1\n");
    schreibe(tmp.path / "per_binary" / "c_0_3333.result.csv.stale", "binary_id;x\nc;ALT-VERWORFEN\n");
    schreibe(tmp.path / "per_binary" / "notiz.txt", "kein Mess-Ergebnis");

    auto const erg = mr::scanne_realm_root(tmp.path);

    EXPECT_TRUE(erg.wurzel_vorhanden);
    ASSERT_EQ(erg.gefundene_csvs.size(), 2u) << "genau die zwei ECHTEN .result.csv-Dateien, NICHT die .stale";
    for (auto const& p : erg.gefundene_csvs)
        EXPECT_FALSE(mr::endet_mit(p.filename().string(), mr::kStaleSuffix))
            << p.string() << " haette nie in gefundene_csvs landen duerfen";
    EXPECT_EQ(erg.uebersprungen_stale, 1u) << "die Wache muss den Ausschluss ZAEHLEN, nicht nur stumm tun";
    EXPECT_FALSE(erg.scan_gekappt);
}

// -- Test 3: "die Wache schlaegt an, wenn du sie entfernst" -- direkter Gegenbeweis am Objekt --
// Simuliert, was OHNE die .stale-Fallunterscheidung in scanne_realm_root() geschaehe: die Datei
// wuerde einfach STUMM verschwinden (nicht eingeschlossen, aber auch NICHT gezaehlt) statt
// LITERAL als "uebersprungen" gemeldet zu werden. uebersprungen_stale ist damit die Zahl, die eine
// entfernte Wache sofort auf 0 fallen liesse -- der Bissbeweis liegt in der Zahl, nicht nur im
// Ausschluss (der bei DIESER Namensform, s. Test 1, auch ohne die eigene Fallunterscheidung
// strukturell nicht durchrutscht -- der Zaehler ist der scharfe Teil).
TEST(A9S4RealmScanStale, UebersprungenStaleIstDerScharfeTeilNichtNurDerAusschluss) {
    TempDir const tmp;
    schreibe(tmp.path / "x_0_aaaa.result.csv", "binary_id;x\nx;1\n");
    schreibe(tmp.path / "x_1_bbbb.result.csv.stale", "binary_id;x\nx;ALT\n");
    schreibe(tmp.path / "x_2_cccc.result.csv.stale", "binary_id;x\nx;ALT2\n");

    auto const erg = mr::scanne_realm_root(tmp.path);
    ASSERT_EQ(erg.gefundene_csvs.size(), 1u);
    EXPECT_EQ(erg.uebersprungen_stale, 2u) << "zwei .stale-Dateien -- eine entfernte Zaehl-Wache liefert hier 0";
}

TEST(A9S4RealmScanStale, FehlendeWurzelIstEhrlichLeerKeinWurf) {
    auto const erg = mr::scanne_realm_root(std::filesystem::path{"/pfad/der/ganz/sicher/nicht/existiert_a9s4"});
    EXPECT_FALSE(erg.wurzel_vorhanden);
    EXPECT_TRUE(erg.gefundene_csvs.empty());
}
