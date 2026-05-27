#pragma once
// V41.F.6.1.R7.1.c axis_02_path_compression CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_02_path_compression_concept.hpp"
#include "concepts/axis_02_path_compression_cache_engine_permutation_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::nodes::axis_02_path_compression {

template <typename Derived>
class PathCompressionStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    PathCompressionStrategyBase() noexcept {
        static_assert(concepts::PathCompressionStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
