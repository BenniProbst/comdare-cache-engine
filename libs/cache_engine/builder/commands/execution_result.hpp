#pragma once
// V32.EE.1 (2026-05-18 spaet) - ExecutionResult fuer Mess-Daten pro EE-Lauf
//
// @subsystem CEB
// @phase_owner CEB

#include "workload.hpp"
#include <chrono>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::commands {

/// D4c/D4d (2026-08-09) -- DIE EINE Stelle, an der "diese Proben tragen keine Aussage" entschieden
/// wird. Bewusst auf der SPANNE definiert und nicht auf ExecutionResult, damit sowohl der Erzeuger
/// (make_execution_result, D4d) als auch der Konsument (multi_compare, D4c) dieselbe Definition
/// benutzen -- zwei Ableitungswege waeren genau die Drift-Klasse, gegen die dieses Paket gebaut ist.
///
/// TOT heisst: leer ODER kein einziger Wert > 0. Beides ist derselbe Befund -- die Messung hat
/// nichts geliefert, was ueber der Uhr-Aufloesung liegt.
///
/// WARUM NICHT "alle Werte gleich": eine Probenreihe aus lauter 1000 ist eine ECHTE Messung mit
/// (verdaechtig) hoher Praezision; sie darf gemessen und getestet werden. Eine Reihe aus lauter
/// Nullen dagegen kann keine Latenz sein -- sie entsteht, wenn nichts gelaufen ist. Nur der
/// zweite Fall ist hier gemeint.
///
/// WARUM > 0 UND NICHT >= 0: negative Latenzen sind ein Zeitnahme-DEFEKT (die HDR-Auswertung
/// zaehlt sie eigens als `verworfen_negativ`). Eine Reihe, die ausschliesslich aus Nullen und
/// negativen Werten besteht, traegt keine Aussage -- und `>= 0` haette genau sie durchgelassen.
[[nodiscard]] inline bool proben_sind_tot(std::span<const std::int64_t> samples) noexcept {
    for (auto v : samples)
        if (v > 0) return false;
    return true;
}

/**
 * @brief ExecutionResult - Mess-Daten einer ExecutionEngine-Ausfuehrung
 * @subsystem CEB
 *
 * Wird vom ExecuteEngineCommand mit Mess-Werten gefuellt + vom
 * CompareEngineCommand zum F15-Vergleich konsumiert.
 */
struct ExecutionResult {
    std::string_view engine_name{"unknown"};
    WorkloadKind     workload_kind{WorkloadKind::YCSB_C_ReadOnly};

    // Mess-Werte
    double                   throughput_ops_per_sec{0.0};
    std::chrono::nanoseconds latency_p50{};
    std::chrono::nanoseconds latency_p99{};
    std::uint64_t            total_cache_misses{0};
    std::uint64_t            memory_footprint_bytes{0};

    // F15-Hypothesen-Werte
    double H1_clu_improvement{0.0};       ///< Cache-Line-Utilization
    double H2_layout_score{0.0};          ///< Layout-Wahl-Effizienz
    double H3_inline_external_ratio{0.0}; ///< Inline-vs-External-Decision

    // V33.A.2: Per-Operation Latency-Samples fuer Welch's t-Test (optional)
    // Wenn leer -> CompareEngineCommand faellt zurueck auf Schwellwert-Vergleich.
    // Wenn gefuellt -> CompareEngineCommand kann welch_t_test() ausfuehren.
    std::vector<std::int64_t> latency_samples_ns{};

    // Status
    bool             success{false};
    std::string_view error_message{};
    // D4d (2026-08-09) -- EIGENES Feld neben success, und das ist kein Duplikat.
    //
    // `success` hat in diesem Feld ZWEI Schreiber-Klassen mit verschiedener Bedeutung:
    //   make_execution_result (Mess-Pfad) -- "es gab eine verwertbare Probe"
    //   ExecuteEngineCommand / der ABI-Adapter -- "die Backend-Operation hat geklappt"
    // Nur der Mess-Pfad kann ueberhaupt etwas ueber die Proben sagen. `degeneriert` traegt genau
    // diese Aussage getrennt, damit ein Schreiber der zweiten Klasse success setzen kann, ohne
    // damit stillschweigend "die Daten sind verwertbar" zu behaupten.
    bool degeneriert{false}; ///< keine einzige Probe > 0 (leer, nur Nullen oder nur Defekt-Werte)
};

} // namespace comdare::cache_engine::builder::commands
