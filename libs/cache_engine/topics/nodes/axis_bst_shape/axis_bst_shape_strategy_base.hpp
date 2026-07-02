#pragma once
// DOSSIER W1/234-K axis_bst_shape CRTP-StrategyBase

#include "concepts/axis_bst_shape_cache_engine_permutation_concept.hpp"
#include "concepts/axis_bst_shape_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::nodes::axis_bst_shape {

struct bst_shape_family_tag {};

template <typename Derived>
class BstShapeStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    BstShapeStrategyBase() noexcept {
        static_assert(concepts::BstShape<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::nodes::axis_bst_shape