#pragma once
// V41.F.6.1.R7.1.d axis_04_node_type CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_04_node_type_concept.hpp"
#include "concepts/axis_04_node_type_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp>                // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)
#include <axes/cacheline/cacheline_config.hpp>  // KF-5: per-Organ Cache-Line-Unterachse
#include <axes/cacheline/node_width_config.hpp> // C2/FF2: Knoten-Breite-in-Cache-Lines-Unterachse

namespace comdare::cache_engine::node {

// KF-5 (2026-06-02): defaulted NTTP CacheLineCfg + CacheLineAware<Cfg> → jeder Node-Typ ist cacheline-fähig.
// Default {} = unverändert (nicht-brechend, ODR-sicher).
// C2 (GO4/#8 F-C, 2026-07-12): zweiter defaulted NTTP NodeWidthCfg + NodeWidthAware<Cfg> → jeder Node-Typ
// trägt die FF2-Unterachse „Knoten-Breite in Cache-Lines" (1..16). Default {} = Native (0) = keine Vorgabe →
// Verhalten/Bytes unverändert (exakt das KF-5-Muster: Blätter bleiben konkrete Klassen, Registry-mp_list
// unberührt, golden-/Gate-1-neutral). Konsum: axis_04_node_type_layout_aware_store.hpp (Chunk-Breite).
template <typename Derived,
          ::comdare::cache_engine::cacheline::CacheLineConfig CacheLineCfg =
              ::comdare::cache_engine::cacheline::CacheLineConfig{},
          ::comdare::cache_engine::cacheline::NodeWidthConfig NodeWidthCfg =
              ::comdare::cache_engine::cacheline::NodeWidthConfig{}>
class NodeTypeStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived>,
                             public ::comdare::cache_engine::cacheline::CacheLineAware<CacheLineCfg>,
                             public ::comdare::cache_engine::cacheline::NodeWidthAware<NodeWidthCfg> {
protected:
    NodeTypeStrategyBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante — ohne
        // sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren. CRTP-Ctor-Guard;
        // universell zusaetzlich via build_axis_variant_version_table() (Typ-Ebene, alle 17 Kompositions-Registries).
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");
        static_assert(concepts::NodeTypeStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::node
