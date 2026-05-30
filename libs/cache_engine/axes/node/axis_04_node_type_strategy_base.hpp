#pragma once
// V41.F.6.1.R7.1.d axis_04_node_type CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_04_node_type_concept.hpp"
#include "concepts/axis_04_node_type_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>

namespace comdare::cache_engine::node {

template <typename Derived>
class NodeTypeStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    NodeTypeStrategyBase() noexcept {
        static_assert(concepts::NodeTypeStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
