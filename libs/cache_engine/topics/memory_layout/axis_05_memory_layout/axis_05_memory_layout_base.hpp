#pragma once
// V41.F.6.1.F1 axis_05_memory_layout CRTP-Basis (Skelett-Stufe-A)

#include "concepts/axis_05_memory_layout_concept.hpp"
#include "../../axis_base.hpp"
#include <type_traits>

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {

template <typename Derived>
class MemoryLayoutBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    MemoryLayoutBase() noexcept {
        static_assert(concepts::MemoryLayoutStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
