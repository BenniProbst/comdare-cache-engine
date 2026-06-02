#pragma once
// V41.F.6.1.R7.1.d axis_04_node_type CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_04_node_type_concept.hpp"
#include "concepts/axis_04_node_type_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <axes/cacheline/cacheline_config.hpp>  // KF-5: per-Organ Cache-Line-Unterachse

namespace comdare::cache_engine::node {

// KF-5 (2026-06-02): defaulted NTTP CacheLineCfg + CacheLineAware<Cfg> → jeder Node-Typ ist cacheline-fähig.
// Default {} = unverändert (nicht-brechend, ODR-sicher).
template <typename Derived,
          ::comdare::cache_engine::cacheline::CacheLineConfig CacheLineCfg = ::comdare::cache_engine::cacheline::CacheLineConfig{}>
class NodeTypeStrategyBase
    : public ::comdare::cache_engine::topics::AxisBase
    , public ::comdare::cache_engine::cacheline::CacheLineAware<CacheLineCfg> {
protected:
    NodeTypeStrategyBase() noexcept {
        static_assert(concepts::NodeTypeStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
