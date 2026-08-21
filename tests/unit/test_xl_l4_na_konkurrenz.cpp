// test_xl_l4_na_konkurrenz -- B08 (K01/F2-5, Designplan par.4 Z61 "XL-L4 n/a-Konkurrenz",
// Einzelfall "kennzahl-ohne-nicht-bestimmbar"): eine KONKURRIEREND belegte Haupt-Achse muss im
// INFO-Blatt als NICHT BESTIMMBAR ("n/a") ausgewiesen werden -- nicht als eine der beiden Zahlen
// und nicht als zwei widerspruechliche Zeilen.
//
// BEFUND (am Objekt, 2026-08-21): beide Backends reichten die HauptAchsenBelegung 1:1 durch
// (xlsx_ergebnis_writer.cpp "for (auto const& e : haupt_) info.zeile(...)"; ergebnis_mappe.hpp
// CSV-Zwilling "info << \"hauptachse;\" ..."). Traegt dieselbe Achse zwei ABWEICHENDE Werte
// (zwei Quellen, zwei Laeufe), standen ZWEI widerspruechliche Zeilen im Blatt -- der Leser kann
// nicht wissen, welche gilt; die Kennzahl kannte keinen n/a-Zustand.
//
// HEILFORM (Mini-Bau, honest-empty-Vokabular): der geteilte Helfer
// hauptachsen_konkurrenz_ausweis() (ergebnis_mappe.hpp) kollabiert NUR den echten Konkurrenzfall
// (gleiche Achse, ABWEICHENDER Wert) auf EINE Zeile mit dem Zell-Token "n/a" (Section-16.2-M4-
// Konvention; Token-Treue: "n/a" parst nie als Zahl -> Text-Zweig). IDENTISCHE Wiederholungen
// sind KEINE Konkurrenz und bleiben byte-unveraendert -- ebenso jede konkurrenzfreie Belegung
// (Byte-Probe im Normalfall-Test).
//
// T-11c-MUTATIONSANKER: den n/a-Zweig des Helfers wegmutieren (Durchreichung wie vorher) bricht
// KonkurrenzFallTraegtNaStattZahl literal.

#include "lager_ablage/ergebnis_mappe.hpp" // Pruefling: Helfer + CsvErgebnisMappe (via Factory)

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

namespace lab = ::comdare::cache_engine::builder::lager_ablage;
namespace fs  = std::filesystem;

struct TempDir {
    fs::path path;
    TempDir() {
        path = fs::temp_directory_path() / ("comdare_xl_l4_" + std::to_string(::rand()));
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

[[nodiscard]] std::string read_file(fs::path const& p) {
    std::ifstream in{p, std::ios::binary};
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

[[nodiscard]] std::size_t zaehle(std::string const& heu, std::string const& nadel) {
    std::size_t n = 0;
    for (auto pos = heu.find(nadel); pos != std::string::npos; pos = heu.find(nadel, pos + 1)) ++n;
    return n;
}

TEST(XlL4NaKonkurrenz, HelferKollabiertNurDenEchtenKonkurrenzfall) {
    // Direkt am geteilten Helfer, drei Eingaenge:
    // (1) KONKURRENZ: gleiche Achse, abweichende Werte -> EINE Zeile "n/a".
    lab::HauptAchsenBelegung konkurrenz{{"target_isa", "avx2"}, {"target_isa", "avx512"}};
    auto const               k = lab::hauptachsen_konkurrenz_ausweis(konkurrenz);
    ASSERT_EQ(k.size(), 1u) << "Konkurrenz muss auf EINEN Ausweis kollabieren";
    EXPECT_EQ(k.front().achse, "target_isa");
    EXPECT_EQ(k.front().wert, "n/a") << "der Ausweis ist das honest-empty-Token, keine der Zahlen";

    // (2) NORMALFALL: zwei verschiedene Achsen -> byte-unveraendert.
    lab::HauptAchsenBelegung normal{{"measurement_tooling", "wallclock"}, {"target_isa", "amd64_v3"}};
    auto const               n = lab::hauptachsen_konkurrenz_ausweis(normal);
    ASSERT_EQ(n.size(), 2u);
    EXPECT_EQ(n[0].achse, "measurement_tooling");
    EXPECT_EQ(n[0].wert, "wallclock");
    EXPECT_EQ(n[1].achse, "target_isa");
    EXPECT_EQ(n[1].wert, "amd64_v3");

    // (3) IDENTISCHE WIEDERHOLUNG ist KEINE Konkurrenz -> unveraendert (2 Zeilen, gleicher Wert).
    lab::HauptAchsenBelegung doppelt{{"target_isa", "avx2"}, {"target_isa", "avx2"}};
    auto const               d = lab::hauptachsen_konkurrenz_ausweis(doppelt);
    ASSERT_EQ(d.size(), 2u) << "identische Wiederholung darf NICHT kollabieren (kein Konkurrenzfall)";
    EXPECT_EQ(d[0].wert, "avx2");
    EXPECT_EQ(d[1].wert, "avx2");
}

TEST(XlL4NaKonkurrenz, KonkurrenzFallTraegtNaStattZahlImInfoBlatt) {
    // E2E am CSV-Backend (dieselbe info_blatt-Schnittstelle wie xlsx; Muster test_a9s3).
    TempDir const tmp;
    auto          mappe = lab::ErgebnisMappenFactory::oeffne(tmp.path, "xl_l4", lab::ErgebnisFormat::csv);
    ASSERT_NE(mappe, nullptr);
    lab::MaschinenSysinfo    sys{};
    lab::HauptAchsenBelegung haupt{{"target_isa", "avx2"}, {"target_isa", "avx512"}, {"filter", "bloom"}};
    mappe->info_blatt(sys, haupt, {});
    auto& b = mappe->blatt(lab::SheetSchluessel{"ycsb_a", "", ""});
    b.kopf(std::vector<std::string>{"binary_id"});
    mappe->schliessen();

    std::string const info = read_file(tmp.path / "xl_l4__INFO.csv");
    EXPECT_NE(info.find("hauptachse;target_isa;n/a\n"), std::string::npos)
        << "die konkurrierende Achse muss als n/a ausgewiesen sein:\n"
        << info;
    EXPECT_EQ(info.find("hauptachse;target_isa;avx2"), std::string::npos)
        << "keine der konkurrierenden Zahlen darf als Belegung stehen:\n"
        << info;
    EXPECT_EQ(info.find("hauptachse;target_isa;avx512"), std::string::npos) << info;
    EXPECT_EQ(zaehle(info, "hauptachse;target_isa;"), 1u) << "genau EIN Ausweis je konkurrierender Achse";
    EXPECT_NE(info.find("hauptachse;filter;bloom\n"), std::string::npos)
        << "die konkurrenzfreie Nachbar-Achse bleibt unveraendert:\n"
        << info;
}

TEST(XlL4NaKonkurrenz, NormalfallBleibtByteIdentisch) {
    // Byte-Probe der bestehenden Golden-Form (dieselben Zeilen wie test_a9s3 sie pinnt):
    // eine konkurrenzfreie Belegung rendert EXAKT wie vor dem XL-L4-Bau.
    TempDir const tmp;
    auto          mappe = lab::ErgebnisMappenFactory::oeffne(tmp.path, "xl_l4_normal", lab::ErgebnisFormat::csv);
    ASSERT_NE(mappe, nullptr);
    lab::MaschinenSysinfo    sys{};
    lab::HauptAchsenBelegung haupt{{"measurement_tooling", "wallclock"}, {"target_isa", "amd64_v3"}};
    mappe->info_blatt(sys, haupt, {});
    auto& b = mappe->blatt(lab::SheetSchluessel{"ycsb_a", "", ""});
    b.kopf(std::vector<std::string>{"binary_id"});
    mappe->schliessen();

    std::string const info = read_file(tmp.path / "xl_l4_normal__INFO.csv");
    EXPECT_NE(info.find("hauptachse;measurement_tooling;wallclock\n"), std::string::npos) << info;
    EXPECT_NE(info.find("hauptachse;target_isa;amd64_v3\n"), std::string::npos) << info;
    EXPECT_EQ(zaehle(info, "hauptachse;"), 2u) << "keine Zeile verschwindet, keine kommt hinzu";
    // Kein hauptachse-n/a im Normalfall (sysinfo-n/a-Zeilen sind die ANDERE, alte "leer"-Doktrin):
    EXPECT_EQ(info.find("hauptachse;measurement_tooling;n/a"), std::string::npos) << info;
    EXPECT_EQ(info.find("hauptachse;target_isa;n/a"), std::string::npos) << info;
}

} // namespace
