#pragma once
// V41.F.6.1 R5.D — Mann-Whitney-U-Test (Wilcoxon Rangsummen-Test), nicht-parametrisch.
//
// @topic builder/commands/stats
//
// **Zweck (Robustheit gegen Wall-Clock-Ausreisser):** der Welch-t-Test (welch_t_test.hpp) nutzt
// Mittelwert + Varianz und ist daher empfindlich gegen einzelne Batch-Spitzen (Scheduler-Preemption,
// Page-Faults, Turbo-Schwankungen) — beobachtet bis ~10× Ausreisser, die SPURIOSE Signifikanz erzeugen
// koennen. Der Mann-Whitney-U-Test vergleicht RAENGE statt Werte: er ist robust gegen solche Ausreisser
// und setzt KEINE Normalverteilung voraus. Als KOMPLEMENTAERER Test ausgewiesen — eine Diskrepanz
// Welch-signifikant ↔ MWU-nicht-signifikant ist ein Warnsignal fuer ausreisser-getriebene Scheinbefunde.
//
// **Methode:** alle Samples beider Gruppen gemeinsam ranken (Durchschnitts-Raenge bei Bindungen),
// Rangsumme R_a der Gruppe a → U_a = R_a − n_a(n_a+1)/2. Normalapproximation (gueltig fuer grosse n,
// hier n=batches≈128) mit Tie-Korrektur der Varianz; zweiseitiger p-Wert via std::erfc.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::commands::stats {

struct MannWhitneyResult {
    double u_statistic {0.0};          ///< U_a (Gruppe a vs. Gruppe b)
    double z_score    {0.0};           ///< standardisierte U-Statistik (Normalapprox)
    double p_value    {1.0};           ///< zweiseitiger p-Wert (erfc-Normalapprox)
    bool   a_stochastically_less {false};  ///< a tendiert zu KLEINEREN Werten (= a schneller bei Latenz)
    bool   valid      {false};         ///< false bei leeren Gruppen
};

/// Mann-Whitney-U / Wilcoxon-Rangsummen-Test. a, b = Latenz-Samples (ns). Bei leerer Gruppe valid=false.
[[nodiscard]] inline MannWhitneyResult mann_whitney_u_test(std::span<const std::int64_t> a,
                                                           std::span<const std::int64_t> b) {
    MannWhitneyResult r {};
    std::size_t const na = a.size();
    std::size_t const nb = b.size();
    if (na == 0 || nb == 0) return r;

    std::size_t const N = na + nb;
    std::vector<std::pair<std::int64_t, int>> all;   // (Wert, Gruppe: 0=a, 1=b)
    all.reserve(N);
    for (auto v : a) all.emplace_back(v, 0);
    for (auto v : b) all.emplace_back(v, 1);
    std::sort(all.begin(), all.end(),
              [](auto const& x, auto const& y) { return x.first < y.first; });

    // Durchschnitts-Raenge (1-basiert) + Tie-Korrektur-Summe sum(t^3 - t).
    std::vector<double> rank(N);
    double tie_sum = 0.0;
    for (std::size_t i = 0; i < N;) {
        std::size_t j = i;
        while (j + 1 < N && all[j + 1].first == all[i].first) ++j;
        double const avg = (static_cast<double>(i + 1) + static_cast<double>(j + 1)) / 2.0;
        for (std::size_t k = i; k <= j; ++k) rank[k] = avg;
        double const t = static_cast<double>(j - i + 1);
        tie_sum += t * t * t - t;
        i = j + 1;
    }

    double R_a = 0.0;
    for (std::size_t k = 0; k < N; ++k) if (all[k].second == 0) R_a += rank[k];

    double const nad = static_cast<double>(na);
    double const nbd = static_cast<double>(nb);
    double const Nd  = static_cast<double>(N);
    double const U_a    = R_a - nad * (nad + 1.0) / 2.0;
    double const mean_U = nad * nbd / 2.0;
    double const var_U  = (nad * nbd / 12.0) * ((Nd + 1.0) - tie_sum / (Nd * (Nd - 1.0)));

    r.u_statistic = U_a;
    r.valid = true;
    if (var_U <= 0.0) {                 // alle Werte identisch (oder n=1) → kein Unterschied
        r.z_score = 0.0; r.p_value = 1.0; r.a_stochastically_less = false;
        return r;
    }
    double const z = (U_a - mean_U) / std::sqrt(var_U);
    r.z_score = z;
    r.p_value = std::erfc(std::fabs(z) / std::sqrt(2.0));   // zweiseitig
    r.a_stochastically_less = (U_a < mean_U);              // a hat kleinere Raenge → a schneller
    return r;
}

}  // namespace comdare::cache_engine::builder::commands::stats
