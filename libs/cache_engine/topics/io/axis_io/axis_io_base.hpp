#pragma once
#include "concepts/axis_io_concept.hpp"
#include "../../axis_base.hpp"
namespace comdare::cache_engine::io::axis_io {
template <typename Derived> class IoBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    IoBase() noexcept {
        static_assert(concepts::IoStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};
}  // namespace
