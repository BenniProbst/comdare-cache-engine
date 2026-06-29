// test_pruefling_type_pilot — #171 (Text-Agent AP-X2/TODO-2, 2026-06-20). Der LITERALE Klein-Pilot fuer die
// additive Pruefling-Typ-Spalte "pruefling_type" (full = Reihe A self-contained / "Originalkonfiguration";
// abstract = Reihe B/C Teilmenge + Host-Fallback). Doc 14 §18-§19 (Pruefling-Slot-Pattern + 3 Kompositionale
// Joins) + cacheline-doc §0/§1.2 ("Paper-Algorithmen als Basis-Lebewesen in Originalkonfiguration").
//
// BEWEIST LITERAL (ohne den HELDen Voll-DLL-Mess-Lauf, header-getrieben, additiv):
//   (1) derive_pruefling_type mappt die BESTEHENDE merge-Mechanik deterministisch: Stufe1_CeOnly→full,
//       Stufe2_PrueflingReplace/Stufe3_FullJoin→abstract (Single-Source, kein neuer Selektions-Pfad).
//   (2) build_sota_passes(m3v2) taggt JEDEN Pass mit pruefling_type — PRT-ART existiert in BEIDEN
//       Auspraegungen (Reihe A = full, Reihe B/C = abstract): beide Varianten je Pruefling erzeugbar.
//   (3) lazy_csv_header endet GANZ auf "pruefling_type" (additiv ans Ende, series/sweep_axis-Muster).
//   (4) format_csv_row emittiert die letzte Spalte = full / abstract / "-" (Basis) — die Auswertung kann
//       Original- vs rekombinierte Konfiguration je Messreihe trennen. Eine cowfix-v1-artige Zeile (Feld
//       leer) emittiert "-" (Datenerhaltung: alte CSV ohne die Spalte liest sie leer/n-a).
//   (5) Die EINE SEPARATE Pilot-CSV (pruefling_type_pilot.csv) traegt je >=1 full- + abstract-Zeile —
//       cowfix-v1-Backup + getrackte tier150_measurements.csv werden NIE beruehrt.
//
// KEIN cl-DLL-Bau noetig: die Spalte reist als REINES Tag durch die EXAKT gleiche CSV-Mechanik wie die
// Mess-Zeilen (format_csv_row). Der reale Voll-DLL-Mess-Lauf (M3, #156) fuehrt run_profile, das exakt diese
// cfg.row_pruefling_type → row.pruefling_type → format_csv_row-Kette nutzt (hier 1:1 belegt).

#include "sota_catalog.hpp"   // build_sota_passes / derive_pruefling_type / SotaPass (#171)
#include "profile_runner.hpp" // load_thesis_profile

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // lazy_csv_header / format_csv_row / LazyMeasuredRow

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace tlz = comdare::cache_engine::thesis_lazy;
namespace ex  = comdare::cache_engine::builder::experiment;
namespace fs  = std::filesystem;

static int  g_fail = 0;
static void check(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

// Letzte ';'-getrennte Spalte einer CSV-Zeile (ohne trailing '\n').
static std::string last_col(std::string const& csv_line) {
    std::string s = csv_line;
    if (!s.empty() && s.back() == '\n') s.pop_back();
    auto const p = s.rfind(';');
    return (p == std::string::npos) ? s : s.substr(p + 1);
}

int main(int argc, char** argv) {
    fs::path const profiles_dir = fs::path("libs") / "cache_engine" / "algorithm_profiles" / "thesis_profiles";
    fs::path const m3v2_xml     = (argc >= 2) ? fs::path(argv[1]) : (profiles_dir / "m3v2_study.profile.xml");
    fs::path const work = (argc >= 3) ? fs::path(argv[2]) : fs::temp_directory_path() / "comdare_pruefling_type_pilot";
    fs::create_directories(work);
    std::cout << "Profil: " << m3v2_xml.string() << "\nWork:   " << work.string() << "\n";

    // ── (1) Die DETERMINISTISCHE Ableitung aus merge (Single-Source). ──
    std::cout << "\n--- (1) derive_pruefling_type (merge → full/abstract) ---\n";
    check("Stufe1_CeOnly → full", tlz::derive_pruefling_type("A", "Stufe1_CeOnly", "") == "full");
    check("Stufe2_PrueflingReplace → abstract",
          tlz::derive_pruefling_type("B", "Stufe2_PrueflingReplace", "") == "abstract");
    check("Stufe3_FullJoin → abstract", tlz::derive_pruefling_type("C", "Stufe3_FullJoin", "") == "abstract");
    check("explizites Override gewinnt", tlz::derive_pruefling_type("A", "Stufe1_CeOnly", "abstract") == "abstract");
    check("Fallback series-id A → full (merge leer)", tlz::derive_pruefling_type("A", "", "") == "full");

    // ── (2) build_sota_passes(m3v2): jeder Pass getaggt; PRT-ART in BEIDEN Auspraegungen. ──
    auto tp = tlz::load_thesis_profile(m3v2_xml);
    check("parse_thesis_profile lieferte das m3v2-Profil", tp.has_value());
    if (!tp) {
        std::cout << "\n==== ABBRUCH: Profil nicht lesbar ====\n";
        return 1;
    }

    std::vector<tlz::SotaPass> const passes = tlz::build_sota_passes(*tp);
    std::cout << "\n--- (2) build_sota_passes = " << passes.size() << " getaggte Paesse ---\n";
    check("mind. ein Pass real baubar", !passes.empty());

    tlz::SotaPass const* prt_full     = nullptr; // PRT-ART Stufe1 = full (Reihe A, self-contained)
    tlz::SotaPass const* prt_abstract = nullptr; // PRT-ART Stufe2/3 = abstract (Reihe A/B, Teilmenge + Fallback)
    for (auto const& p : passes) {
        // #178: pruefling_type ist die mechanische Wahrheit (aus merge) — NICHT mehr an die Reihe gekoppelt
        // (Stufe2-abstract liegt jetzt in Reihe A, Stufe3-abstract in Reihe B).
        if (p.lebewesen == "prt_art" && p.pruefling_type == "full") prt_full = &p;
        if (p.lebewesen == "prt_art" && p.pruefling_type == "abstract") prt_abstract = &p;
    }
    check("PRT-ART als FULL (Stufe1 self-contained / Originalkonfiguration, Reihe A) vorhanden", prt_full != nullptr);
    check("PRT-ART als ABSTRACT (Stufe2/3 Teilmenge + Host-Fallback) vorhanden", prt_abstract != nullptr);

    // ── (3) lazy_csv_header endet auf pruefling_type. ──
    std::string const hdr       = ex::lazy_csv_header();
    std::string       hdr_noeol = hdr;
    if (!hdr_noeol.empty() && hdr_noeol.back() == '\n') hdr_noeol.pop_back();
    std::cout << "\n--- (3) lazy_csv_header letzte Spalte = '" << last_col(hdr) << "' ---\n";
    check("Header endet GANZ auf 'pruefling_type' (additiv ans Ende)", last_col(hdr) == "pruefling_type");

    // ── (4)+(5) format_csv_row: je 1 full- + abstract-Zeile + 1 Basis-Zeile (leer→'-') in EINE SEPARATE CSV. ──
    auto make_row = [&](tlz::SotaPass const* p, char const* fallback_bid) {
        ex::LazyMeasuredRow r;
        r.binary_id       = (p != nullptr) ? p->view_binary_id : std::string{fallback_bid};
        r.setting_label   = "concurrency.thread_count=1/repetition.repetition_index=0";
        r.n_ops           = 1000;
        r.total_ns        = 123456;
        r.timed_ops       = 1000;
        r.two_phase_valid = true; // Pilot: Tag-Beleg (der reale Mess-Lauf setzt dies aus der Messung)
        r.series          = (p != nullptr) ? p->series : std::string{"-"};
        r.pruefling_type  = (p != nullptr) ? p->pruefling_type : std::string{}; // #171
        r.working_set_n   = 16384;
        return r;
    };
    ex::LazyMeasuredRow const row_full = make_row(prt_full, "sota_tier=sota::A::prt_art");
    ex::LazyMeasuredRow const row_abstract =
        make_row(prt_abstract, "sota_tier=sota::A::prt_art"); // #178: abstract (Stufe2) liegt in Reihe A
    ex::LazyMeasuredRow row_basis = make_row(nullptr, "search_algo=art/path_compression=none"); // leer → "-"

    std::string const csv_full     = ex::format_csv_row(row_full);
    std::string const csv_abstract = ex::format_csv_row(row_abstract);
    std::string const csv_basis    = ex::format_csv_row(row_basis);

    std::cout << "\n--- (4) format_csv_row letzte Spalte je Zeile ---\n";
    std::cout << "  full-Zeile     pruefling_type = '" << last_col(csv_full) << "'\n";
    std::cout << "  abstract-Zeile pruefling_type = '" << last_col(csv_abstract) << "'\n";
    std::cout << "  basis-Zeile    pruefling_type = '" << last_col(csv_basis) << "' (leer → '-')\n";
    check("full-Zeile traegt pruefling_type=full", last_col(csv_full) == "full");
    check("abstract-Zeile traegt pruefling_type=abstract", last_col(csv_abstract) == "abstract");
    check("basis-Zeile traegt pruefling_type=- (Datenerhaltung)", last_col(csv_basis) == "-");
    check("two_phase_valid=1 in der full-Zeile", csv_full.find(";1;") != std::string::npos);

    // ── (5) Die EINE SEPARATE Pilot-CSV (NIE cowfix-v1/tier150 beruehren). ──
    fs::path const out_csv = work / "pruefling_type_pilot.csv";
    {
        std::ofstream csv{out_csv.string(), std::ios::trunc};
        csv << hdr;
        csv << csv_full;
        csv << csv_abstract;
        csv << csv_basis;
    }
    std::cout << "\n--- (5) SEPARATE Pilot-CSV geschrieben: " << out_csv.string() << " ---\n";
    // Literal die 3 Daten-Zeilen ohne Header ausgeben (die LITERALE Belegzeile fuers Log).
    std::cout << "  ZEILE full     : ..." << "pruefling_type=" << last_col(csv_full)
              << "  (binary_id=" << row_full.binary_id << ")\n";
    std::cout << "  ZEILE abstract : ..." << "pruefling_type=" << last_col(csv_abstract)
              << "  (binary_id=" << row_abstract.binary_id << ")\n";
    check("Pilot-CSV existiert", fs::exists(out_csv));

    std::cout << "\n==== #171 pruefling_type-Pilot: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
