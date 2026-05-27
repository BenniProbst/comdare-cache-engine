#pragma once
// V41.F.6.1.R7.5.j axis_09b_simd_extension CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_09b_simd_extension_concept.hpp"
#include "concepts/axis_09b_simd_extension_cache_engine_permutation_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::hardware::axis_09b_simd_extension {

template <typename Derived>
class SimdExtensionStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    SimdExtensionStrategyBase() noexcept {
        static_assert(concepts::SimdExtensionStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
