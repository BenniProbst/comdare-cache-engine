#pragma once
// V41.F.6.1 R6 (2026-05-29) — Multiple-Comparison-Korrektur (Bonferroni + Holm-Bonferroni)
//
// @subsystem CEB (Statistik-Utility, neben welch_t_test.hpp)
// @phase_owner CEB
//
// F15 vergleicht VIELE Achsen-Kompositionen paarweise (jeder Welch-t-Test liefert einen p-Wert).
// Bei m Vergleichen mit Signifikanz-Niveau alpha steigt die familienweise Fehlerrate (FWER) auf
// ~1-(1-alpha)^m — ohne Korrektur werden zufaellige "Signifikanzen" produziert. Diese Utility
// kontrolliert die FWER:
//   - Bonferroni: p_adj_i = min(1, p_i * m). Einfach, konservativ. (Dunn 1961)
//   - Holm-Bonferroni: step-down, gleichmaessig maechtiger bei gleicher FWER-Kontrolle. (Holm 1979,
//     "A Simple Sequentially Rejective Multiple Test Procedure", Scand. J. Statist. 6(2):65-70)
//
// Beide liefern korrigierte p-Werte in der ORIGINAL-Reihenfolge der Eingabe; eine Hypothese ist bei
// FWER alpha signifikant gdw. ihr korrigierter p-Wert <= alpha.

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <span>
#include <vector>

namespace comdare::cache_engine::builder::commands::stats {

/// Bonferroni-Korrektur: p_adj_i = min(1, p_i * m), m = Zahl der Vergleiche.
[[nodiscard]] inline std::vector<double> bonferroni_adjust(std::span<const double> p_values) {
    std::vector<double> out;
    out.reserve(p_values.size());
    double const m = static_cast<double>(p_values.size());
    for (double p : p_values) {
        double adj = p * m;
        out.push_back(adj > 1.0 ? 1.0 : adj);
    }
    return out;
}

/// Holm-Bonferroni (step-down) korrigierte p-Werte, in Original-Reihenfolge.
/// Sortiert aufsteigend, skaliert den k-kleinsten (0-basiert) mit (m-k), erzwingt Monotonie
/// (laufendes Maximum) und kappt bei 1; streut dann zurueck auf die Original-Indizes.
[[nodiscard]] inline std::vector<double> holm_bonferroni_adjust(std::span<const double> p_values) {
    std::size_t const n = p_values.size();
    std::vector<double> out(n, 1.0);
    if (n == 0) return out;

    // Indizes nach p aufsteigend sortieren.
    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::sort(order.begin(), order.end(),
              [&](std::size_t a, std::size_t b) { return p_values[a] < p_values[b]; });

    double running_max = 0.0;
    for (std::size_t k = 0; k < n; ++k) {
        std::size_t const idx = order[k];
        double adj = static_cast<double>(n - k) * p_values[idx];  // (m-k) * p_(k)
        if (adj > 1.0) adj = 1.0;
        if (adj < running_max) adj = running_max;  // Monotonie erzwingen
        running_max = adj;
        out[idx] = adj;
    }
    return out;
}

/// Zahl der bei FWER alpha signifikanten Hypothesen (korrigierter p-Wert <= alpha).
[[nodiscard]] inline std::size_t count_significant(std::span<const double> adjusted_p, double alpha) {
    std::size_t n = 0;
    for (double p : adjusted_p) if (p <= alpha) ++n;
    return n;
}

}  // namespace comdare::cache_engine::builder::commands::stats
