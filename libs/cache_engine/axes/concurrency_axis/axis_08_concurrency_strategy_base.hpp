#pragma once
// V41.F.6.1.R7.3 axis_08_concurrency CRTP-StrategyBase (Goldstandard)
// Ersetzt axis_08_concurrency_base.hpp (Stufe-A).

#include "concepts/axis_08_concurrency_concept.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp> // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)

namespace comdare::cache_engine::concurrency_axis {

template <typename Derived>
class ConcurrencyStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
protected:
    ConcurrencyStrategyBase() noexcept {
        static_assert(concepts::ConcurrencyStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::concurrency_axis
