#pragma once
// V32.EE.1 (2026-05-18 spaet) - ExecutionResult fuer Mess-Daten pro EE-Lauf
//
// @subsystem CEB
// @phase_owner CEB

#include "workload.hpp"
#include <chrono>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::builder::commands {

/**
 * @brief ExecutionResult - Mess-Daten einer ExecutionEngine-Ausfuehrung
 * @subsystem CEB
 *
 * Wird vom ExecuteEngineCommand mit Mess-Werten gefuellt + vom
 * CompareEngineCommand zum F15-Vergleich konsumiert.
 */
struct ExecutionResult {
    std::string_view engine_name {"unknown"};
    WorkloadKind workload_kind {WorkloadKind::YCSB_C_ReadOnly};

    // Mess-Werte
    double throughput_ops_per_sec {0.0};
    std::chrono::nanoseconds latency_p50 {};
    std::chrono::nanoseconds latency_p99 {};
    std::uint64_t total_cache_misses {0};
    std::uint64_t memory_footprint_bytes {0};

    // F15-Hypothesen-Werte
    double H1_clu_improvement {0.0};   ///< Cache-Line-Utilization
    double H2_layout_score {0.0};      ///< Layout-Wahl-Effizienz
    double H3_inline_external_ratio {0.0};  ///< Inline-vs-External-Decision

    // Status
    bool success {false};
    std::string_view error_message {};
};

}  // namespace comdare::cache_engine::builder::commands
