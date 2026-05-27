#pragma once
// V41.F.6.1.R7.1 axis_12 General-Hardware CRTP-Base
#include "concepts/axis_12_general_hardware_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::hardware::axis_12_general_hardware {

template <typename Derived>
class GeneralHardwareBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    GeneralHardwareBase() noexcept {
        static_assert(concepts::GeneralHardwareStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace comdare::cache_engine::hardware::axis_12_general_hardware
