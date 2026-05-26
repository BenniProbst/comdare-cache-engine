#pragma once
// V41.F.6.1.F1 axis_02_path_compression CRTP-Basis (2026-05-26 Skelett-Stufe-A)

#include "concepts/axis_02_path_compression_concept.hpp"
#include "../../axis_base.hpp"
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_02_path_compression {

template <typename Derived>
class PathCompressionBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    PathCompressionBase() noexcept {
        static_assert(concepts::PathCompressionStrategy<Derived>,
            "Pflicht: Derived muss PathCompressionStrategy erfuellen");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
            "Pflicht: Derived erfuellt AxisBaseConcept (via AxisBase Inheritance)");
    }
};

}  // namespace
