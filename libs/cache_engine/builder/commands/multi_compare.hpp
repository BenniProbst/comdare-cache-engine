#pragma once
// V41.F.6.1 R6 (2026-05-29) — multi_compare: N Kompositionen gegen eine Baseline, FWER-kontrolliert
//
// @subsystem CEB (Mess-Auswertung)
// @phase_owner CEB
//
// CompareEngineCommand vergleicht GENAU ZWEI Engines (paarweise, Welch). Der eigentliche
// F15-Workflow vergleicht aber VIELE Achsen-Kompositionen gegen eine Baseline (z.B. std::map oder
// prt-art) — und braucht dann FWER-Kontrolle, sonst entstehen bei m Vergleichen Zufalls-
// "Signifikanzen". Diese Utility verbindet die drei Statistik-Bausteine zum kompletten Workflow:
//   1) welch_t_test pro Kandidat-vs-Baseline   (Effektgroesse + Roh-p)
//   2) holm_bonferroni_adjust ueber alle Roh-p  (FWER-Kontrolle, step-down)
//   3) Signifikanz-Markierung bei FWER alpha + Schneller-als-Baseline-Flag.
//
// Arbeitet direkt auf ExecutionResult (engine_name + latency_samples_ns) → maximal integriert.

#include "execution_result.hpp"
#include "welch_t_test.hpp"
#include "multiple_comparison.hpp"

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::commands::stats {

/// Ein Kandidat-vs-Baseline-Vergleich mit Roh- + FWER-korrigiertem p-Wert.
struct PairwiseComparison {
    std::string_view name {};                 ///< Kompositions-/Engine-Name des Kandidaten
    WelchResult       welch {};               ///< Welch-Resultat (Kandidat = a, Baseline = b)
    double            raw_p {1.0};             ///< unkorrigierter p-Wert (1.0 wenn Welch ungueltig)
    double            adjusted_p {1.0};        ///< Holm-Bonferroni-korrigierter p-Wert
    bool              significant {false};     ///< adjusted_p <= alpha
    bool              faster_than_baseline {false};  ///< mean_a < mean_b (kleinere Latenz)
};

/// Ergebnis eines Multi-Vergleichs gegen eine Baseline.
struct MultiCompareReport {
    std::vector<PairwiseComparison> comparisons {};
    std::size_t                     significant_count {0};
    double                          alpha {0.05};
};

/// Vergleicht jede Kandidaten-ExecutionResult gegen die Baseline (Welch ueber latency_samples_ns),
/// korrigiert alle p-Werte per Holm-Bonferroni und markiert Signifikanz bei FWER alpha.
/// Reihenfolge der comparisons == Reihenfolge der candidates.
[[nodiscard]] inline MultiCompareReport multi_compare_against_baseline(
    ExecutionResult const& baseline,
    std::span<const ExecutionResult> candidates,
    double alpha = 0.05) {
    MultiCompareReport rep;
    rep.alpha = alpha;
    rep.comparisons.reserve(candidates.size());

    std::vector<double> raw_p;
    raw_p.reserve(candidates.size());

    std::span<const std::int64_t> const base_samples{baseline.latency_samples_ns};
    for (auto const& c : candidates) {
        PairwiseComparison pc;
        pc.name  = c.engine_name;
        pc.welch = welch_t_test(std::span<const std::int64_t>{c.latency_samples_ns}, base_samples);
        pc.raw_p = pc.welch.valid ? pc.welch.p_value : 1.0;
        pc.faster_than_baseline = pc.welch.valid && (pc.welch.mean_a < pc.welch.mean_b);
        raw_p.push_back(pc.raw_p);
        rep.comparisons.push_back(pc);
    }

    auto const adj = holm_bonferroni_adjust(std::span<const double>{raw_p});
    for (std::size_t i = 0; i < rep.comparisons.size(); ++i) {
        rep.comparisons[i].adjusted_p  = adj[i];
        rep.comparisons[i].significant = (adj[i] <= alpha);
        if (rep.comparisons[i].significant) ++rep.significant_count;
    }
    return rep;
}

}  // namespace comdare::cache_engine::builder::commands::stats
