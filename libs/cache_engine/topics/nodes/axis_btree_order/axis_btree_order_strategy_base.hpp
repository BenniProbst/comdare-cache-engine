#pragma once
// DOSSIER W1/234-K axis_btree_order CRTP-StrategyBase

#include "concepts/axis_btree_order_cache_engine_permutation_concept.hpp"
#include "concepts/axis_btree_order_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::nodes::axis_btree_order {

struct btree_order_family_tag {};

template <typename Derived>
class BtreeOrderStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    BtreeOrderStrategyBase() noexcept {
        static_assert(concepts::BtreeOrderShape<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::nodes::axis_btree_order