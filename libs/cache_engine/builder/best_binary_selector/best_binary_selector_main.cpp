// best_binary_selector_main — CLI-Frontend (Versprechen 1: beste-Binary-Auswahl + Versand).
//
// Aufruf:
//   best_binary_selector --csv <mess.csv> --tiere <tiere-dir> --out <out-dir>
//                        [--metric ns_per_op|insert|lookup|erase|scan|rmw]
//                        [--top N]   [--name <artifact-name>]
//
// Liest die Mess-CSV, rankt je gewählter Metrik (Strategy) über two_phase_valid-Zeilen, findet die
// reale perm.dll des Gewinners (Repository) und liefert DLL + .version + Manifest aus (Builder).
#include "best_binary_selector.hpp"

#include <cstdio>
#include <iostream>
#include <string>

namespace bb = comdare::cache_engine::best_binary;

namespace {

void usage() {
    std::cout <<
        "best_binary_selector — automatische Auswahl + Versand der besten Tier-Binary\n"
        "  --csv <file>     Mess-CSV (lazy_csv_header WIDE-Schema)            [pflicht]\n"
        "  --tiere <dir>    tiere/-Baum (build/thesis_tiere/tiere)           [pflicht]\n"
        "  --out <dir>      Ausgabe-Verzeichnis fuer das Versand-Artefakt    [pflicht]\n"
        "  --metric <m>     ns_per_op|insert|lookup|erase|scan|rmw  (default ns_per_op)\n"
        "  --top <N>        N beste Kandidaten listen (default 5)\n"
        "  --name <s>       Basisname des Artefakts (default best_<metric>)\n";
}

std::string arg_after(int argc, char** argv, int& i) {
    if (i + 1 >= argc) return {};
    return argv[++i];
}

}  // namespace

int main(int argc, char** argv) {
    std::string csv_path, tiere_dir, out_dir, metric_str = "ns_per_op", art_name;
    int top_n = 5;

    for (int i = 1; i < argc; ++i) {
        std::string const a = argv[i];
        if      (a == "--csv")    csv_path   = arg_after(argc, argv, i);
        else if (a == "--tiere")  tiere_dir  = arg_after(argc, argv, i);
        else if (a == "--out")    out_dir    = arg_after(argc, argv, i);
        else if (a == "--metric") metric_str = arg_after(argc, argv, i);
        else if (a == "--name")   art_name   = arg_after(argc, argv, i);
        else if (a == "--top")    { try { top_n = std::stoi(arg_after(argc, argv, i)); } catch (...) {} }
        else if (a == "--help" || a == "-h") { usage(); return 0; }
        else { std::cerr << "Unbekanntes Argument: " << a << "\n"; usage(); return 2; }
    }
    if (csv_path.empty() || tiere_dir.empty() || out_dir.empty()) { usage(); return 2; }

    auto const metric_opt = bb::metric_from_string(metric_str);
    if (!metric_opt) { std::cerr << "Unbekannte Metrik: " << metric_str << "\n"; return 2; }
    bb::Metric const metric = *metric_opt;
    if (art_name.empty()) art_name = "best_" + bb::metric_name(metric);

    // 1) CSV einlesen.
    std::vector<bb::MeasurementRow> rows;
    int const n = bb::parse_measurement_csv(csv_path, rows);
    if (n < 0) { std::cerr << "CSV konnte nicht gelesen werden: " << csv_path << "\n"; return 1; }
    std::cout << "[csv]    " << n << " Datenzeilen gelesen aus " << csv_path << "\n";

    // 2) Rangbildung (Strategy = gewaehlte Metrik).
    bb::RankingCriterion crit{metric};
    auto const ranked = bb::rank_binaries(rows, crit);
    if (ranked.empty()) {
        std::cerr << "Keine two_phase_valid-Zeilen mit Metrik '" << bb::metric_name(metric) << "' (>0).\n";
        return 1;
    }

    std::cout << "[rank]   Kriterium=" << bb::metric_name(metric)
              << " (Median ns, kleiner=besser), " << ranked.size() << " distinkte binary_ids:\n";
    int const show = (top_n < static_cast<int>(ranked.size())) ? top_n : static_cast<int>(ranked.size());
    for (int r = 0; r < show; ++r) {
        std::printf("  #%-2d  median=%12.3f ns  n=%-3zu  %s\n",
                    r + 1, ranked[r].median_value, ranked[r].samples, ranked[r].binary_id.c_str());
    }

    // 3) Gewinner → reale perm.dll auffinden (Repository, orch_make_stem-Round-Trip).
    bb::RankedBinary const& winner = ranked.front();
    bb::TiereDllRepository repo{tiere_dir};
    auto const dir_opt = repo.resolve_dir(winner.binary_id);
    if (!dir_opt) {
        std::cerr << "[FEHLER] keine reale perm.dll im tiere/-Baum fuer Gewinner:\n         "
                  << winner.binary_id << "\n";
        return 1;
    }
    std::cout << "[dll]    Gewinner-perm.dll: " << (*dir_opt / "perm.dll").string() << "\n";

    // 4) Versand-Artefakt bauen (Builder).
    bb::ShippedArtifactBuilder builder{out_dir, art_name};
    std::string err;
    auto const art = builder.build(winner, metric, *dir_opt, err);
    if (!art) { std::cerr << "[FEHLER] Versand fehlgeschlagen: " << err << "\n"; return 1; }

    std::cout << "[ship]   Artefakt ausgeliefert:\n";
    std::cout << "         dll      = " << art->shipped_dll.string()   << "\n";
    std::cout << "         manifest = " << art->manifest_path.string() << "\n";
    std::cout << "         abi      = major " << bb::kAbiMajor << " minor " << bb::kAbiMinor
              << "  build_version=" << art->dll_build_version << "\n";
    std::cout << "[ok]     beste binary_id (Metrik " << bb::metric_name(metric) << "): "
              << winner.binary_id << "  (median=" << winner.median_value << " ns)\n";
    return 0;
}
