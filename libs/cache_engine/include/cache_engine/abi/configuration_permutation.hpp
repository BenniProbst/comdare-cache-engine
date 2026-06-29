#pragma once
// configuration_permutation - XML-zu-Type-Mapping vom CacheEngineBuilder (REV 7 §4.2(d))
//
// Ein konkretes ConfigurationPermutation aggregiert ALLE compile-time-Wahlen
// fuer einen bestimmten Build-Permutations-Slot. Vom CacheEngineBuilder via
// XML-Konfiguration zur Compile-time gesetzt.

#include "processing_strategy.hpp"

#include <cstdint>

namespace comdare {

template <typename ProcessingStrategyT, typename ConcurrencyT, typename SchedulerT, typename HeuristicT,
          typename AllocatorStrategyT>
struct configuration_permutation {
    using strategy_t    = ProcessingStrategyT;
    using concurrency_t = ConcurrencyT;
    using scheduler_t   = SchedulerT;
    using heuristic_t   = HeuristicT;
    using allocator_t   = AllocatorStrategyT;

    // Unique ID fuer Permutation (Hash aus allen Wahlen) — wird vom Builder gesetzt
    std::uint64_t permutation_fingerprint = 0;
};

} // namespace comdare
