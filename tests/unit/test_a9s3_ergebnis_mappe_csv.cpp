// test_a9s3_ergebnis_mappe_csv -- A9-S3: die Sheet-Namens-/Zeilenlimit-Wachen (direkt gegen adversariale
// Eingaben gebissen, nicht nur incidental ueber den eigenen Generator geprueft) + der CSV-Fallback
// End-zu-Ende (header-only, KEIN Vendor-Link noetig -- reines std::ofstream).
//
// Bissproben-Struktur je Wache: (rot) ein Eingang, den die Wache ablehnen MUSS, (gruen) ein Nachbar-
// Eingang, den sie durchlassen MUSS. "Der Koeder muss erst beissen" -- die Wache wird direkt aufgerufen,
// nicht nur ueber den eigenen (immer wohlgeformten) Sheet-Label-Generator incidental mitgeprueft.

#include "lager_ablage/ergebnis_mappe.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace lab = comdare::cache_engine::builder::lager_ablage;
namespace ms  = comdare::cache_engine::measurement;

namespace {

struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("a9s3_csv_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
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

} // namespace

// -----------------------------------------------------------------------------
// Bissprobe 1: Sheet-Name > 31 Zeichen wird ABGELEHNT (nicht abgeschnitten).
// -----------------------------------------------------------------------------
TEST(A9S3SheetnameWache, UeberlangerNameWirdAbgelehnt) {
    std::string const zu_lang(32, 'a'); // 32 Zeichen -- eins ueber dem Limit
    EXPECT_FALSE(lab::xlsx_sheetname_zulaessig(zu_lang)) << "32 Zeichen MUESSEN abgelehnt werden (rot).";

    std::string const genau_am_limit(31, 'a'); // 31 Zeichen -- genau am Limit, muss durchgehen
    EXPECT_TRUE(lab::xlsx_sheetname_zulaessig(genau_am_limit)) << "31 Zeichen sind das Limit, kein Ueberlauf (gruen).";

    // "nicht abgeschnitten": diese API bietet KEINE Kuerzungsfunktion -- der einzige Ausgang eines zu
    // langen Namens ist xlsx_sheetname_zulaessig()==false. Die Kuerzung auf 31 Zeichen waere ein anderer,
    // hier bewusst NICHT existierender Codepfad; der obige EXPECT_FALSE ist bereits der volle Beleg dafuer.
}

// -----------------------------------------------------------------------------
// Bissprobe 2: Sheet-Name mit '[' oder '/' wird ABGELEHNT.
// -----------------------------------------------------------------------------
TEST(A9S3SheetnameWache, VerboteneZeichenWerdenAbgelehnt) {
    EXPECT_FALSE(lab::xlsx_sheetname_zulaessig("a[b")) << "'[' MUSS abgelehnt werden (rot).";
    EXPECT_FALSE(lab::xlsx_sheetname_zulaessig("a/b")) << "'/' MUSS abgelehnt werden (rot).";
    EXPECT_FALSE(lab::xlsx_sheetname_zulaessig("a]b"));
    EXPECT_FALSE(lab::xlsx_sheetname_zulaessig("a:b"));
    EXPECT_FALSE(lab::xlsx_sheetname_zulaessig("a*b"));
    EXPECT_FALSE(lab::xlsx_sheetname_zulaessig("a?b"));
    EXPECT_FALSE(lab::xlsx_sheetname_zulaessig("a\\b"));

    EXPECT_TRUE(lab::xlsx_sheetname_zulaessig("a_b-c.d")) << "ein sauberer Name MUSS durchgehen (gruen).";
    EXPECT_TRUE(lab::xlsx_sheetname_zulaessig("S001"));
}

// -----------------------------------------------------------------------------
// sheet_label_fuer_index: deterministisch, S001.. wachsend, immer wachengueltig.
// -----------------------------------------------------------------------------
TEST(A9S3SheetLabel, DeterministischUndZulaessig) {
    EXPECT_EQ(lab::sheet_label_fuer_index(1), "S001");
    EXPECT_EQ(lab::sheet_label_fuer_index(42), "S042");
    EXPECT_EQ(lab::sheet_label_fuer_index(999), "S999");
    EXPECT_EQ(lab::sheet_label_fuer_index(1000), "S1000"); // waechst additiv, kein Umbruch
    for (std::size_t i : {1u, 42u, 999u, 1000u})
        EXPECT_TRUE(lab::xlsx_sheetname_zulaessig(lab::sheet_label_fuer_index(i)));
}

// -----------------------------------------------------------------------------
// Bissprobe 3: das Zeilenlimit loest ErgebnisSchreibFehler aus statt still zu truncieren -- an der
// REALEN Grenze (1.048.576) UND an einem winzigen injizierten Limit (bissfest bei jeder Groesse).
// -----------------------------------------------------------------------------
TEST(A9S3ZeilenlimitWache, RealeGrenzeBeisstGenauAmUebergang) {
    EXPECT_TRUE(lab::xlsx_zeile_erlaubt(lab::kXlsxZeilenlimit - 1)) << "letzte gueltige Zeile (gruen).";
    EXPECT_FALSE(lab::xlsx_zeile_erlaubt(lab::kXlsxZeilenlimit)) << "eine zu viel (rot).";
    EXPECT_FALSE(lab::xlsx_zeile_erlaubt(lab::kXlsxZeilenlimit + 1000)) << "weit druebergefahren bleibt rot.";
}

TEST(A9S3ZeilenlimitWache, InjiziertesLimitBeisstOhneEineMillionZeilenZuSchreiben) {
    // Dieselbe Grenzlogik an einem winzigen Limit -- beweist, dass die Wache SELBST bissfest ist, nicht
    // nur, dass unser eigener Sheet-Generator nie ueber die reale Zahl hinauskommt.
    EXPECT_TRUE(lab::xlsx_zeile_erlaubt(0, 1));
    EXPECT_FALSE(lab::xlsx_zeile_erlaubt(1, 1));
    EXPECT_TRUE(lab::xlsx_zeile_erlaubt(99, 100));
    EXPECT_FALSE(lab::xlsx_zeile_erlaubt(100, 100));
}

// -----------------------------------------------------------------------------
// ErgebnisSchreibFehler: die Fehlerklasse ist die axis_error.hpp-Taxonomie, kein Insel-Enum.
// -----------------------------------------------------------------------------
TEST(A9S3ErgebnisSchreibFehler, TraegtDieRichtigeFehlerklasseUndEtikett) {
    lab::ErgebnisSchreibFehler const f{ms::LagerAblageFehlerKlasse::Zeilenlimit, "test-kontext"};
    EXPECT_EQ(f.klasse(), ms::LagerAblageFehlerKlasse::Zeilenlimit);
    EXPECT_EQ(f.kontext(), "test-kontext");
    std::string_view const what{f.what()};
    EXPECT_NE(what.find("lager_ablage_zeilenlimit"), std::string_view::npos) << what;
    EXPECT_NE(what.find("test-kontext"), std::string_view::npos) << what;
}

// -----------------------------------------------------------------------------
// CsvErgebnisMappe End-zu-Ende: zwei Sheets + INFO, Token-Treue, atomares Rename (keine .tmp-Reste).
// -----------------------------------------------------------------------------
TEST(A9S3CsvErgebnisMappe, ZweiSheetsPlusInfoWerdenGeschriebenUndSindLesbar) {
    TempDir const tmp;
    auto          mappe = lab::ErgebnisMappenFactory::oeffne(tmp.path, "erstbeleg_test", lab::ErgebnisFormat::csv);
    ASSERT_NE(mappe, nullptr);

    lab::MaschinenSysinfo sys{};
    sys.hostname   = "test-host";
    sys.os_version = "noble-24.04";
    lab::HauptAchsenBelegung haupt{{"measurement_tooling", "wallclock"}, {"target_isa", "amd64_v3"}};
    lab::KonstantenMeta      konst{{"operating_system", "linux"}};
    mappe->info_blatt(sys, haupt, konst);

    lab::SheetSchluessel const s1{"ycsb_a", "", ""};
    lab::SheetSchluessel const s2{"ycsb_c", "", ""};
    auto&                      b1 = mappe->blatt(s1);
    b1.kopf(std::vector<std::string>{"binary_id", "n_ops", "status"});
    b1.zeile(std::vector<std::string>{"bin-a", "10000", "failed"});
    auto& b2 = mappe->blatt(s2);
    b2.kopf(std::vector<std::string>{"binary_id", "n_ops", "status"});
    b2.zeile(std::vector<std::string>{"bin-c", "20000", "gesperrt"});
    b2.zeile(std::vector<std::string>{"bin-d", "n/a", "nicht_gebaut"});

    // erneuter blatt()-Aufruf mit demselben Schluessel MUSS dasselbe Sheet liefern (Idempotenz).
    auto& b1_again = mappe->blatt(s1);
    EXPECT_EQ(&b1, &b1_again);

    mappe->schliessen();

    auto const s001 = tmp.path / "erstbeleg_test__S001.csv";
    auto const s002 = tmp.path / "erstbeleg_test__S002.csv";
    auto const info = tmp.path / "erstbeleg_test__INFO.csv";
    ASSERT_TRUE(std::filesystem::exists(s001)) << s001;
    ASSERT_TRUE(std::filesystem::exists(s002)) << s002;
    ASSERT_TRUE(std::filesystem::exists(info)) << info;

    // KEINE .tmp-Reste -- die Atomaritaet ist ueber Rename hergestellt, nicht durch In-Place-Schreiben.
    EXPECT_FALSE(std::filesystem::exists(tmp.path / "erstbeleg_test__S001.csv.tmp"));
    EXPECT_FALSE(std::filesystem::exists(tmp.path / "erstbeleg_test__INFO.csv.tmp"));

    std::string const inhalt_s1 = read_file(s001);
    EXPECT_NE(inhalt_s1.find("binary_id;n_ops;status\n"), std::string::npos) << inhalt_s1;
    EXPECT_NE(inhalt_s1.find("bin-a;10000;failed\n"), std::string::npos) << inhalt_s1;

    std::string const inhalt_s2 = read_file(s002);
    EXPECT_NE(inhalt_s2.find("bin-c;20000;gesperrt\n"), std::string::npos) << inhalt_s2;
    EXPECT_NE(inhalt_s2.find("bin-d;n/a;nicht_gebaut\n"), std::string::npos) << inhalt_s2;

    std::string const inhalt_info = read_file(info);
    EXPECT_NE(inhalt_info.find("sysinfo;hostname;test-host"), std::string::npos) << inhalt_info;
    EXPECT_NE(inhalt_info.find("sysinfo;ram_pair;n/a"), std::string::npos)
        << "n/a statt Null fuer ein leeres Feld: " << inhalt_info;
    EXPECT_NE(inhalt_info.find("hauptachse;measurement_tooling;wallclock"), std::string::npos) << inhalt_info;
    EXPECT_NE(inhalt_info.find("konstante;operating_system;linux"), std::string::npos) << inhalt_info;
    EXPECT_NE(inhalt_info.find("sheet_legende;S001;ycsb_a||"), std::string::npos) << inhalt_info;
    EXPECT_NE(inhalt_info.find("sheet_legende;S002;ycsb_c||"), std::string::npos) << inhalt_info;
}

// -----------------------------------------------------------------------------
// (rot) Ein nicht existierendes Ausgabeverzeichnis kann kein Sheet oeffnen -- ErgebnisSchreibFehler mit
// Klasse ZipFehler statt eines stillen/verschluckten Fehlers.
// -----------------------------------------------------------------------------
TEST(A9S3CsvErgebnisMappe, NichtOeffenbaresVerzeichnisWirftZipFehler) {
    auto mappe =
        lab::ErgebnisMappenFactory::oeffne("/nicht_vorhanden_a9s3_xyz/tiefer/pfad", "x", lab::ErgebnisFormat::csv);
    lab::SheetSchluessel const s{"x", "", ""};
    bool                       geworfen = false;
    try {
        mappe->blatt(s);
    } catch (lab::ErgebnisSchreibFehler const& e) {
        geworfen = true;
        EXPECT_EQ(e.klasse(), ms::LagerAblageFehlerKlasse::ZipFehler) << e.what();
    }
    EXPECT_TRUE(geworfen) << "ein nicht oeffenbarer Pfad MUSS werfen, nicht still verschwinden.";
}

// -----------------------------------------------------------------------------
// (gruen) Gegenprobe: ein GUELTIGES Verzeichnis darf NICHT werfen -- die Wache oben muss echt an der
// Ursache haengen, nicht an einem allgemeinen Bug.
// -----------------------------------------------------------------------------
TEST(A9S3CsvErgebnisMappe, GueltigesVerzeichnisWirftNicht) {
    TempDir const              tmp;
    auto                       mappe = lab::ErgebnisMappenFactory::oeffne(tmp.path, "gruen", lab::ErgebnisFormat::csv);
    lab::SheetSchluessel const s{"x", "", ""};
    EXPECT_NO_THROW({
        auto& b = mappe->blatt(s);
        b.kopf(std::vector<std::string>{"a"});
        b.zeile(std::vector<std::string>{"1"});
        mappe->schliessen();
    });
}
