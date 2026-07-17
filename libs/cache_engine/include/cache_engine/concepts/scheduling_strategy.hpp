#pragma once
// ---------------------------------------------------------------------------
// DEPRECATED (2026-07-16, Marker nachgezogen 2026-07-17 F6): HISTORISCHER
// V32.EE.5-ALGORITHMUS-ACHSEN-ENTWURF (Runtime-vtable) -- 0 Konsumenten. Die
// 4. Schwester der vtable-Achsen (numa_affinity/locking_mode/hardware_strategy
// tragen den Marker seit 2026-07-16); hier nachgezogen. repo-weiter grep zeigt
// nur diese Definitionsdatei; die Treffer in
// builder/commands/axis_library_registry.hpp:289-294 sind reine deskriptive
// source_ref-STRINGS, KEINE C++-Konsumtion (vgl. dort Z.311/314:
// "0 Konsumenten ... scheduling_strategy.hpp bleibt hier UNBERUEHRT").
// deprecated-by-design: wird durch die kommende compile-time CRTP+Concept
//   Scheduling-SYSTEM-Achse (#37, User 2026-07-17: Scheduling = System-Achse,
//   unter der die CEB gebaut wird, KEIN Runtime-Switch) ersetzt. Runtime-vtable
//   hier = Doktrin-Verletzung (no_runtime_switch / compile_time_only), nie
//   konsumiert.
// Die SYSTEM-Seite (Pflicht-Systemachse im CacheEngineBuilder) wird NEU gebaut
//   -- Dossier 23 (Abschnitt 6, S-2; User-Entscheid 2026-07-16).
// NICHT reaktivieren, NICHT loeschen; Rest-Semantik ohne Live-Gegenstueck in
//   der Delta-Matrix (Dossier 23 Abschnitt 2) als TODO getrackt.
// ---------------------------------------------------------------------------
// V32.EE.5 (2026-05-18 spaet) - Achse 13 SCHEDULING-STRATEGY (NEU)
//
// @achse 13
// @subsystem CE
// @reuse_status (b)
//
// User-Direktive: NEUE Achse fuer Scheduling-Strategien.
// Wichtig: SIMD-Einheiten sind Hardware-limitiert (typ. 2 von N Cores).

#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::concepts {

/// Achse 13.1 Worker-Pool-Layout
enum class WorkerPoolLayout : std::uint8_t { ThreadPerCore = 0, WorkStealing = 1, CpuPinning = 2, FreePool = 3 };

/// Achse 13.3 Heterogeneous-Core-Dispatch (Intel Hybrid P/E)
enum class HeteroCoreDispatch : std::uint8_t { None = 0, HybridAware = 1, PCoresOnly = 2, ECoresOnly = 3 };

/// Achse 13.4 Co-Routine-Strategy
enum class CoRoutineStrategy : std::uint8_t { None = 0, Interleave = 1, DependencyTracking = 2 };

/// Achse 13.5 Batch-Granularity
enum class BatchGranularity : std::uint8_t { Single = 0, MicroBatch = 1, MacroBatch = 2 };

/**
 * @brief ISchedulingStrategy - Concept Achse 13 (NEU)
 * @achse 13
 * @subsystem CE
 * @reuse_status (b)
 *
 * 13.2 SIMD-Worker-Count-Limit ist HARDWARE-limitiert (typ. 2 von N).
 * Andere Worker laufen scalar normal parallel.
 */
class ISchedulingStrategy {
public:
    [[nodiscard]] virtual WorkerPoolLayout   get_worker_pool_layout() const noexcept      = 0;
    [[nodiscard]] virtual std::size_t        get_simd_worker_count_limit() const noexcept = 0;
    [[nodiscard]] virtual HeteroCoreDispatch get_hetero_core_dispatch() const noexcept    = 0;
    [[nodiscard]] virtual CoRoutineStrategy  get_co_routine_strategy() const noexcept     = 0;
    [[nodiscard]] virtual BatchGranularity   get_batch_granularity() const noexcept       = 0;

    virtual ~ISchedulingStrategy() = default;
};

/**
 * @brief DefaultSchedulingStrategy - Auto-Permutator-Basis
 * @achse 13
 * @reuse_status (a)
 */
struct DefaultSchedulingStrategy : ISchedulingStrategy {
    WorkerPoolLayout   worker_pool{WorkerPoolLayout::ThreadPerCore};
    std::size_t        simd_workers{2}; ///< Hardware-Limit, typ. 2 von N Cores
    HeteroCoreDispatch hetero{HeteroCoreDispatch::HybridAware};
    CoRoutineStrategy  co_routine{CoRoutineStrategy::Interleave};
    BatchGranularity   batch{BatchGranularity::MicroBatch};

    [[nodiscard]] WorkerPoolLayout   get_worker_pool_layout() const noexcept override { return worker_pool; }
    [[nodiscard]] std::size_t        get_simd_worker_count_limit() const noexcept override { return simd_workers; }
    [[nodiscard]] HeteroCoreDispatch get_hetero_core_dispatch() const noexcept override { return hetero; }
    [[nodiscard]] CoRoutineStrategy  get_co_routine_strategy() const noexcept override { return co_routine; }
    [[nodiscard]] BatchGranularity   get_batch_granularity() const noexcept override { return batch; }
};

} // namespace comdare::cache_engine::concepts
