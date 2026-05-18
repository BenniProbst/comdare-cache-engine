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

    /// HH.3 Konfigurierbarer Verdict-Schwellwert (default 5%)
    static constexpr double kDefaultWinnerThreshold = 1.05;

    CompareEngineCommand(ExecutionResult result_a, ExecutionResult result_b,
                         double winner_threshold = kDefaultWinnerThreshold) noexcept
        : result_a_{result_a}, result_b_{result_b}, winner_threshold_{winner_threshold} {}

    /// HH.3 H1/H2/H3 Hypothesen-Validierung (vereinfacht ohne Welch-T-Test)
    [[nodiscard]] bool h1_clu_validated() const noexcept {
        return result_a_.H1_clu_improvement >= 1.0;  // CE-EE-A muss CLU verbessern
    }
    [[nodiscard]] bool h2_layout_validated() const noexcept {
        return result_a_.H2_layout_score >= 1.0;
    }
    [[nodiscard]] bool h3_inline_external_validated() const noexcept {
        // H3: Inline vs External Decision sinnvoll wenn Ratio in [0.2, 0.8]
        return result_a_.H3_inline_external_ratio >= 0.2
            && result_a_.H3_inline_external_ratio <= 0.8;
    }

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

        // HH.3 Konfigurierbarer Schwellwert
        if (throughput_ratio_ > winner_threshold_) {
            verdict_ = Verdict::EE_A_Wins;
        } else if (throughput_ratio_ < (1.0 / winner_threshold_)) {
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
    double winner_threshold_;
    Verdict verdict_ {Verdict::InconclusiveData};
    double throughput_ratio_ {0.0};
    long long latency_delta_ns_ {0};
    std::uint64_t cache_miss_improvement_ {0};
    double memory_footprint_ratio_ {0.0};
};

}  // namespace comdare::cache_engine::builder::commands
