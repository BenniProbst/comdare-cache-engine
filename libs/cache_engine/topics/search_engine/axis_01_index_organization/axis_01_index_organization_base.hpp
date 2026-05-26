#pragma once
#include "concepts/axis_01_index_organization_concept.hpp"
#include "../../axis_base.hpp"
namespace comdare::cache_engine::search_engine::axis_01_index_organization {
template <typename Derived> class IndexOrganizationBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    IndexOrganizationBase() noexcept {
        static_assert(concepts::IndexOrganizationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};
}  // namespace
