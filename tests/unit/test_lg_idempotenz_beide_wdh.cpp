// test_lg_idempotenz_beide_wdh -- B09(b) (K01, Designplan par.4 Z44 "LG-Idempotenz 2x3", Klasse
// K2): die SECHSTE Zelle der Idempotenz-Matrix -- [beide Formate x Wiederholungslauf].
//
// MATRIX-NENNER (Session-Befund 18.08., objektweit): {xlsx, csv, beide} x {Erstlauf, WDH} = 6.
//   [xlsx x Erst]  + [csv x Erst]  : test_a9s4_mess_report_render / test_a9s5 (:275/:308)
//   [beide x Erst]                 : test_a9s5_ergebnis_mappe_naht:335/:132
//   [xlsx x WDH]   + [csv x WDH]   : test_a9s4_skip_manifest:113/:83/:97
//   [beide x WDH]                  : FEHLTE -- DIESE Datei.
//
// WAS DIE KOMBI-ZELLE NEU BEWEIST (nicht die Summe der Einzel-Zellen): im SELBEN Ziel-Ordner
// laufen BEIDE Strategien; der xlsx-Skip (Manifest) muss auch dann halten, wenn csv-Artefakte
// desselben Bestands daneben liegen -- und die csv-Haelfte behaelt ihre dokumentierte
// NICHT-Idempotenz (Owner-Doktrin, gepinnt in test_a9s4 "CsvFormatUmgehtDenFilter": der
// CSV-Testpfad liest den Skip-Zustand NICHT). Eine Kombi-Fassung, die den Skip-Zustand aus
// "irgendeiner vorhandenen Datei" ableitete, faellt hier auf.
//
// F2-ABNAHMEFORMEL ("SKIP-Zweitlauf ruft den Mess-Callback 0-mal"): WOERTLICH gedeckt durch
// test_lg_skip_callback_null.cpp am Iterator (Anrechnung statt Doppelung); DIESE Datei traegt
// die Render-/Format-Seite derselben Doktrin.
//
// T-11c-MUTATIONSANKER: die Manifest-Ergaenzung nach Lauf 1 wegmutieren laesst den xlsx-WDH
// wieder schreiben -- XlsxHaelfteBleibtIdempotent bricht literal (mtime/Anzahl).

#include "../../tools/mess_report/mess_report_render.hpp"
#include "../../tools/mess_report/skip_manifest.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

namespace mr  = ::comdare::cache_engine::tools::mess_report;
namespace lab = ::comdare::cache_engine::builder::lager_ablage;
namespace fs  = std::filesystem;

struct TempDir {
    fs::path path;
    TempDir() {
        path = fs::temp_directory_path() / ("comdare_lg_beide_wdh_" + std::to_string(::rand()));
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

void schreibe(fs::path const& p, std::string const& inhalt) {
    std::ofstream out{p, std::ios::binary | std::ios::trunc};
    out << inhalt;
}

[[nodiscard]] std::size_t zaehle_endung(fs::path const& dir, std::string const& endung) {
    std::size_t     n = 0;
    std::error_code ec;
    for (auto const& e : fs::directory_iterator{dir, ec})
        if (e.path().filename().string().ends_with(endung)) ++n;
    return n;
}

inline constexpr char kCsvQuelle[] = "binary_id;workload\nbin_a;x\n";

TEST(LgIdempotenzBeideWdh, SechsteZelleBeideFormateWiederholungslauf) {
    TempDir const  tmp;
    fs::path const quelle = tmp.path / "quelle.csv";
    fs::path const ziel   = tmp.path / "ziel";
    schreibe(quelle, kCsvQuelle);

    // -- ERSTLAUF BEIDER STRATEGIEN in DASSELBE Ziel (die Beide-Wahl der Naht = zwei oeffne()). --
    mr::fuehre_render_aus({quelle}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    mr::fuehre_render_aus({quelle}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::csv);

    ASSERT_EQ(zaehle_endung(ziel, ".xlsx"), 1u) << "Erstlauf: genau eine xlsx";
    std::size_t const csv_nach_erstlauf = zaehle_endung(ziel, ".csv");
    ASSERT_GE(csv_nach_erstlauf, 1u) << "Erstlauf: csv-Artefakte liegen im SELBEN Ziel";
    // csv-Beobachter: unter kPlanZeitstempelPlatzhalter ist der Stamm deterministisch -- ein
    // WDH-csv-Lauf ueberschreibt DENSELBEN Namen (gemessen 2026-08-21: Anzahl bleibt 2). Das
    // "schreibt WIEDER" ist deshalb an der mtime beobachtbar, nicht an der Datei-Zahl.
    fs::path erste_csv;
    for (auto const& e : fs::directory_iterator{ziel})
        if (e.path().extension() == ".csv") erste_csv = e.path();
    ASSERT_FALSE(erste_csv.empty());
    auto const csv_mtime_v1 = fs::last_write_time(erste_csv);

    fs::path erste_xlsx;
    for (auto const& e : fs::directory_iterator{ziel})
        if (e.path().extension() == ".xlsx") erste_xlsx = e.path();
    ASSERT_FALSE(erste_xlsx.empty());
    auto const groesse_v1 = fs::file_size(erste_xlsx);
    auto const mtime_v1   = fs::last_write_time(erste_xlsx);

    // -- WIEDERHOLUNGSLAUF BEIDER STRATEGIEN ueber identischen Bestand. ---------------------------
    mr::fuehre_render_aus({quelle}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    mr::fuehre_render_aus({quelle}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::csv);

    // (1) xlsx-Haelfte IDEMPOTENT -- trotz csv-Nachbarschaft im selben Ziel:
    EXPECT_EQ(zaehle_endung(ziel, ".xlsx"), 1u) << "WDH: 0 neue xlsx-Schreibungen (Manifest-Skip haelt)";
    EXPECT_EQ(fs::file_size(erste_xlsx), groesse_v1) << "WDH: die xlsx ist byte-unveraendert";
    EXPECT_EQ(fs::last_write_time(erste_xlsx), mtime_v1) << "WDH: die xlsx wurde nicht neu geschrieben";

    // (2) csv-Haelfte behaelt ihre dokumentierte NICHT-Idempotenz (Owner: der CSV-Pfad liest den
    //     Skip-Zustand nicht; test_a9s4 "CsvFormatUmgehtDenFilter" ist die Einzel-Zellen-Quelle).
    //     Beobachter = mtime (der Platzhalter-Stamm wird ueberschrieben, s.o.):
    EXPECT_EQ(zaehle_endung(ziel, ".csv"), csv_nach_erstlauf)
        << "WDH ueberschreibt den deterministischen csv-Stamm (keine neuen Namen)";
    EXPECT_NE(fs::last_write_time(erste_csv), csv_mtime_v1)
        << "WDH: die csv-Haelfte schreibt WIEDER (dokumentierte Doktrin) -- taete sie es nicht, "
           "laese der csv-Pfad ploetzlich einen Skip-Zustand";

    // (3) Nenner-Schluss: 5 vorbestandene Zellen (Kopf) + DIESE = 6/6 -- die Matrix ist voll.
    SUCCEED() << "Zelle [beide x Wiederholungslauf] belegt; Matrix 6/6 (Nenner im Dateikopf)";
}

} // namespace
