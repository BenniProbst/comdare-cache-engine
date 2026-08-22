// test_lg_e2e_xlsx_facade -- B06 (K01/F2-5, Designplan par.4 Z56 "LG-E2Exlsx Facade", Klasse
// XL-L1-Familie "test laeuft nie"): ENDE-ZU-ENDE ueber die profile_run_facade bis zur
// xlsx-Ergebnis-Mappe.
//
// VORBESTAND (Anrechnung statt Doppelung): die xlsx-KETTE existiert und ist getestet --
// test_a9s5_ergebnis_mappe_naht prueft die NAHT isoliert (ZIP-Signatur, Zellwerte, csv+xlsx aus
// EINER Mappe), test_a9s3_* pruefen Writer/Registry. Was FEHLTE, ist der Beweis ueber die FACADE:
// run_profile(Args) -> die Mappe entsteht wirklich als .xlsx-Datei ("xlsx IST DIE
// AUSGABE", Owner-KERN 26.07.: <writeback_methods> LEER/FEHLEND => xlsx).
//
// LANDE-HARMONISIERUNG (W2-Zug 4/10, KON32-01/S13-02): seit dem s13-Ziel-Filter heisst
// LEER/FEHLEND => NUR xlsx -- eine NICHT deklarierte measurements.csv darf NICHT mehr
// entstehen (der fruehere rohe CSV-Strom war exakt die vom Owner als falsch deklarierte
// KON32-01-Luecke). Dieser Test prueft die Abwesenheit; die CSV-MIT-Kopf-Substanz deckt
// seither test_s13_02_zielfilter_vier_faelle (Faelle 1/3, deklarierte CSV) und
// test_s13_01_csv_kind_projektion (Byte-Orakel der Projektion).
//
// FAHRWEG (Muster test_t2a_f4_facade_plan_durchreichung, derselbe leichte Lauf): Mini-Profil
// planner_thesis_min, Stub-Compile (0 == Erfolg), provision_only, max_binaries=1 -- KEIN echter
// Bau, KEINE Messung; die Mappe haengt am gemeinsamen Pfad (profile_run_entry: mappe.oeffnen NACH
// CSV-Kopf) und wird beim Schliessen geschrieben. TABU-DISZIPLIN: ergebnis_mappe_naht.hpp und
// profile_run_entry.hpp werden NUR KONSUMIERT (kein Edit -- Kollisions-Tabu des Strangs).
//
// NENNER (literal, je mit fremder Quelle):
//   * Datei-Nenner: GENAU EINE .xlsx im Lauf-Verzeichnis (dateien_mit_endung).
//   * Blatt-Nenner: GENAU ZWEI Sheets -- Owner-Kanon (ergebnis_mappe.hpp:7-8): "xlsx = EINE Datei
//     mit EINEM Sheet je gewaehlter Unter-Achsen-Permutation + zusaetzlichem INFO-Sheet"; die
//     Kopf-Mappe traegt EIN Daten-Sheet + das INFO-Sheet. Der ZIP-Traeger nennt jeden
//     "xl/worksheets/sheet"-Eintrag zweimal (Local Header + Central Directory) -> 4 Treffer.
//   * CSV-Nenner (S13-02): das Mini-Profil deklariert KEINE writeback_methods -> es darf
//     KEINE measurements.csv liegen (Default = NUR xlsx); die Mappe entsteht trotzdem
//     (Kopf-Mappe, A9-S5-Doktrin "geschrieben wird in schliessen()").
//   * Stamm-Nenner: die .xlsx liegt im Lauf-Verzeichnis und beginnt mit der
//     ZIP-Signatur PK\x03\x04 (eine xlsx IST ein ZIP -- dieselbe Probe wie test_a9s5).
//
// T-11c-MUTATIONSANKER: den Writer-Anschluss des Tests wegmutieren (provision_only-Lauf ohne
// Mappen-Erwartung faehrt gegen ein GELOESCHTES Lauf-Verzeichnis) -> Datei-Nenner bricht literal.

#include "profile_run_entry.hpp" // run_profile + RunProfileArgs (Facade-Eintritt; NUR Konsum)

#include "comdare_test_tmp.hpp" // per-User-Temp gegen CI-Kollisionen

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifndef COMDARE_LG_E2E_PROFIL
#error "COMDARE_LG_E2E_PROFIL must point to tests/unit/thesis_tiere/planner_thesis_min.profile.xml"
#endif

namespace {

namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace fs  = std::filesystem;

[[nodiscard]] std::vector<std::string> dateien_mit_endung(fs::path const& dir, std::string const& endung) {
    std::vector<std::string> out;
    std::error_code          ec;
    for (auto const& e : fs::directory_iterator{dir, ec}) {
        auto const name = e.path().filename().string();
        if (name.size() >= endung.size() && name.compare(name.size() - endung.size(), endung.size(), endung) == 0)
            out.push_back(name);
    }
    return out;
}

[[nodiscard]] std::string read_file(fs::path const& p) {
    std::ifstream in{p, std::ios::binary};
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

[[nodiscard]] std::size_t zaehle_teilstring(std::string const& heu, std::string const& nadel) {
    std::size_t n = 0;
    for (std::size_t pos = heu.find(nadel); pos != std::string::npos; pos = heu.find(nadel, pos + 1)) ++n;
    return n;
}

TEST(LgE2eXlsxFacade, RunProfileErzeugtDieMappeAlsXlsxOhneUndeklarierteCsv) {
    std::error_code ec;
    fs::path const  base = ::comdare::test::user_tmp_dir() / "comdare_lg_e2e_xlsx";
    fs::remove_all(base, ec);
    fs::path const lauf = base / "lauf1";
    fs::create_directories(lauf, ec);

    tlz::RunProfileArgs a;
    a.profile_path      = fs::path{COMDARE_LG_E2E_PROFIL};
    a.out_csv           = lauf / "measurements.csv";
    a.src_dir           = lauf / "src";
    a.dll_dir           = lauf / "dll";
    a.compile           = [](ex::BuildJob const&) -> int { return 0; }; // Stub: Ablage ist die Frage
    a.provision_only    = true;
    a.run_sota_series   = false;
    a.max_binaries      = 1;
    a.cores_per_build   = 1;
    a.build_parallelism = 1;

    tlz::RunProfileResult const r = tlz::run_profile(a);
    ASSERT_EQ(r.exit_code, 0) << "der leichte provision_only-Lauf muss regulaer enden";

    // CSV-Nenner (LANDE-HARMONISIERUNG, KON32-01/S13-02): das Mini-Profil deklariert KEINE
    // writeback_methods -> Default = NUR xlsx; die NICHT deklarierte measurements.csv darf
    // NICHT liegen (der Alt-Kontrakt "CSV entsteht immer" war die KON32-01-Luecke; die
    // CSV-MIT-Kopf-Substanz deckt test_s13_02 Faelle 1/3 + test_s13_01 mit deklarierter CSV).
    EXPECT_FALSE(fs::exists(a.out_csv)) << "nicht deklarierte measurements.csv darf NICHT entstehen (S13-02)";

    // Datei-Nenner: GENAU EINE .xlsx neben der CSV ("xlsx IST DIE AUSGABE"; writeback fehlt im
    // Mini-Profil => xlsx-Default).
    auto const xlsx = dateien_mit_endung(lauf, ".xlsx");
    ASSERT_EQ(xlsx.size(), 1u) << "die Facade muss die Ergebnis-Mappe als EINE .xlsx ablegen";

    // ZIP-Signatur (eine xlsx IST ein ZIP -- dieselbe Probe wie test_a9s5).
    std::string const bytes = read_file(lauf / xlsx.front());
    ASSERT_GE(bytes.size(), 4u);
    EXPECT_EQ(bytes.compare(0, 4, "PK\x03\x04"), 0) << "keine ZIP-Signatur -- das ist keine xlsx";

    // Blatt-Nenner: genau ZWEI Sheets (Daten-Sheet + INFO-Sheet, Owner-Kanon ergebnis_mappe.hpp:7-8)
    // => der Eintragsname erscheint VIERMAL (Local Header + Central Directory je Blatt;
    // ZIP-Eintragsnamen reisen unkomprimiert im Traeger).
    EXPECT_EQ(zaehle_teilstring(bytes, "xl/worksheets/sheet"), 4u)
        << "Blatt-Nenner: Daten-Sheet + INFO-Sheet erwartet (2 Traeger-Nennungen je Blatt)";

    // Wiederholungs-Stabilitaet: ein zweiter Lauf in ein FRISCHES Verzeichnis traegt denselben
    // Datei-Nenner (kein Zufalls-Artefakt des ersten Laufs).
    fs::path const lauf2 = base / "lauf2";
    fs::create_directories(lauf2, ec);
    a.out_csv = lauf2 / "measurements.csv";
    a.src_dir = lauf2 / "src";
    a.dll_dir = lauf2 / "dll";
    ASSERT_EQ(tlz::run_profile(a).exit_code, 0);
    EXPECT_EQ(dateien_mit_endung(lauf2, ".xlsx").size(), 1u);
    EXPECT_FALSE(fs::exists(a.out_csv)) << "auch im Wiederholungslauf: keine undeklarierte CSV (S13-02)";
}

} // namespace
