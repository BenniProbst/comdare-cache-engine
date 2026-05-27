#pragma once
// V41.F.6.1.R7.1.b axis_05_memory_layout CRTP-Basis (Goldstandard-Nachruestung)

#include "concepts/axis_05_memory_layout_concept.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include "../../axis_base.hpp"
#include <type_traits>

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {

/// MemoryLayoutBase — CRTP-Wurzel fuer alle Memory-Layout-Strategien.
///
/// 3-fach Concept-Check im Konstruktor (Goldstandard-Pattern analog
/// axis_06_allocator_strategy_base.hpp + axis_12_general_hardware_base.hpp):
///   1. MemoryLayoutStrategy (Achsen-eigenes Strategie-Concept)
///   2. CacheEnginePermutationStrategy (Cross-Achsen Permutations-API)
///   3. AxisBaseConcept (Cross-Achsen Mess-API via AxisBase Wurzel)
template <typename Derived>
class MemoryLayoutBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    MemoryLayoutBase() noexcept {
        static_assert(concepts::MemoryLayoutStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
