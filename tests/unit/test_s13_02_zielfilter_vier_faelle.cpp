// test_s13_02_zielfilter_vier_faelle.cpp -- S13-02 (#18/D-1, KON32-01): der Ziel-Filter
// <writeback_methods> entscheidet die PERSISTENZ -- am ECHTEN run_profile, vier Faelle mit
// literalem Verzeichnis-Listing (Abnahme-Form aus 20260817-DESIGN-s13-buendel-di25.md 4.1).
//
// DIE LUECKE (KON32-01, Owner): der offizielle CSV-Strom (a.out_csv) war ein UNBEDINGTER Kanal.
// Deklarierte ein Profil nur xlsx, entstand trotzdem eine measurements.csv; fehlte der Block
// (Default xlsx, Owner-KERN 26.07.), ebenfalls. Der Filter sass nur an der Mappe (A9-S5), nicht
// am rohen Strom -- zwei Kanaele, einer davon am Filter vorbei.
//
// T-1-PROTOKOLL (ROT ZUERST, 2026-08-21): dieser Test wurde VOR dem S13-01/-02-Umbau geschrieben
// und gegen ce 66de5c09 ausgefuehrt. Faelle (2) nur-xlsx und (4) Block-fehlt bissen ROT (die
// nicht deklarierte measurements.csv lag auf der Platte); Faelle (1) und (3) waren gruen (der
// Mappen-Filter stand bereits). Das literale Rot steht im Strang-Ergebnis
// (backups-workflow/20260820-w2-sofortstaffel/s13-schema-kette-ergebnis.md).
//
// DIE VIER FAELLE (je: eigener Profil-Klon, eigener Lauf-Ordner, LITERALES Listing):
//   (1) nur csv    -> measurements.csv + __S001.csv-Kind, KEINE .xlsx
//   (2) nur xlsx   -> genau EINE .xlsx, KEINE measurements.csv, KEIN __S001.csv
//   (3) beide      -> .xlsx UND measurements.csv (und __S001.csv-Kind)
//   (4) Block FEHLT (Bestandsfall der drei frueheren blocklosen Profile; seit ce fbe48f99 tragen
//       alle 11 getrackten Profile einen Block, deshalb faehrt dieser Fall per Design-Auflage
//       mit einem TEST-LOKALEN Profil) -> Default xlsx: wie Fall (2).
//
// KOEDER JE FALL (Design-Abnahme): eine Fassung, die die NICHT deklarierte Datei doch schreibt,
// wird ROT -- genau das war der Ist-Zustand fuer (2)/(4). Umgekehrt beisst (1), wenn eine Fassung
// die Mappe trotz csv-only auf die Platte schliesst.
//
// WARUM provision_only: der Filter sitzt an der PERSISTENZ-Naht (oeffnen/schliessen), nicht an
// der Mess-Schleife. Ein provision-Lauf durchlaeuft dieselbe Naht (Header-only-Ausgaben) und
// braucht weder Toolchain noch DLL-Load (Muster test_t2a_f4_facade_plan_durchreichung: Stub-
// Compile, max_binaries=1, exit 0 am Objekt bewiesen).
//
// Build: plain main (KEIN gtest), Return 0/1 -- Muster test_t2a_f4_facade_plan_durchreichung.

#include "experiment_run_entry.hpp" // T-6-Spiegel: run_experiment_profile + RunExperimentArgs
#include "profile_run_entry.hpp"    // das geprueft Glied: run_profile + RunProfileArgs

#include "comdare_test_tmp.hpp" // per-User-Temp gegen CI-Kollisionen

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#ifndef COMDARE_S13_THESIS_MIN
#error "COMDARE_S13_THESIS_MIN must point to tests/unit/thesis_tiere/planner_thesis_min.profile.xml"
#endif
#ifndef COMDARE_S13_EXPERIMENT_KERN
#error "COMDARE_S13_EXPERIMENT_KERN must point to tests/unit/thesis_tiere/experiment_kern_seam_fixture.xml"
#endif

namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace fs  = std::filesystem;

namespace {

int g_fail = 0;

void check_true(std::string const& what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

std::string lies_datei(fs::path const& p) {
    std::ifstream is(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
}

/// Profil-Klon mit eingesetztem <writeback_methods>-Block (leer = Block FEHLT, Fall 4).
/// planner_thesis_min traegt keinen Block -- der Klon setzt ihn VOR den schliessenden Root-Tag.
fs::path klon_profil(fs::path const& ziel_dir, std::string const& fall, std::string const& block) {
    std::string text = lies_datei(fs::path{COMDARE_S13_THESIS_MIN});
    if (text.empty()) return {};
    std::string const anker = "</comdare_thesis_profile>";
    auto const        pos   = text.rfind(anker);
    if (pos == std::string::npos) return {};
    if (!block.empty()) text.insert(pos, block + "\n");
    std::error_code ec;
    fs::create_directories(ziel_dir, ec);
    fs::path const p = ziel_dir / (fall + ".profile.xml");
    std::ofstream  os{p, std::ios::trunc | std::ios::binary};
    os << text;
    os.close();
    return os.good() ? p : fs::path{};
}

/// Alle regulaeren Dateien des Ordners -- das LITERALE Listing der Abnahme.
std::vector<std::string> listing(fs::path const& dir) {
    std::vector<std::string> namen;
    std::error_code          ec;
    if (!fs::exists(dir, ec)) return namen;
    for (auto const& e : fs::directory_iterator(dir, ec)) {
        if (e.is_regular_file()) namen.push_back(e.path().filename().string());
    }
    std::sort(namen.begin(), namen.end());
    return namen;
}

std::size_t zaehl_endung(std::vector<std::string> const& namen, std::string const& endung) {
    std::size_t n = 0;
    for (auto const& s : namen)
        if (s.size() >= endung.size() && s.compare(s.size() - endung.size(), endung.size(), endung) == 0) ++n;
    return n;
}

bool enthaelt(std::vector<std::string> const& namen, std::string const& name) {
    return std::find(namen.begin(), namen.end(), name) != namen.end();
}

/// EIN Lauf des echten run_profile: Stub-Compile, provision_only, 1 Binary. Die Naht
/// (oeffnen -> kopf -> schliessen) laeuft vollstaendig; gemessen wird nichts.
tlz::RunProfileResult lauf(fs::path const& profil, fs::path const& lauf_dir) {
    std::error_code ec;
    fs::create_directories(lauf_dir, ec);
    tlz::RunProfileArgs a;
    a.profile_path      = profil;
    a.out_csv           = lauf_dir / "measurements.csv";
    a.src_dir           = lauf_dir / "src";
    a.dll_dir           = lauf_dir / "dll";
    a.compile           = [](ex::BuildJob const&) -> int { return 0; };
    a.provision_only    = true;
    a.run_sota_series   = false;
    a.max_binaries      = 1;
    a.cores_per_build   = 1;
    a.build_parallelism = 1;
    return tlz::run_profile(a);
}

struct FallErwartung {
    std::string fall;
    std::string block;
    bool        csv_erwartet;  ///< measurements.csv MUSS liegen
    bool        xlsx_erwartet; ///< genau EINE .xlsx MUSS liegen
};

} // namespace

int main() {
    std::error_code ec;
    fs::path const  base = ::comdare::test::user_tmp_dir() / "comdare_s13_02_zielfilter";
    fs::remove_all(base, ec); // Reste eines Vorlaufs: die Abwesenheits-Faelle duerfen nichts erben
    fs::create_directories(base, ec);

    std::cout << "== S13-02: <writeback_methods> steuert die PERSISTENZ -- vier Faelle ==\n";

    // Die Grundgesamtheit der Faelle ist die FALL-MATRIX des Designs (4.1), nicht dieser Test.
    std::vector<FallErwartung> const faelle{
        {"fall1_nur_csv", "  <writeback_methods>\n    <method value=\"csv\"/>\n  </writeback_methods>", true, false},
        {"fall2_nur_xlsx", "  <writeback_methods>\n    <method value=\"xlsx\"/>\n  </writeback_methods>", false, true},
        {"fall3_beide",
         "  <writeback_methods>\n    <method value=\"csv\"/>\n    <method value=\"xlsx\"/>\n  </writeback_methods>",
         true, true},
        {"fall4_block_fehlt", "", false, true}, // Default xlsx (Owner-KERN 26.07.)
    };

    for (auto const& f : faelle) {
        std::cout << "-- " << f.fall << " --\n";
        fs::path const profil = klon_profil(base / "profile", f.fall, f.block);
        check_true(f.fall + ": Profil-Klon liegt", !profil.empty());
        if (profil.empty()) continue;

        fs::path const lauf_dir = base / f.fall;
        auto const     r        = lauf(profil, lauf_dir);
        check_true(f.fall + ": der Lauf endet regulaer (exit 0)", r.exit_code == 0);

        auto const namen = listing(lauf_dir);
        std::cout << "  [LISTING " << f.fall << "] " << namen.size() << " Datei(en):";
        for (auto const& n : namen) std::cout << " " << n;
        std::cout << "\n";

        bool const        csv_da = enthaelt(namen, "measurements.csv");
        std::size_t const xlsx_n = zaehl_endung(namen, ".xlsx");
        std::size_t const kind_n = zaehl_endung(namen, "__S001.csv");

        if (f.csv_erwartet) {
            check_true(f.fall + ": measurements.csv liegt (deklariert: csv)", csv_da);
            check_true(f.fall + ": das __S001.csv-Kind der Mappe liegt", kind_n == 1);
        } else {
            // DER KOEDER DES FALLES: die NICHT deklarierte Datei darf NICHT liegen. Vor S13-01/-02
            // schrieb der rohe Strom sie bedingungslos -- genau hier biss das T-1-Rot.
            check_true(f.fall + ": KEINE measurements.csv (csv NICHT deklariert)", !csv_da);
            check_true(f.fall + ": KEIN __S001.csv-Kind (csv NICHT deklariert)", kind_n == 0);
        }
        if (f.xlsx_erwartet) {
            check_true(f.fall + ": genau EINE .xlsx liegt (deklariert bzw. Default)", xlsx_n == 1);
        } else {
            check_true(f.fall + ": KEINE .xlsx (xlsx NICHT deklariert)", xlsx_n == 0);
        }
    }

    // ============================================================================================
    // T-6-SPIEGEL: DIESELBE Wirkung an der ZWEITEN Naht (run_experiment_profile). Zwei Faelle
    // genuegen als Biss (nur-xlsx = der Koeder-Fall, nur-csv = der Positiv-Fall); die volle
    // Vier-Fall-Matrix oben laeuft an der Profil-Naht, die Naht-Mechanik selbst ist geteilt
    // (MappenNaht). Der Experiment-Lauf misst hier NICHTS (Stub-Compile -> keine ladbare DLL,
    // exit deshalb bewusst NICHT geprueft) -- geprueft wird ausschliesslich die PERSISTENZ-Spur
    // im Lauf-Ordner, und die entsteht an der Naht VOR und NACH der Mess-Schleife.
    // ============================================================================================
    struct ExpFall {
        std::string fall;
        std::string block;
        bool        csv_erwartet;
    };
    std::vector<ExpFall> const exp_faelle{
        {"exp_nur_xlsx", "  <writeback_methods>\n    <method value=\"xlsx\"/>\n  </writeback_methods>", false},
        {"exp_nur_csv", "  <writeback_methods>\n    <method value=\"csv\"/>\n  </writeback_methods>", true},
    };
    for (auto const& f : exp_faelle) {
        std::cout << "-- " << f.fall << " (run_experiment_profile) --\n";
        std::string text = lies_datei(fs::path{COMDARE_S13_EXPERIMENT_KERN});
        check_true(f.fall + ": Experiment-Fixture lesbar", !text.empty());
        if (text.empty()) continue;
        std::string const anker = "</comdare_experiment>";
        auto const        pos   = text.rfind(anker);
        check_true(f.fall + ": Root-Schliess-Tag gefunden", pos != std::string::npos);
        if (pos == std::string::npos) continue;
        text.insert(pos, f.block + "\n");
        std::error_code ec2;
        fs::create_directories(base / "profile", ec2);
        fs::path const profil = base / "profile" / (f.fall + ".xml");
        {
            std::ofstream os{profil, std::ios::trunc | std::ios::binary};
            os << text;
        }
        fs::path const lauf_dir = base / f.fall;
        fs::create_directories(lauf_dir, ec2);
        tlz::RunExperimentArgs ea;
        ea.profile_path = profil;
        ea.out_csv      = lauf_dir / "measurements.csv";
        ea.src_dir      = lauf_dir / "src";
        ea.dll_dir      = lauf_dir / "dll";
        ea.compile      = [](ex::BuildJob const&) -> int { return 0; };
        ea.max_binaries = 1;
        (void)tlz::run_experiment_profile(ea); // exit hier ohne Aussage: nichts messbar (s. Kopf)

        auto const namen = listing(lauf_dir);
        std::cout << "  [LISTING " << f.fall << "] " << namen.size() << " Datei(en):";
        for (auto const& n : namen) std::cout << " " << n;
        std::cout << "\n";
        bool const csv_da = enthaelt(namen, "measurements.csv");
        if (f.csv_erwartet) {
            check_true(f.fall + ": measurements.csv liegt (deklariert: csv)", csv_da);
        } else {
            check_true(f.fall + ": KEINE measurements.csv (csv NICHT deklariert)", !csv_da);
            check_true(f.fall + ": KEIN __S001.csv-Kind", zaehl_endung(namen, "__S001.csv") == 0);
        }
    }

    if (g_fail == 0) {
        std::cout << "ALLE PRUEFUNGEN GRUEN (4 Faelle Profil-Naht + 2 Faelle Experiment-Naht)\n";
        std::error_code cec;
        fs::remove_all(base, cec);
        return 0;
    }
    std::cout << g_fail << " PRUEFUNG(EN) ROT -- Artefakte liegen unter " << base.string() << "\n";
    return 1;
}
