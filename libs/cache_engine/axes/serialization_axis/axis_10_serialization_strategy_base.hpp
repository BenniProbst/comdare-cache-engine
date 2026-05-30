#pragma once
// V41.F.6.1.R7.5.c axis_10_serialization CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_10_serialization_concept.hpp"
#include "concepts/axis_10_serialization_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>

namespace comdare::cache_engine::serialization_axis {

template <typename Derived>
class SerializationStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    SerializationStrategyBase() noexcept {
        static_assert(concepts::SerializationStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
