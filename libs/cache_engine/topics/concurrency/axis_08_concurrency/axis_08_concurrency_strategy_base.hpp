#pragma once
// V41.F.6.1.R7.3 axis_08_concurrency CRTP-StrategyBase (Goldstandard)
// Ersetzt axis_08_concurrency_base.hpp (Stufe-A).

#include "concepts/axis_08_concurrency_concept.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::concurrency::axis_08_concurrency {

template <typename Derived>
class ConcurrencyStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    ConcurrencyStrategyBase() noexcept {
        static_assert(concepts::ConcurrencyStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
