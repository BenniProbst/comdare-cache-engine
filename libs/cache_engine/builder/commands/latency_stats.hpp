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
    std::size_t const k = std::min(copy.size() - 1,
        static_cast<std::size_t>(q * static_cast<double>(copy.size())));
    std::nth_element(copy.begin(), copy.begin() + static_cast<std::ptrdiff_t>(k), copy.end());
    return std::chrono::nanoseconds{copy[k]};
}

[[nodiscard]] inline std::chrono::nanoseconds latency_p50_ns(std::span<const std::int64_t> s) {
    return percentile_ns(s, 0.50);
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

}  // namespace comdare::cache_engine::builder::commands::stats
