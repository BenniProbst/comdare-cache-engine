#pragma once
// V41.F.6.1.R7.5.h axis_01_index_organization CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_01_index_organization_concept.hpp"
#include "concepts/axis_01_index_organization_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp> // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)

namespace comdare::cache_engine::index_organization {

template <typename Derived>
class IndexOrganizationStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
protected:
    IndexOrganizationStrategyBase() noexcept {
        static_assert(concepts::IndexOrganizationStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::index_organization
