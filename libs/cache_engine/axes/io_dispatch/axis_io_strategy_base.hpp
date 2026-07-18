#pragma once
// V41.F.6.1.R7.5.f axis_io CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_io_concept.hpp"
#include "concepts/axis_io_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp> // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ) // V41.F.2: stabile Include-Dir-Form (war "../../axis_base.hpp", bricht nach Move)

namespace comdare::cache_engine::io_dispatch {

template <typename Derived>
class IoStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
protected:
    IoStrategyBase() noexcept {
        static_assert(concepts::IoStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::io_dispatch
