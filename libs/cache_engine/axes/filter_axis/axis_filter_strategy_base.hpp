#pragma once
// V41.F.6.1.R7.5.e axis_filter CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_filter_concept.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>

namespace comdare::cache_engine::filter_axis {

template <typename Derived>
class FilterStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    FilterStrategyBase() noexcept {
        static_assert(concepts::FilterStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::filter_axis
