#pragma once
// V41.F.6.1 F.6 axis_01_page_type CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_01_page_type_concept.hpp"
#include "concepts/axis_01_page_type_cache_engine_permutation_concept.hpp"
#include "../../axis_base.hpp"
#include <axes/cacheline/cacheline_config.hpp>  // KF-5: per-Organ Cache-Line-Unterachse

namespace comdare::cache_engine::nodes::axis_01_page_type {

// KF-5 (2026-06-02): defaulted NTTP CacheLineCfg + CacheLineAware<Cfg> → jeder Page-Type ist cacheline-fähig.
// Default {} = unverändert (nicht-brechend, ODR-sicher).
template <typename Derived,
          ::comdare::cache_engine::cacheline::CacheLineConfig CacheLineCfg = ::comdare::cache_engine::cacheline::CacheLineConfig{}>
class PageTypeStrategyBase
    : public ::comdare::cache_engine::topics::AxisBase
    , public ::comdare::cache_engine::cacheline::CacheLineAware<CacheLineCfg> {
protected:
    PageTypeStrategyBase() noexcept {
        static_assert(concepts::PageTypeStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
