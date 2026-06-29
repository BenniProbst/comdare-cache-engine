#pragma once
// processing_strategy - Compile-time Permutation + Runtime-Verhalten (REV 7 §4.2(d))
//
// Definiert fuer den ExecutionEngine-Kern:
//   - Permutation-statisch (Compile-time PermutationFlags)
//   - Verhalten-runtime-dynamisch (CacheEngineMode)
//   - Bereiche: Limits, Verhalten, Heuristiken, Allokation, Scheduling, Concurrency

#include "../concepts/permutation_flags.hpp"
#include "../concepts/cache_engine_mode.hpp"
#include "../allocators/allocator_permutation_flags.hpp"

namespace comdare {

template <cache_engine::PermutationFlags CT_Flags        = {},
          cache_engine::CacheEngineMode  RT_Mode_Default = cache_engine::CacheEngineMode::HEURISTIC_STATIC,
          cache_engine::allocator::AllocatorPermutationFlags AllocFlags = {}>
struct processing_strategy {
    // Compile-time:
    static constexpr cache_engine::PermutationFlags                     compile_time_flags   = CT_Flags;
    static constexpr cache_engine::CacheEngineMode                      default_runtime_mode = RT_Mode_Default;
    static constexpr cache_engine::allocator::AllocatorPermutationFlags allocator_flags      = AllocFlags;

    // Runtime-dynamische Variable (kann waehrend Workload geaendert werden via Heuristik)
    cache_engine::CacheEngineMode current_mode = RT_Mode_Default;

    // Limits-Bereich (Runtime-dynamisch)
    struct Limits {
        std::size_t max_cache_pages        = 0; // 0 = unlimited
        std::size_t max_concurrent_writers = 0;
        std::size_t prefetch_distance_max  = 16;
    } limits;

    // Verhalten-Bereich
    struct Verhalten {
        bool enable_telemetry_collection = true;
        bool enable_adaptive_resize      = true;
        bool enable_hugepage_filler      = true;
    } verhalten;
};

} // namespace comdare
