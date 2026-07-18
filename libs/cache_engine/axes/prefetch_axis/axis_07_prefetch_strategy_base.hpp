#pragma once
// V41.F.6.1.R7.5.a axis_07_prefetch CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_07_prefetch_concept.hpp"
#include "concepts/axis_07_prefetch_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp> // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)

namespace comdare::cache_engine::prefetch_axis {

template <typename Derived>
class PrefetchStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
protected:
    PrefetchStrategyBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante — ohne
        // sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren. CRTP-Ctor-Guard;
        // universell zusaetzlich via build_axis_variant_version_table() (Typ-Ebene, alle 17 Kompositions-Registries).
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");
        static_assert(concepts::PrefetchStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::prefetch_axis
