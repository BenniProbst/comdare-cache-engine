// best_binary_selector_main — CLI-Frontend (Versprechen 1: beste-Binary-Auswahl + Versand).
//
// Aufruf:
//   best_binary_selector --csv <mess.csv> --tiere <tiere-dir> --out <out-dir>
//                        [--metric ns_per_op|insert|lookup|erase|scan|rmw]
//                        [--objectives <m1,m2,...>]
//                        [--top N]   [--name <artifact-name>]
//
// Liest die Mess-CSV, rankt je gewählter Metrik (Strategy) über two_phase_valid-Zeilen, findet die
// reale perm.dll des Gewinners (Repository) und liefert DLL + .version + Manifest aus (Builder).
//
// WELLE C T-8 (Owner-Entscheid 2026-07-10): das ERGEBNIS der Auswahl ist die PARETO-FRONT ueber alle
// in der CSV wertbaren Zielgroessen (Richtung je Zielgroesse aus dem Katalog-Spiegel). --metric waehlt
// nur noch die Versand-SICHT auf die Front (bestes Front-Mitglied in dieser Metrik) -- ein dominierter
// Kandidat kann nicht mehr versandt werden. Die Front steht vollstaendig im Manifest.
#include "best_binary_selector.hpp"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>

namespace bb = comdare::cache_engine::best_binary;

namespace {

void usage() {
    std::cout << "best_binary_selector — automatische Auswahl + Versand der besten Tier-Binary\n"
                 "  --csv <file>     Mess-CSV (lazy_csv_header WIDE-Schema)            [pflicht]\n"
                 "  --tiere <dir>    tiere/-Baum (build/thesis_tiere/tiere)           [pflicht]\n"
                 "  --out <dir>      Ausgabe-Verzeichnis fuer das Versand-Artefakt    [pflicht]\n"
                 "  --metric <m>     ns_per_op|insert|lookup|erase|scan|rmw  (default ns_per_op)\n"
                 "                   = Versand-SICHT auf die Pareto-Front, NICHT das Auswahl-Ergebnis\n"
                 "  --objectives <l> Komma-Liste der Pareto-Zielgroessen (default: alle in der CSV\n"
                 "                   wertbaren Metriken; --metric ist immer dabei)\n"
                 "  --top <N>        N beste Kandidaten listen (default 5)\n"
                 "  --name <s>       Basisname des Artefakts (default best_<metric>)\n";
}

/// Komma-Liste in Metriken aufloesen (honest-fail: ein unbekannter Token ist ein Fehler, kein Skip).
bool parse_metric_list(std::string const& list, std::vector<bb::Metric>& out) {
    std::size_t pos = 0;
    while (pos <= list.size()) {
        std::size_t const sep   = list.find(',', pos);
        std::size_t const end   = (sep == std::string::npos) ? list.size() : sep;
        std::string const token = list.substr(pos, end - pos);
        if (!token.empty()) {
            auto const m = bb::metric_from_string(token);
            if (!m) {
                std::cerr << "Unbekannte Zielgroesse: " << token << "\n";
                return false;
            }
            if (std::find(out.begin(), out.end(), *m) == out.end()) out.push_back(*m);
        }
        if (sep == std::string::npos) break;
        pos = sep + 1;
    }
    return true;
}

std::string arg_after(int argc, char** argv, int& i) {
    if (i + 1 >= argc) return {};
    return argv[++i];
}

} // namespace

int main(int argc, char** argv) {
    std::string csv_path, tiere_dir, out_dir, metric_str = "ns_per_op", art_name, objectives_str;
    int         top_n = 5;

    for (int i = 1; i < argc; ++i) {
        std::string const a = argv[i];
        if (a == "--csv")
            csv_path = arg_after(argc, argv, i);
        else if (a == "--tiere")
            tiere_dir = arg_after(argc, argv, i);
        else if (a == "--out")
            out_dir = arg_after(argc, argv, i);
        else if (a == "--metric")
            metric_str = arg_after(argc, argv, i);
        else if (a == "--objectives")
            objectives_str = arg_after(argc, argv, i);
        else if (a == "--name")
            art_name = arg_after(argc, argv, i);
        else if (a == "--top") {
            try {
                top_n = std::stoi(arg_after(argc, argv, i));
            } catch (...) {}
        } else if (a == "--help" || a == "-h") {
            usage();
            return 0;
        } else {
            std::cerr << "Unbekanntes Argument: " << a << "\n";
            usage();
            return 2;
        }
    }
    if (csv_path.empty() || tiere_dir.empty() || out_dir.empty()) {
        usage();
        return 2;
    }

    auto const metric_opt = bb::metric_from_string(metric_str);
    if (!metric_opt) {
        std::cerr << "Unbekannte Metrik: " << metric_str << "\n";
        return 2;
    }
    bb::Metric const metric = *metric_opt;
    if (art_name.empty()) art_name = "best_" + bb::metric_name(metric);

    // 1) CSV einlesen. (REV-DATA-04): strikte Zahl-Validierung — verworfene Zeilen mit Diagnose ausweisen.
    std::vector<bb::MeasurementRow> rows;
    std::vector<std::string>        rejects;
    int const                       n = bb::parse_measurement_csv(csv_path, rows, &rejects);
    if (n < 0) {
        std::cerr << "CSV konnte nicht gelesen werden: " << csv_path << "\n";
        return 1;
    }
    std::cout << "[csv]    " << n << " Datenzeilen gelesen aus " << csv_path << "\n";
    if (!rejects.empty()) {
        std::cerr << "[csv]    WARN: " << rejects.size() << " Zeile(n) verworfen (strikte Zahl-Validierung):\n";
        std::size_t const show_rejects = (rejects.size() < 5u) ? rejects.size() : 5u;
        for (std::size_t i = 0; i < show_rejects; ++i) std::cerr << "         " << rejects[i] << "\n";
        if (rejects.size() > show_rejects)
            std::cerr << "         ... (+" << (rejects.size() - show_rejects) << " weitere)\n";
    }

    // 2) Rangbildung (Strategy = gewaehlte Metrik). (REV-DATA-07): stratifiziert je Mess-Zelle; Kandidaten
    //    ohne volle Zell-Abdeckung werden disqualifiziert (Diagnose unten).
    bb::RankingCriterion     crit{metric};
    std::vector<std::string> disqualified;
    auto const               ranked = bb::rank_binaries(rows, crit, &disqualified);
    if (!disqualified.empty()) {
        std::cerr << "[rank]   WARN: " << disqualified.size()
                  << " Kandidat(en) disqualifiziert (unvollstaendige Zell-Abdeckung, REV-DATA-07):\n";
        for (auto const& d : disqualified) std::cerr << "         " << d << "\n";
    }
    if (ranked.empty()) {
        std::cerr << "Keine two_phase_valid-Zeilen mit Metrik '" << bb::metric_name(metric)
                  << "' (>0) und voller Zell-Abdeckung.\n";
        return 1;
    }

    std::cout << "[rank]   Kriterium=" << bb::metric_name(metric) << " (Median der Zell-Mediane ns, kleiner=besser), "
              << ranked.size() << " distinkte binary_ids:\n";
    int const show = (top_n < static_cast<int>(ranked.size())) ? top_n : static_cast<int>(ranked.size());
    for (int r = 0; r < show; ++r) {
        std::printf("  #%-2d  median=%12.3f ns  n=%-3zu  zellen=%-3zu  %s\n", r + 1, ranked[r].median_value,
                    ranked[r].samples, ranked[r].cells, ranked[r].binary_id.c_str());
    }

    // 2b) WELLE C T-8: DAS ERGEBNIS DER AUSWAHL IST DIE PARETO-FRONT. Zielgroessen: explizit genannt
    //     (--objectives) oder alle in dieser CSV wertbaren Metriken; --metric ist immer dabei, damit die
    //     Versand-Sicht auf der Front definiert ist.
    std::vector<bb::Metric> objective_metrics;
    if (!objectives_str.empty()) {
        if (!parse_metric_list(objectives_str, objective_metrics)) return 2;
        if (std::find(objective_metrics.begin(), objective_metrics.end(), metric) == objective_metrics.end())
            objective_metrics.push_back(metric);
    } else {
        static constexpr bb::Metric kAllMetrics[] = {bb::Metric::ns_per_op, bb::Metric::insert, bb::Metric::lookup,
                                                     bb::Metric::erase,     bb::Metric::scan,   bb::Metric::rmw};
        for (bb::Metric m : kAllMetrics)
            if (m == metric || !bb::rank_binaries(rows, bb::RankingCriterion{m}).empty())
                objective_metrics.push_back(m);
    }

    std::vector<bb::ParetoObjective> objectives;
    objectives.reserve(objective_metrics.size());
    std::string objectives_line;
    for (bb::Metric m : objective_metrics) {
        bb::OptimizationDirection const dir = bb::metric_direction(m);
        objectives.push_back(bb::ParetoObjective{m, dir});
        if (!objectives_line.empty()) objectives_line += ",";
        objectives_line += bb::metric_name(m);
        objectives_line += (dir == bb::OptimizationDirection::Minimize) ? ":min" : ":max";
    }

    std::vector<std::string> pareto_disq;
    auto const               front = bb::pareto_rank_binaries(rows, objectives, &pareto_disq);
    if (!pareto_disq.empty()) {
        std::cerr << "[pareto] WARN: " << pareto_disq.size() << " Diagnose(n) der Mehrziel-Auswertung:\n";
        for (auto const& d : pareto_disq) std::cerr << "         " << d << "\n";
    }
    std::cout << "[pareto] Zielgroessen=" << objectives_line << "  Front: " << front.front_size << " von "
              << front.entries.size() << " Kandidat(en) nicht-dominiert:\n";
    for (std::size_t i = 0; i < front.entries.size(); ++i) {
        bb::ParetoCandidate const& c = front.entries[i];
        std::printf("  %s ", c.on_front ? "[FRONT]     " : "[dominiert] ");
        for (std::size_t k = 0; k < c.values.size(); ++k)
            std::printf("%s=%.3f ", bb::metric_name(objectives[k].metric).c_str(), c.values[k]);
        std::printf(" %s%s\n", c.binary_id.c_str(),
                    c.on_front ? "" : (" (dominiert von " + c.dominated_by + ")").c_str());
    }

    // 3) Versand-SICHT auf die Front (nie ein dominierter Kandidat) -> reale perm.dll auffinden.
    bb::ParetoCandidate const* view = bb::front_best_in(front, metric);
    if (view == nullptr) {
        std::cerr << "[FEHLER] leere Pareto-Front (kein Kandidat in allen Zielgroessen wertbar).\n";
        return 1;
    }
    auto const winner_it = std::find_if(ranked.begin(), ranked.end(),
                                        [&](bb::RankedBinary const& rb) { return rb.binary_id == view->binary_id; });
    if (winner_it == ranked.end()) {
        std::cerr << "[FEHLER] Front-Kandidat nicht im Einzel-Ranking: " << view->binary_id << "\n";
        return 1;
    }
    bb::RankedBinary const& winner = *winner_it;
    if (winner.binary_id != ranked.front().binary_id)
        std::cout << "[pareto] Hinweis: Einzelsieger-Ranking wollte '" << ranked.front().binary_id
                  << "', dieser ist NICHT auf der Front -> versandt wird das Front-Mitglied '" << winner.binary_id
                  << "'.\n";
    bb::TiereDllRepository repo{tiere_dir};
    auto const             dir_opt = repo.resolve_dir(winner.binary_id);
    if (!dir_opt) {
        std::cerr << "[FEHLER] keine reale perm.dll im tiere/-Baum fuer Gewinner:\n         " << winner.binary_id
                  << "\n";
        return 1;
    }
    std::cout << "[dll]    Gewinner-perm.dll: " << (*dir_opt / "perm.dll").string() << "\n";

    // 4) Versand-Artefakt bauen (Builder). Die Front wandert MIT ins Manifest (kein Informationsverlust:
    //    das ausgelieferte Artefakt weist aus, aus welcher Front es die Einzelsieger-Sicht ist).
    bb::ShippedArtifactBuilder builder{out_dir, art_name};
    std::vector<std::string>   front_ids;
    front_ids.reserve(front.front_size);
    for (std::size_t i = 0; i < front.front_size; ++i) front_ids.push_back(front.entries[i].binary_id);
    builder.set_pareto_context(objectives_line, std::move(front_ids));
    std::string err;
    auto const  art = builder.build(winner, metric, *dir_opt, err);
    if (!art) {
        std::cerr << "[FEHLER] Versand fehlgeschlagen: " << err << "\n";
        return 1;
    }

    std::cout << "[ship]   Artefakt ausgeliefert:\n";
    std::cout << "         dll      = " << art->shipped_dll.string() << "\n";
    std::cout << "         manifest = " << art->manifest_path.string() << "\n";
    std::cout << "         abi      = major " << bb::kAbiMajor << " minor " << bb::kAbiMinor
              << "  build_version=" << art->dll_build_version << "\n";
    std::cout << "[ok]     Pareto-Front (" << front.front_size << " Kandidat(en), Zielgroessen " << objectives_line
              << "); versandte Sicht Metrik " << bb::metric_name(metric) << ": " << winner.binary_id
              << "  (median=" << winner.median_value << " ns)\n";
    return 0;
}
