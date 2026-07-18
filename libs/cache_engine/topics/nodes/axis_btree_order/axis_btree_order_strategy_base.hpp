#pragma once
// DOSSIER W1/234-K axis_btree_order CRTP-StrategyBase

#include "concepts/axis_btree_order_cache_engine_permutation_concept.hpp"
#include <topics/organ_axis.hpp> // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)
#include "concepts/axis_btree_order_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::nodes::axis_btree_order {

struct btree_order_family_tag {};

template <typename Derived>
class BtreeOrderStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
protected:
    BtreeOrderStrategyBase() noexcept {
        static_assert(concepts::BtreeOrderShape<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::nodes::axis_btree_order