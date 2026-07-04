#pragma once
// V41.F.6.1 R5.E (2026-05-29) — latency_stats: wiederverwendbare Latenz-Perzentile/Statistik
//
// @subsystem CEB (Mess-Auswertung, neben welch_t_test.hpp + multiple_comparison.hpp + result_aggregator.hpp)
// @phase_owner CEB
//
// Extrahiert aus ExecuteEngineCommand::percentile_ns (war privat + MUTIERTE die Eingabe via
// nth_element). Hier wiederverwendbar + NON-MUTIEREND (kopiert intern) — damit die Roh-Samples
// fuer Welch's t-Test (CompareEngineCommand) in Original-Reihenfolge erhalten bleiben und mehrere
// Konsumenten (ExecuteEngine, ResultAggregator, Analyse) dieselbe Perzentil-Definition teilen.
//
// Perzentil-Methode: Nearest-Rank, k = min(n-1, floor(q*n)).

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <span>
#include <vector>

namespace comdare::cache_engine::builder::commands::stats {

/// Nearest-Rank-Perzentil der Latenz-Samples (non-mutierend; q wird auf [0,1] geklemmt).
[[nodiscard]] inline std::chrono::nanoseconds percentile_ns(std::span<const std::int64_t> samples, double q) {
    if (samples.empty()) return std::chrono::nanoseconds{0};
    if (q < 0.0) q = 0.0;
    if (q > 1.0) q = 1.0;
    std::vector<std::int64_t> copy(samples.begin(), samples.end());
    std::size_t const k = std::min(copy.size() - 1, static_cast<std::size_t>(q * static_cast<double>(copy.size())));
    std::nth_element(copy.begin(), copy.begin() + static_cast<std::ptrdiff_t>(k), copy.end());
    return std::chrono::nanoseconds{copy[k]};
}

[[nodiscard]] inline std::chrono::nanoseconds latency_p50_ns(std::span<const std::int64_t> s) {
    return percentile_ns(s, 0.50);
}
[[nodiscard]] inline std::chrono::nanoseconds latency_p95_ns(std::span<const std::int64_t> s) {
    return percentile_ns(s, 0.95);
}
[[nodiscard]] inline std::chrono::nanoseconds latency_p99_ns(std::span<const std::int64_t> s) {
    return percentile_ns(s, 0.99);
}

/// Minimum (0 bei leerer Eingabe).
[[nodiscard]] inline std::int64_t latency_min_ns(std::span<const std::int64_t> s) {
    if (s.empty()) return 0;
    return *std::min_element(s.begin(), s.end());
}
/// Maximum (0 bei leerer Eingabe).
[[nodiscard]] inline std::int64_t latency_max_ns(std::span<const std::int64_t> s) {
    if (s.empty()) return 0;
    return *std::max_element(s.begin(), s.end());
}
/// arithmetisches Mittel in ns (0.0 bei leerer Eingabe; long-double-Akkumulation gegen Overflow).
[[nodiscard]] inline double latency_mean_ns(std::span<const std::int64_t> s) {
    if (s.empty()) return 0.0;
    long double sum = 0.0L;
    for (auto v : s) sum += static_cast<long double>(v);
    return static_cast<double>(sum / static_cast<long double>(s.size()));
}

// ── #165-A (P-MD9, 2026-06-20): WINSORIZED MEAN — Lehrbuch-Robust-Statistik ─────────────────────────────────
// Benanntes Muster: "Winsorized Mean" (Winsorizing nach C. P. Winsor; siehe Dixon & Tukey 1968, "Approximate
// Behavior of the Distribution of Winsorized t", Technometrics 10(1):83–98). ABGRENZUNG zum getrimmten Mittel:
// das getrimmte Mittel ENTFERNT die Extrem-Samples; das winsorisierte Mittel BEHÄLT alle n Samples, CLAMPt aber
// die unteren/oberen Ausreißer auf die jeweilige Perzentil-Grenze [P(trim_q), P(1-trim_q)] und mittelt dann über
// ALLE n geklemmten Werte. Robust gegen einzelne System-Störungs-Spitzen (Scheduler/IRQ), ohne die Stichprobengröße
// zu verkleinern → ein stabilerer zentraler Lagewert als das arithmetische Mittel für die Latenz-Auswertung.
//
// NON-MUTIEREND (kopiert intern, identisch zu percentile_ns): die Roh-Samples bleiben in Original-Reihenfolge für
// Welch's t-Test/Perzentile erhalten. trim_q wird auf [0, 0.5) geklemmt (symmetrischer Trim je Flanke); trim_q<=0
// ⇒ kein Clamping ⇒ exakt latency_mean_ns. Leere Eingabe ⇒ 0.0.
//
// Grenzen via percentile_ns (Nearest-Rank, dieselbe Single-Source-Perzentil-Definition) → keine Methoden-Drift.
[[nodiscard]] inline double winsorized_mean_ns(std::span<const std::int64_t> samples, double trim_q) {
    if (samples.empty()) return 0.0;
    if (trim_q <= 0.0) return latency_mean_ns(samples); // kein Trim → arithmetisches Mittel über alle n
    if (trim_q >= 0.5) trim_q = 0.5 - 1e-9;             // symmetrischer Trim < halbe Stichprobe (kein Kollaps)
    std::int64_t const lo = percentile_ns(samples, trim_q).count();       // untere Winsor-Grenze P(trim_q)
    std::int64_t const hi = percentile_ns(samples, 1.0 - trim_q).count(); // obere  Winsor-Grenze P(1-trim_q)
    // Robust gegen lo>hi (degenerierte/winzige Stichprobe): in geordnete [min(lo,hi), max(lo,hi)] normalisieren.
    std::int64_t const clamp_lo = (lo <= hi) ? lo : hi;
    std::int64_t const clamp_hi = (lo <= hi) ? hi : lo;
    long double        sum      = 0.0L;
    for (auto v : samples) {
        std::int64_t const w = (v < clamp_lo) ? clamp_lo : (v > clamp_hi ? clamp_hi : v); // auf [lo,hi] winsorisieren
        sum += static_cast<long double>(w);
    }
    return static_cast<double>(sum / static_cast<long double>(samples.size())); // Mittel über ALLE n (behalten!)
}

} // namespace comdare::cache_engine::builder::commands::stats
