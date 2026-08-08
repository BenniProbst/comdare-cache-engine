// tests/unit/test_a9s4_skip_manifest.cpp -- A9-S4, vierter Owner-Nachtrag Punkt 2: SKIP bei
// validem Bestand fuer exakt dieselbe Binary. Deckt sowohl das Sidecar-Manifest
// (tools/mess_report/skip_manifest.hpp) direkt als auch seine Verdrahtung in
// mess_report_render.hpp (berechne_plan/fuehre_render_aus) ab -- ein Lauf, ein zweiter Lauf
// gegen dieselbe Quelle (muss alles skippen), ein dritter Lauf mit einer neuen Binary daneben
// (muss additiv nur die neue schreiben, die alte Datei bleibt unangetastet).

#include "../../tools/mess_report/mess_report_render.hpp"
#include "../../tools/mess_report/skip_manifest.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <ctime>
#include <filesystem>
#include <fstream>
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
               ("a9s4_skip_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
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

} // namespace

// ── Manifest direkt ──────────────────────────────────────────────────────────────────────────

TEST(A9S4SkipManifest, FehlendesManifestIstEhrlichLeer) {
    TempDir const tmp;
    EXPECT_TRUE(mr::lies_manifest(tmp.path).empty());
}

TEST(A9S4SkipManifest, ErgaenzenUndLesenRoundtrip) {
    TempDir const tmp;
    mr::ergaenze_manifest(tmp.path, {"bin_a", "bin_b"});
    auto const bestand = mr::lies_manifest(tmp.path);
    EXPECT_EQ(bestand.size(), 2u);
    EXPECT_TRUE(bestand.contains("bin_a"));
    EXPECT_TRUE(bestand.contains("bin_b"));
}

TEST(A9S4SkipManifest, ErgaenzenIstAdditivBestehendeEintraegeBleiben) {
    TempDir const tmp;
    mr::ergaenze_manifest(tmp.path, {"bin_a"});
    mr::ergaenze_manifest(tmp.path, {"bin_b"});
    auto const bestand = mr::lies_manifest(tmp.path);
    EXPECT_EQ(bestand.size(), 2u) << "der zweite Aufruf darf den ersten Eintrag nicht verdraengen";
    EXPECT_TRUE(bestand.contains("bin_a"));
    EXPECT_TRUE(bestand.contains("bin_b"));
}

TEST(A9S4SkipManifest, LeereListeIstNoOpKeinVerzeichnisAngelegt) {
    TempDir const         tmp;
    std::filesystem::path nie_angelegt = tmp.path / "unterordner";
    mr::ergaenze_manifest(nie_angelegt, {});
    EXPECT_FALSE(std::filesystem::exists(nie_angelegt)) << "lazy: kein Anlegen ohne echten Anlass";
}

// ── filtere_bereits_vorhandene ───────────────────────────────────────────────────────────────

TEST(A9S4SkipManifest, FilterTrenntNeuUndBereitsVorhandenXlsx) {
    TempDir const tmp;
    mr::ergaenze_manifest(tmp.path, {"bin_a"});
    std::map<std::string, mr::ZeilenGruppe> gruppen;
    gruppen["bin_a"] = {"bin_a", {{"bin_a", "1"}}};
    gruppen["bin_b"] = {"bin_b", {{"bin_b", "1"}}};

    auto const erg = mr::filtere_bereits_vorhandene(gruppen, tmp.path, lab::ErgebnisFormat::xlsx);
    ASSERT_EQ(erg.neu.size(), 1u);
    EXPECT_TRUE(erg.neu.contains("bin_b"));
    ASSERT_EQ(erg.bereits_vorhanden.size(), 1u);
    EXPECT_EQ(erg.bereits_vorhanden.front(), "bin_a");
}

TEST(A9S4SkipManifest, CsvFormatUmgehtDenFilterVollstaendig) {
    TempDir const tmp;
    mr::ergaenze_manifest(tmp.path, {"bin_a"}); // Manifest EXISTIERT und nennt bin_a
    std::map<std::string, mr::ZeilenGruppe> gruppen;
    gruppen["bin_a"] = {"bin_a", {{"bin_a", "1"}}};

    auto const erg = mr::filtere_bereits_vorhandene(gruppen, tmp.path, lab::ErgebnisFormat::csv);
    EXPECT_EQ(erg.neu.size(), 1u) << "CSV-Testpfad liest den Skip-Zustand NICHT (Owner: CSV wird nie verwendet)";
    EXPECT_TRUE(erg.bereits_vorhanden.empty());
}

// ── End-zu-Ende: render -> render (skip) -> render mit neuer Binary (additiv) ───────────────

inline constexpr char kCsvA[]     = "binary_id;workload\nbin_a;x\n";
inline constexpr char kCsvB_neu[] = "binary_id;workload\nbin_b;y\n";

TEST(A9S4SkipManifest, ZweiterLaufGegenDieselbeQuelleSchreibtNichtsDrittTerNeueBinaryAdditiv) {
    TempDir const               tmp;
    std::filesystem::path const quelle_a = tmp.path / "quelle_a.csv";
    std::filesystem::path const ziel     = tmp.path / "ziel";
    schreibe(quelle_a, kCsvA);

    // Lauf 1: frisch, ein neuer Sheet-Eintrag.
    mr::fuehre_render_aus({quelle_a}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    std::size_t dateien_nach_lauf1 = 0;
    for (auto const& e : std::filesystem::directory_iterator(ziel))
        if (e.path().extension() == ".xlsx") ++dateien_nach_lauf1;
    ASSERT_EQ(dateien_nach_lauf1, 1u);

    std::filesystem::path erste_datei;
    for (auto const& e : std::filesystem::directory_iterator(ziel))
        if (e.path().extension() == ".xlsx") erste_datei = e.path();
    ASSERT_FALSE(erste_datei.empty());
    auto const groesse_erste_datei_v1 = std::filesystem::file_size(erste_datei);
    auto const zeit_erste_datei_v1    = std::filesystem::last_write_time(erste_datei);

    // Lauf 2: dieselbe Quelle -- MUSS ueberhaupt nichts schreiben (alles im Manifest).
    mr::fuehre_render_aus({quelle_a}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    std::size_t dateien_nach_lauf2 = 0;
    for (auto const& e : std::filesystem::directory_iterator(ziel))
        if (e.path().extension() == ".xlsx") ++dateien_nach_lauf2;
    EXPECT_EQ(dateien_nach_lauf2, 1u) << "Lauf 2 gegen dieselbe Binary darf keine zweite Datei anlegen";
    EXPECT_EQ(std::filesystem::file_size(erste_datei), groesse_erste_datei_v1)
        << "die erste Datei darf durch Lauf 2 nicht veraendert werden (byte-identisch)";
    EXPECT_EQ(std::filesystem::last_write_time(erste_datei), zeit_erste_datei_v1)
        << "die erste Datei darf nicht neu geschrieben worden sein (mtime unveraendert)";

    // Lauf 3: eine NEUE Binary (bin_b) daneben -- additiv, alte Datei unangetastet. ANDERER
    // Zeitstempel als Lauf 1/2 (reales render() nutzt jetzt_utc(), das real weiterlaeuft; der
    // Kollisions-Schutz unten (eigener Test) prueft den Fall GLEICHER Zeitstempel separat).
    std::filesystem::path const quelle_b = tmp.path / "quelle_b.csv";
    schreibe(quelle_b, kCsvB_neu);
    mr::DatumZeit const eine_sekunde_spaeter{"19700101", "000001"};
    mr::fuehre_render_aus({quelle_b}, ziel, eine_sekunde_spaeter, lab::ErgebnisFormat::xlsx);
    std::size_t dateien_nach_lauf3 = 0;
    for (auto const& e : std::filesystem::directory_iterator(ziel))
        if (e.path().extension() == ".xlsx") ++dateien_nach_lauf3;
    EXPECT_EQ(dateien_nach_lauf3, 2u) << "eine neue Binary muss ADDITIV eine zweite Datei erzeugen";
    EXPECT_TRUE(std::filesystem::exists(erste_datei)) << "die alte Datei bleibt bestehen";
    EXPECT_EQ(std::filesystem::file_size(erste_datei), groesse_erste_datei_v1)
        << "die alte Datei bleibt auch nach Lauf 3 unveraendert";

    auto const manifest = mr::lies_manifest(ziel);
    EXPECT_EQ(manifest.size(), 2u);
    EXPECT_TRUE(manifest.contains("bin_a"));
    EXPECT_TRUE(manifest.contains("bin_b"));
}

TEST(A9S4SkipManifest, PlanZeigtSkipVorschauOhneZuSchreiben) {
    TempDir const               tmp;
    std::filesystem::path const quelle = tmp.path / "quelle.csv";
    std::filesystem::path const ziel   = tmp.path / "ziel";
    schreibe(quelle, kCsvA);

    mr::fuehre_render_aus({quelle}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    std::size_t vor_plan = 0;
    for (auto const& e : std::filesystem::directory_iterator(ziel))
        if (e.path().extension() == ".xlsx") ++vor_plan;
    ASSERT_EQ(vor_plan, 1u);

    auto const plan = mr::berechne_plan({quelle}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    EXPECT_TRUE(plan.schreibt_nichts);
    ASSERT_EQ(plan.bereits_vorhanden_binary_ids.size(), 1u);
    EXPECT_EQ(plan.bereits_vorhanden_binary_ids.front(), "bin_a");
    EXPECT_TRUE(plan.sheets.empty());

    std::size_t nach_plan = 0;
    for (auto const& e : std::filesystem::directory_iterator(ziel))
        if (e.path().extension() == ".xlsx") ++nach_plan;
    EXPECT_EQ(nach_plan, 1u) << "plan() darf niemals schreiben";
}

// ── Kollisions-Schutz: NIE still ueberschreiben (der Defekt, den die Lazy-mkdir-Reparatur oben
// zuerst freigelegt hat -- zwei GETRENNTE render-Aufrufe fuer VERSCHIEDENE Binaries, die zufaellig
// denselben Zeitstempel + dieselbe (leere) dynamische kv-Kette tragen, duerfen NIE dieselbe Datei
// treffen und sich gegenseitig ueberschreiben). ──────────────────────────────────────────────

inline constexpr char kCsvC_andereBinary[] = "binary_id;workload\nbin_c;z\n";

TEST(A9S4SkipManifest, GleicherDateinameAusZweiGetrenntenLaeufenBrichtLautAbStattStillZuUeberschreiben) {
    TempDir const               tmp;
    std::filesystem::path const quelle_a = tmp.path / "quelle_a.csv";
    std::filesystem::path const quelle_c = tmp.path / "quelle_c.csv";
    std::filesystem::path const ziel     = tmp.path / "ziel";
    schreibe(quelle_a, kCsvA);
    schreibe(quelle_c, kCsvC_andereBinary);

    // Lauf 1: bin_a, Epochen-Platzhalter (deterministisch, kein Wall-Clock-Rauschen im Test).
    mr::fuehre_render_aus({quelle_a}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    std::filesystem::path erste_datei;
    for (auto const& e : std::filesystem::directory_iterator(ziel))
        if (e.path().extension() == ".xlsx") erste_datei = e.path();
    ASSERT_FALSE(erste_datei.empty());
    auto const groesse_v1 = std::filesystem::file_size(erste_datei);

    // Lauf 2: bin_c (eine ANDERE, neue Binary), DERSELBE Zeitstempel -> derselbe Dateiname (beide
    // Quellen haben nur einen konstanten workload-Wert, also eine leere dynamische kv-Kette ->
    // "19700101-000000.xlsx" in BEIDEN Faellen). Das MUSS werfen, NICHT still ueberschreiben.
    EXPECT_THROW((void)([&] {
                     mr::fuehre_render_aus({quelle_c}, ziel, mr::kPlanZeitstempelPlatzhalter,
                                           lab::ErgebnisFormat::xlsx);
                 }()),
                 mr::MessReportFehler);

    // Die ERSTE Datei muss unangetastet geblieben sein -- die Wache greift VOR jedem Schreiben.
    EXPECT_TRUE(std::filesystem::exists(erste_datei));
    EXPECT_EQ(std::filesystem::file_size(erste_datei), groesse_v1);
    // bin_c darf NICHT stillschweigend ins Manifest gewandert sein (der Abbruch kam VOR
    // schliessen()/ergaenze_manifest()).
    EXPECT_FALSE(mr::lies_manifest(ziel).contains("bin_c"));
}
