#pragma once
// V41.F.6.1.F1 axis_07_prefetch CRTP-Basis (Skelett-Stufe-A)

#include "concepts/axis_07_prefetch_concept.hpp"
#include "../../axis_base.hpp"
#include <type_traits>

namespace comdare::cache_engine::prefetch::axis_07_prefetch {

template <typename Derived>
class PrefetchBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    PrefetchBase() noexcept {
        static_assert(concepts::PrefetchStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
