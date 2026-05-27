#pragma once
// V41.F.6.1.R7.5.a axis_07_prefetch CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_07_prefetch_concept.hpp"
#include "concepts/axis_07_prefetch_cache_engine_permutation_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::prefetch::axis_07_prefetch {

template <typename Derived>
class PrefetchStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    PrefetchStrategyBase() noexcept {
        static_assert(concepts::PrefetchStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
