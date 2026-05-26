#pragma once
// V41.F.6.1.F1 axis_04_node_type CRTP-Basis (2026-05-26 Skelett-Stufe-A)

#include "concepts/axis_04_node_type_concept.hpp"
#include "../../axis_base.hpp"
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_04_node_type {

template <typename Derived>
class NodeTypeBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    NodeTypeBase() noexcept {
        static_assert(concepts::NodeTypeStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
