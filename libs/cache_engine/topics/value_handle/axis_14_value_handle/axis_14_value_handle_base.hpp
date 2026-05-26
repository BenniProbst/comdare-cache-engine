#pragma once
#include "concepts/axis_14_value_handle_concept.hpp"
#include "../../axis_base.hpp"
namespace comdare::cache_engine::value_handle::axis_14_value_handle {
template <typename Derived> class ValueHandleBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    ValueHandleBase() noexcept {
        static_assert(concepts::ValueHandleStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};
}  // namespace
