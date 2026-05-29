#pragma once
// V32.DD.1 (2026-05-18) + V33.A.1 (2026-05-21) - ExecuteEngineCommand
//
// @subsystem CEB
// @command_pattern Execute
// @phase_owner CEB (Phase 6 EXECUTE)

#include "i_command.hpp"
#include "workload.hpp"
#include "execution_result.hpp"
#include "latency_stats.hpp"   // R5.E: geteilte, non-mutierende Perzentil-Berechnung

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

namespace comdare::cache_engine::builder::commands {

/**
 * @brief OperationOutcome - Ergebnis EINER Engine-Operation in der Workload-Loop
 * @subsystem CEB
 *
 * Wird vom EngineCallable pro Operation an die Loop zurueckgegeben. Erlaubt
 * der Engine, Mess-Daten ueber den Funktor-Mechanismus zurueckzugeben ohne dass
 * die ICommand-Schicht auf konkrete Engine-Typen abhaengt.
 */
struct OperationOutcome {
    std::uint64_t cache_misses_delta {0};  ///< Cache-Misses durch diese Op (z.B. via perf_event)
    std::uint64_t bytes_touched {0};       ///< Memory-Traffic dieser Op
    bool success {true};
};

/**
 * @brief EngineCallable - injizierbare Engine-Operation
 * @subsystem CEB
 *
 * Signature: (op_index, workload_kind, seed) -> OperationOutcome
 * Cache-Engine/PrtArt-Adapter implementieren dies + reichen es an
 * ExecuteEngineCommand. Nullptr bzw. leerer Funktor -> Simulation (V32-Pfad).
 */
using EngineCallable = std::function<OperationOutcome(std::size_t, WorkloadKind, std::uint64_t)>;

/**
 * @brief ExecuteEngineCommand - fuehrt eine ExecutionEngine auf einer Permutation aus
 * @subsystem CEB
 * @command_pattern Execute
 * @phase_owner CEB
 *
 * Wird vom CacheEngineBuilder pro ExecutionEngine (EE-A = CacheEngine, EE-B = PrtArt)
 * pro Permutation instanziiert + ausgefuehrt. Resultat wird in result_ gespeichert
 * und vom CompareEngineCommand verglichen.
 *
 * V33.A.1: Echte Engine-Integration via EngineCallable (Dependency-Injection).
 * Wenn engine_callable_ leer -> Simulation (V32-Pfad, rueckwaertskompatibel).
 * Wenn gesetzt -> echte Operation pro Schritt, echte p50/p99 via percentile-Berechnung.
 *
 * Beispiel-Nutzung mit echtem Engine-Callable:
 *   auto ce_callable = [&ce](std::size_t op, WorkloadKind wk, std::uint64_t seed) {
 *       std::uint64_t key = derive_key(op, seed);
 *       auto result = ce.lookup(key);
 *       return OperationOutcome{result.cache_misses, result.bytes_read, result.found};
 *   };
 *   ExecuteEngineCommand cmd("CacheEngine-EE-A", workload, ce_callable);
 *   cmd.execute();
 */
class ExecuteEngineCommand : public ICommand {
public:
    /// V32 Konstruktor (Simulation, rueckwaertskompatibel)
    ExecuteEngineCommand(std::string_view engine_name, Workload workload) noexcept
        : engine_name_{engine_name}, workload_{workload} {}

    /// V33.A.1 Konstruktor mit echtem Engine-Callable
    ExecuteEngineCommand(std::string_view engine_name, Workload workload, EngineCallable callable) noexcept
        : engine_name_{engine_name}, workload_{workload}, engine_callable_{std::move(callable)} {}

    [[nodiscard]] std::string_view command_name() const noexcept override {
        return "ExecuteEngineCommand";
    }

    int execute() override {
        result_.engine_name = engine_name_;
        result_.workload_kind = workload_.kind;

        std::vector<std::int64_t> op_latencies_ns;
        op_latencies_ns.reserve(workload_.operation_count);

        std::uint64_t cache_misses_total = 0;
        std::uint64_t bytes_touched_total = 0;
        std::uint64_t ops_succeeded = 0;

        const auto overall_start = std::chrono::steady_clock::now();

        for (std::size_t op = 0; op < workload_.operation_count; ++op) {
            const auto op_start = std::chrono::steady_clock::now();

            OperationOutcome outcome {};
            if (engine_callable_) {
                outcome = engine_callable_(op, workload_.kind, workload_.seed);
            } else {
                outcome.cache_misses_delta = (op % 7 == 0) ? 1u : 0u;
                outcome.bytes_touched = 64;
                outcome.success = true;
            }

            const auto op_end = std::chrono::steady_clock::now();
            const auto op_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(op_end - op_start).count();
            op_latencies_ns.push_back(op_ns);

            cache_misses_total += outcome.cache_misses_delta;
            bytes_touched_total += outcome.bytes_touched;
            ops_succeeded += outcome.success ? 1u : 0u;
        }

        const auto overall_elapsed = std::chrono::steady_clock::now() - overall_start;
        const auto overall_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(overall_elapsed).count();

        result_.throughput_ops_per_sec =
            (overall_ns > 0)
                ? (static_cast<double>(workload_.operation_count) * 1e9 / static_cast<double>(overall_ns))
                : 0.0;

        // R5.E: geteilte, non-mutierende Perzentile (op_latencies_ns bleibt in Original-Reihenfolge
        // → unten als latency_samples_ns fuer Welch's t-Test gemovet).
        result_.latency_p50 = stats::percentile_ns(std::span<const std::int64_t>{op_latencies_ns}, 0.50);
        result_.latency_p99 = stats::percentile_ns(std::span<const std::int64_t>{op_latencies_ns}, 0.99);
        result_.total_cache_misses = cache_misses_total;
        result_.memory_footprint_bytes =
            engine_callable_ ? bytes_touched_total : workload_.record_count * 64;

        // V33.A.2: Latency-Samples behalten fuer Welch's t-Test in CompareEngineCommand
        result_.latency_samples_ns = std::move(op_latencies_ns);

        result_.H1_clu_improvement = 1.0;
        result_.H2_layout_score = 1.0;
        result_.H3_inline_external_ratio = 0.5;

        result_.success = (ops_succeeded == workload_.operation_count);
        return result_.success ? 0 : 1;
    }

    [[nodiscard]] bool is_parallelizable() const noexcept override {
        return true;
    }

    [[nodiscard]] const ExecutionResult& result() const noexcept {
        return result_;
    }

private:
    // V41.F.6.1 R5.E: percentile_ns nach latency_stats.hpp extrahiert (wiederverwendbar + non-mutierend).

    std::string_view engine_name_;
    Workload workload_;
    EngineCallable engine_callable_ {};
    ExecutionResult result_ {};
};

}  // namespace comdare::cache_engine::builder::commands
