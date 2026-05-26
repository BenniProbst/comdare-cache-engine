#pragma once
#include "concepts/axis_09_isa_concept.hpp"
#include "../../axis_base.hpp"
namespace comdare::cache_engine::hardware::axis_09_isa {
template <typename Derived> class IsaBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    IsaBase() noexcept {
        static_assert(concepts::IsaStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};
}  // namespace
