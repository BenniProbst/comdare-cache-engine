#pragma once
// V41.F.6.1.R7.5.f axis_io CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_io_concept.hpp"
#include "concepts/axis_io_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp> // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ) // V41.F.2: stabile Include-Dir-Form (war "../../axis_base.hpp", bricht nach Move)

namespace comdare::cache_engine::io_dispatch {

template <typename Derived>
class IoStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
protected:
    IoStrategyBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante — ohne
        // sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren. CRTP-Ctor-Guard;
        // universell zusaetzlich via build_axis_variant_version_table() (Typ-Ebene, alle 17 Kompositions-Registries).
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");
        static_assert(concepts::IoStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::io_dispatch
