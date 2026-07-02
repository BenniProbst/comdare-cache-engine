#pragma once
// DOSSIER W1/234-K axis_skip_list_shape CRTP-StrategyBase

#include "concepts/axis_skip_list_shape_cache_engine_permutation_concept.hpp"
#include "concepts/axis_skip_list_shape_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::nodes::axis_skip_list_shape {

struct skip_list_shape_family_tag {};

template <typename Derived>
class SkipListShapeStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    SkipListShapeStrategyBase() noexcept {
        static_assert(concepts::SkipListShape<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::nodes::axis_skip_list_shape