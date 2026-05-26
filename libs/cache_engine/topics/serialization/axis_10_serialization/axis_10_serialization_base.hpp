#pragma once
#include "concepts/axis_10_serialization_concept.hpp"
#include "../../axis_base.hpp"
namespace comdare::cache_engine::serialization::axis_10_serialization {
template <typename Derived> class SerializationBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    SerializationBase() noexcept {
        static_assert(concepts::SerializationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};
}  // namespace
