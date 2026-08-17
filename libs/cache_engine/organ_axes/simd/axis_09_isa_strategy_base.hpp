#pragma once
// V41.F.6.1.R7.5.i axis_09_isa CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_09_isa_concept.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp> // INC-1a: isa ist HEUTE Organ (in kCompositionAxisNames); INC-2d homet es zu hardware_isa-System

namespace comdare::cache_engine::simd {

template <typename Derived>
class IsaStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
protected:
    IsaStrategyBase() noexcept {
        static_assert(concepts::IsaStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::simd
