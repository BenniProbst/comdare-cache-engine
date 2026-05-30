#pragma once
// V41.F.6.1.R7.5.d axis_14_value_handle CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_14_value_handle_concept.hpp"
#include "concepts/axis_14_value_handle_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>

namespace comdare::cache_engine::value_handle_axis {

template <typename Derived>
class ValueHandleStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    ValueHandleStrategyBase() noexcept {
        static_assert(concepts::ValueHandleStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
