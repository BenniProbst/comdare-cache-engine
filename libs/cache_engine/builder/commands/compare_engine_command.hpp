#pragma once
// V32.DD.1 (2026-05-18) - CompareEngineCommand (F15-Forschungsmission)
//
// @subsystem CEB
// @command_pattern Compare
// @phase_owner CEB (Phase 7 COMPARE)

#include "i_command.hpp"
#include "execution_result.hpp"

namespace comdare::cache_engine::builder::commands {

/**
 * @brief CompareEngineCommand - vergleicht zwei ExecutionEngine-Resultate (F15)
 * @subsystem CEB
 * @command_pattern Compare
 * @phase_owner CEB
 *
 * Implementiert F15-Forschungsmission: vergleicht EE-A (CacheEngine) vs EE-B (PrtArt)
 * auf einer Permutation:
 * - Throughput-Ratio (PrtArt schneller? CE schneller? gleich?)
 * - Latency-Delta (Mikrosekunden-Differenz)
 * - Cache-Miss-Verbesserung (CLU/Footprint/H1-Hypothese)
 * - Memory-Footprint-Verhaeltnis (H2-Hypothese)
 *
 * Aufruf:
 *   auto cmp = CompareEngineCommand(result_ee_a, result_ee_b);
 *   cmp.execute(); // berechnet Vergleichs-Metriken
 *   auto verdict = cmp.verdict(); // {ee_a_wins, ee_b_wins, tie}
 */
class CompareEngineCommand : public ICommand {
public:
    enum class Verdict {
        EE_A_Wins,
        EE_B_Wins,
        Tie,
        InconclusiveData
    };

    CompareEngineCommand(ExecutionResult result_a, ExecutionResult result_b) noexcept
        : result_a_{result_a}, result_b_{result_b} {}

    [[nodiscard]] std::string_view command_name() const noexcept override {
        return "CompareEngineCommand";
    }

    int execute() override {
        // V32.EE.1: konkrete Vergleichs-Logik
        if (!result_a_.success || !result_b_.success) {
            verdict_ = Verdict::InconclusiveData;
            return 1;
        }

        // F15-Vergleich
        throughput_ratio_ = (result_b_.throughput_ops_per_sec > 0.0)
            ? result_a_.throughput_ops_per_sec / result_b_.throughput_ops_per_sec
            : 0.0;
        latency_delta_ns_ =
            (result_b_.latency_p99 - result_a_.latency_p99).count();
        cache_miss_improvement_ =
            (result_a_.total_cache_misses < result_b_.total_cache_misses)
                ? (result_b_.total_cache_misses - result_a_.total_cache_misses)
                : 0;
        memory_footprint_ratio_ = (result_b_.memory_footprint_bytes > 0)
            ? static_cast<double>(result_a_.memory_footprint_bytes)
                / static_cast<double>(result_b_.memory_footprint_bytes)
            : 0.0;

        // Schwellwerte fuer Verdict (V32.1 final-tunable)
        constexpr double winner_threshold = 1.05;  // 5% Vorteil
        if (throughput_ratio_ > winner_threshold) {
            verdict_ = Verdict::EE_A_Wins;
        } else if (throughput_ratio_ < (1.0 / winner_threshold)) {
            verdict_ = Verdict::EE_B_Wins;
        } else {
            verdict_ = Verdict::Tie;
        }
        return 0;
    }

    [[nodiscard]] Verdict verdict() const noexcept { return verdict_; }
    [[nodiscard]] double throughput_ratio() const noexcept { return throughput_ratio_; }
    [[nodiscard]] long long latency_delta_ns() const noexcept { return latency_delta_ns_; }
    [[nodiscard]] std::uint64_t cache_miss_improvement() const noexcept { return cache_miss_improvement_; }
    [[nodiscard]] double memory_footprint_ratio() const noexcept { return memory_footprint_ratio_; }

private:
    ExecutionResult result_a_;
    ExecutionResult result_b_;
    Verdict verdict_ {Verdict::InconclusiveData};
    double throughput_ratio_ {0.0};
    long long latency_delta_ns_ {0};
    std::uint64_t cache_miss_improvement_ {0};
    double memory_footprint_ratio_ {0.0};
};

}  // namespace comdare::cache_engine::builder::commands
