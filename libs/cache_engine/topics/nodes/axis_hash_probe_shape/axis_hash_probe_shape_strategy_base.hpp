#pragma once
// DOSSIER W1/234-K axis_hash_probe_shape CRTP-StrategyBase

#include "concepts/axis_hash_probe_shape_cache_engine_permutation_concept.hpp"
#include "concepts/axis_hash_probe_shape_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::nodes::axis_hash_probe_shape {

struct hash_probe_shape_family_tag {};

template <typename Derived>
class HashProbeShapeStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    HashProbeShapeStrategyBase() noexcept {
        static_assert(concepts::HashProbeShape<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::nodes::axis_hash_probe_shape