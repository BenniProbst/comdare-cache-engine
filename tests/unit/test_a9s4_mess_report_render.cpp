// tests/unit/test_a9s4_mess_report_render.cpp -- A9-S4: Gruppierung nach binary_id, Dateinamens-
// Bau ueber die Dynamik-Kette, LED-61-Determinismus (zwei plan-Laeufe, diff==0) und der Fassung-3-
// Pflichtspalten-Abbruch -- gegen eine SYNTHETISCHE, kleine Fixture (nicht das reale Erstbeleg-
// Archiv, das ist A9-S5 und laeuft ueber die echte Binary, nicht ueber diesen Unit-Test).

#include "../../tools/mess_report/mess_report_render.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace mr  = ::comdare::cache_engine::tools::mess_report;
namespace lab = ::comdare::cache_engine::builder::lager_ablage;

namespace {

struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("a9s4_render_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
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
    std::ofstream f(p, std::ios::binary);
    f << inhalt;
}

// Zwei binary_ids, EINE Spalte (workload) mit zwei Werten (dynamisch -> sweep), EINE Spalte
// (platform) mit ueberall demselben Wert (konstant -> Meta), EINE Spalte (working_set_n) mit
// GENAU EINEM belegten Wert (konstant), keine Fassung-3-Spalten.
inline constexpr char kFixtureCsv[] = "binary_id;workload;platform;working_set_n;ns_per_op\n"
                                      "bin_a;ycsb_a;linux-x86_64;4096;11.5\n"
                                      "bin_a;ycsb_b;linux-x86_64;4096;22.5\n"
                                      "bin_b;ycsb_a;linux-x86_64;4096;9.0\n";

} // namespace

TEST(A9S4MessReportRender, GruppiertNachBinaryIdUndZaehltZeilenKorrekt) {
    TempDir const tmp;
    schreibe(tmp.path / "quelle.csv", kFixtureCsv);
    std::vector<std::filesystem::path> const quellen{tmp.path / "quelle.csv"};

    auto const plan =
        mr::berechne_plan(quellen, tmp.path / "ziel", mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);

    ASSERT_EQ(plan.sheets.size(), 2u);
    EXPECT_EQ(plan.gelesene_zeilen, 3u);
    EXPECT_EQ(plan.ohne_binary_id_uebersprungen, 0u);
    // std::map sortiert nach binary_id -- bin_a vor bin_b, deterministisch.
    EXPECT_EQ(plan.sheets[0].organ_unter, "bin_a");
    EXPECT_EQ(plan.sheets[0].zeilen_anzahl, 2u);
    EXPECT_EQ(plan.sheets[1].organ_unter, "bin_b");
    EXPECT_EQ(plan.sheets[1].zeilen_anzahl, 1u);
}

TEST(A9S4MessReportRender, DynamischeSpalteImDateinamenKonstanteInMeta) {
    TempDir const tmp;
    schreibe(tmp.path / "quelle.csv", kFixtureCsv);
    std::vector<std::filesystem::path> const quellen{tmp.path / "quelle.csv"};

    auto const plan =
        mr::berechne_plan(quellen, tmp.path / "ziel", mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);

    EXPECT_NE(plan.dateiname_stamm.find("workload=sweep"), std::string::npos)
        << "workload variiert (ycsb_a/ycsb_b) -- muss als 'sweep' im Dateinamen stehen: " << plan.dateiname_stamm;
    EXPECT_EQ(plan.dateiname_stamm.find("platform="), std::string::npos)
        << "platform ist konstant -- darf NICHT im Dateinamen stehen (nur Meta): " << plan.dateiname_stamm;
    EXPECT_EQ(plan.dateiname_stamm.find("working_set_n="), std::string::npos)
        << "working_set_n ist konstant -- darf NICHT im Dateinamen stehen (nur Meta): " << plan.dateiname_stamm;

    bool platform_in_meta = false, working_set_in_meta = false;
    for (auto const& m : plan.konstanten_meta) {
        if (m.achse == "platform") {
            platform_in_meta = true;
            EXPECT_EQ(m.wert, "linux-x86_64");
        }
        if (m.achse == "working_set_n") {
            working_set_in_meta = true;
            EXPECT_EQ(m.wert, "4096");
        }
    }
    EXPECT_TRUE(platform_in_meta);
    EXPECT_TRUE(working_set_in_meta);
}

TEST(A9S4MessReportRender, PlanIstDeterministisch_ZweiLaeufeDiffNull) {
    TempDir const tmp;
    schreibe(tmp.path / "quelle.csv", kFixtureCsv);
    std::vector<std::filesystem::path> const quellen{tmp.path / "quelle.csv"};

    auto const p1 =
        mr::berechne_plan(quellen, tmp.path / "ziel", mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    auto const p2 =
        mr::berechne_plan(quellen, tmp.path / "ziel", mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);

    std::ostringstream o1, o2;
    mr::drucke_plan(p1, o1);
    mr::drucke_plan(p2, o2);
    EXPECT_EQ(o1.str(), o2.str()) << "LED-61: zwei plan-Laeufe ueber dieselbe Quelle muessen byte-gleich sein";
    EXPECT_EQ(p1.dateiname_stamm, "19700101-000000_workload=sweep")
        << "Epochen-Platzhalter + einzige dynamische Spalte -- literal nachpruefbar";
}

TEST(A9S4MessReportRender, ZeilenOhneBinaryIdWerdenGezaehltNichtEingemischt) {
    TempDir const tmp;
    schreibe(tmp.path / "quelle.csv", "binary_id;x\n;1\nbin_a;2\n");
    std::vector<std::filesystem::path> const quellen{tmp.path / "quelle.csv"};

    auto const plan =
        mr::berechne_plan(quellen, tmp.path / "ziel", mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    EXPECT_EQ(plan.gelesene_zeilen, 2u);
    EXPECT_EQ(plan.ohne_binary_id_uebersprungen, 1u);
    ASSERT_EQ(plan.sheets.size(), 1u);
    EXPECT_EQ(plan.sheets[0].zeilen_anzahl, 1u);
}

TEST(A9S4MessReportRender, FehlendeBinaryIdSpalteBrichtEhrlichAb) {
    TempDir const tmp;
    schreibe(tmp.path / "quelle.csv", "workload;x\na;1\n"); // keine binary_id-Spalte
    std::vector<std::filesystem::path> const quellen{tmp.path / "quelle.csv"};

    // nodiscard-konform: berechne_plan() liefert [[nodiscard]] RenderPlan -- ein (void)-Cast im
    // Lambda-Koerper haelt den Wache-Bau des Rueckgabewerts intakt UND befriedigt die Wache hier.
    EXPECT_THROW(
        (void)mr::berechne_plan(quellen, tmp.path / "ziel", mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx),
        mr::MessReportFehler);
}

TEST(A9S4MessReportRender, HeaderAbweichungZweierQuellenBrichtAb) {
    TempDir const tmp;
    schreibe(tmp.path / "a.csv", "binary_id;x\nbin_a;1\n");
    schreibe(tmp.path / "b.csv", "binary_id;y\nbin_b;2\n"); // andere Kopfzeile
    std::vector<std::filesystem::path> const quellen{tmp.path / "a.csv", tmp.path / "b.csv"};

    EXPECT_THROW(
        (void)mr::berechne_plan(quellen, tmp.path / "ziel", mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx),
        mr::MessReportFehler);
}

TEST(A9S4MessReportRender, RenderSchreibtEineXlsxDateiMitZweiSheets) {
    TempDir const tmp;
    schreibe(tmp.path / "quelle.csv", kFixtureCsv);
    std::vector<std::filesystem::path> const quellen{tmp.path / "quelle.csv"};
    std::filesystem::path const              ziel = tmp.path / "ziel";
    std::filesystem::create_directories(ziel);

    mr::fuehre_render_aus(quellen, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);

    // dateiname_stamm ist "19700101-000000_workload=sweep" (Test oben, literal geprueft) + ".xlsx".
    std::filesystem::path const erwartet = ziel / "19700101-000000_workload=sweep.xlsx";
    EXPECT_TRUE(std::filesystem::exists(erwartet)) << erwartet.string() << " wurde nicht angelegt";
    EXPECT_GT(std::filesystem::file_size(erwartet), 0u);
}
