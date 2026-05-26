#pragma once
#include "concepts/axis_filter_concept.hpp"
#include "../../axis_base.hpp"
namespace comdare::cache_engine::filter::axis_filter {
template <typename Derived> class FilterBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    FilterBase() noexcept {
        static_assert(concepts::FilterStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};
}  // namespace
