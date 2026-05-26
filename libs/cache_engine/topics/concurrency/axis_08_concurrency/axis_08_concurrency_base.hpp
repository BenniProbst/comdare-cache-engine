#pragma once
// V41.F.6.1.F1 axis_08_concurrency CRTP-Basis (Skelett-Stufe-A)

#include "concepts/axis_08_concurrency_concept.hpp"
#include "../../axis_base.hpp"
#include <type_traits>

namespace comdare::cache_engine::concurrency::axis_08_concurrency {

template <typename Derived>
class ConcurrencyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    ConcurrencyBase() noexcept {
        static_assert(concepts::ConcurrencyStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
