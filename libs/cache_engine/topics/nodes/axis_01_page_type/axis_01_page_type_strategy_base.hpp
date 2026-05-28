#pragma once
// V41.F.6.1 F.6 axis_01_page_type CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_01_page_type_concept.hpp"
#include "concepts/axis_01_page_type_cache_engine_permutation_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::nodes::axis_01_page_type {

template <typename Derived>
class PageTypeStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    PageTypeStrategyBase() noexcept {
        static_assert(concepts::PageTypeStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
