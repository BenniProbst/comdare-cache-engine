// tests/unit/test_lg_xlsx_alt_nie_vernichten.cpp -- F2-5 Posten 2: LG-XlsxAlt "nie vernichten"
// (Designplan par.2/K5, NIE-fallen-Klasse "Messdaten nie loeschen"; GOAL VI.4: neue Binary-Version
// => neuer Datensatz NEBEN dem alten).
//
// GEGENSTAND: tools/mess_report -- fuehre_render_aus + Sidecar-Skip-Manifest (skip_manifest.hpp).
// Owner woertlich (08.08., skip_manifest.hpp Kopf): "Jede neue Version dieser Binary erzeugt auch
// neue Messdaten fuer die geupdateten Eigenschaften der binary und behaelt die alte Version
// zusaetzlich." Binary-Identitaet ist an diesem Objekt der binary_id-String (die Achsen-
// Kompositions-Kette traegt versionierte Glieder) -- ein Versions-WECHSEL ist hier also ein neuer
// binary_id-String; die build_version-Spalte reist als Tag-Spalte mit (kUnterAchsenKandidaten).
//
// WAS DIESER TEST NEU BEWEIST (Anrechnung statt Doppelung, A2.2): test_a9s4_skip_manifest.cpp:113
// prueft den additiven Drittlauf ueber DATEIZAHL + file_size + mtime der Alt-Datei. Was fehlte, ist
// die VERNICHTUNGS-Negativprobe der NIE-fallen-Klasse: die Alt-Datei bleibt beim Versions-Wechsel
// ueber den VOLLEN INHALT byte-identisch (nicht nur gleich gross), und der Alt-Eintrag im Manifest
// bleibt bestehen. Genau die deckt Abschnitt 1; Abschnitt 2 dokumentiert den T-4-Gegeneingang:
// GLEICHE Version = definiertes Verhalten je Skip-Doktrin (vierter Owner-Nachtrag Punkt 2: SKIP --
// kein zweiter Datensatz, keine Vernichtung).
//
// ZAEHL-NENNER AUS FREMDER QUELLE (T-3): die erwarteten Datei- und Manifest-Zahlen (1 nach Lauf 1,
// 2 nach Lauf 2) stammen aus den QUELL-LITERALEN dieses Tests (Anzahl distinkter binary_id-Strings
// je Quelle), nicht aus einem Zaehler des Renderers; der Byte-Nenner der Negativprobe wird VOR dem
// jeweils naechsten Lauf gelesen. ASCII-only.

#include "../../tools/mess_report/mess_report_render.hpp"
#include "../../tools/mess_report/skip_manifest.hpp"

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
               ("lg_xlsx_alt_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
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

[[nodiscard]] std::string lies_bytes(std::filesystem::path const& p) {
    std::ifstream     f(p, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

[[nodiscard]] std::vector<std::filesystem::path> xlsx_dateien(std::filesystem::path const& dir) {
    std::vector<std::filesystem::path> out;
    for (auto const& e : std::filesystem::directory_iterator(dir))
        if (e.path().extension() == ".xlsx") out.push_back(e.path());
    return out;
}

// EIN binary_id-String je Quelle; der Versions-Wechsel ist der Wechsel des Ketten-Strings (v1 -> v2),
// die build_version-Tag-Spalte traegt dieselbe Aussage als Daten-Spalte mit.
inline constexpr char kCsvVersion1[] = "binary_id;workload;build_version\nbin_x_v1;w;1.0.0.c\n";
inline constexpr char kCsvVersion2[] = "binary_id;workload;build_version\nbin_x_v2;w;2.0.0.c\n";

} // namespace

// -- 1. Versions-WECHSEL: additiver neuer Datensatz, Alt-Datei VOLLBYTE-identisch (Negativprobe) --

TEST(LgXlsxAltNieVernichten, VersionswechselSchreibtAdditivAltDateiBleibtByteIdentisch) {
    TempDir const               tmp;
    std::filesystem::path const quelle_v1 = tmp.path / "quelle_v1.csv";
    std::filesystem::path const quelle_v2 = tmp.path / "quelle_v2.csv";
    std::filesystem::path const ziel      = tmp.path / "ziel";
    schreibe(quelle_v1, kCsvVersion1);
    schreibe(quelle_v2, kCsvVersion2);

    // Lauf 1: alte Version (bin_x_v1). T-3-Nenner: EIN binary_id-Literal in kCsvVersion1 -> 1 Datei.
    mr::fuehre_render_aus({quelle_v1}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    auto const nach_lauf1 = xlsx_dateien(ziel);
    ASSERT_EQ(nach_lauf1.size(), 1u) << "Nenner aus kCsvVersion1: genau ein binary_id-Literal";
    std::filesystem::path const alt_datei = nach_lauf1.front();
    std::string const           alt_bytes = lies_bytes(alt_datei);
    ASSERT_FALSE(alt_bytes.empty()) << "Byte-Nenner der Negativprobe (VOR Lauf 2 gelesen)";

    // Lauf 2: NEUE Version (bin_x_v2) mit eigenem Zeitstempel -> eigener Dateiname (additiv daneben).
    mr::DatumZeit const eine_sekunde_spaeter{"19700101", "000001"};
    mr::fuehre_render_aus({quelle_v2}, ziel, eine_sekunde_spaeter, lab::ErgebnisFormat::xlsx);

    auto const nach_lauf2 = xlsx_dateien(ziel);
    EXPECT_EQ(nach_lauf2.size(), 2u) << "Nenner aus beiden Quell-Literalen: neue Version ADDITIV daneben";
    ASSERT_TRUE(std::filesystem::exists(alt_datei)) << "die Alt-Datei existiert nach dem Versions-Wechsel weiter";

    // DIE NEGATIVPROBE der NIE-fallen-Klasse: VOLLER Inhalt, nicht nur Groesse/mtime (Bestand :113).
    std::string const alt_bytes_nach_lauf2 = lies_bytes(alt_datei);
    EXPECT_EQ(alt_bytes_nach_lauf2.size(), alt_bytes.size())
        << "Alt-Datei: Groesse veraendert -- Messdaten wurden angefasst";
    EXPECT_TRUE(alt_bytes_nach_lauf2 == alt_bytes)
        << "Alt-Datei: Bytes veraendert -- ein Lauf mit NEUER Version darf den alten Datensatz NIE "
           "ueberschreiben/vernichten (GOAL VI.4, Messdaten-nie-loeschen)";

    // Der neue Datensatz ist real ein ZWEITER (anderer Pfad, eigener Inhalt).
    std::filesystem::path neue_datei;
    for (auto const& p : nach_lauf2)
        if (p != alt_datei) neue_datei = p;
    ASSERT_FALSE(neue_datei.empty()) << "der neue Datensatz liegt NEBEN dem alten, nicht an seiner Stelle";
    EXPECT_FALSE(lies_bytes(neue_datei).empty());

    // Und die Buchfuehrung behaelt BEIDE Versionen (additives Manifest, skip_manifest.hpp).
    auto const manifest = mr::lies_manifest(ziel);
    EXPECT_EQ(manifest.size(), 2u) << "Nenner: zwei binary_id-Literale ueber beide Quellen";
    EXPECT_TRUE(manifest.contains("bin_x_v1")) << "der Alt-Eintrag bleibt bestehen";
    EXPECT_TRUE(manifest.contains("bin_x_v2"));
}

// -- 2. T-4-Gegeneingang: GLEICHE Version = definiertes Verhalten je Skip-Doktrin (SKIP) ----------

TEST(LgXlsxAltNieVernichten, GleicheVersionZweitlaufSkipptOhneVernichtung) {
    TempDir const               tmp;
    std::filesystem::path const quelle_v1 = tmp.path / "quelle_v1.csv";
    std::filesystem::path const ziel      = tmp.path / "ziel";
    schreibe(quelle_v1, kCsvVersion1);

    mr::fuehre_render_aus({quelle_v1}, ziel, mr::kPlanZeitstempelPlatzhalter, lab::ErgebnisFormat::xlsx);
    auto const nach_lauf1 = xlsx_dateien(ziel);
    ASSERT_EQ(nach_lauf1.size(), 1u);
    std::string const alt_bytes = lies_bytes(nach_lauf1.front());
    ASSERT_FALSE(alt_bytes.empty());

    // Zweitlauf mit EXAKT DERSELBEN Version (gleicher binary_id-String), eigener Zeitstempel: die
    // Skip-Doktrin (vierter Owner-Nachtrag Punkt 2) definiert SKIP -- kein zweiter Datensatz, und
    // erst recht keine Vernichtung des bestehenden.
    mr::DatumZeit const eine_sekunde_spaeter{"19700101", "000001"};
    mr::fuehre_render_aus({quelle_v1}, ziel, eine_sekunde_spaeter, lab::ErgebnisFormat::xlsx);

    auto const nach_lauf2 = xlsx_dateien(ziel);
    EXPECT_EQ(nach_lauf2.size(), 1u) << "gleiche Version -> SKIP: kein zweiter Datensatz (definiertes Verhalten)";
    EXPECT_TRUE(lies_bytes(nach_lauf1.front()) == alt_bytes)
        << "gleiche Version -> SKIP: der bestehende Datensatz bleibt VOLLBYTE-identisch";
    auto const manifest = mr::lies_manifest(ziel);
    EXPECT_EQ(manifest.size(), 1u) << "Nenner: EIN binary_id-Literal -- das Manifest bleibt unveraendert";
    EXPECT_TRUE(manifest.contains("bin_x_v1"));
}
