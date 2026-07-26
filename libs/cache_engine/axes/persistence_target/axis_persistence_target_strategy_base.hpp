#pragma once
// STRUKT-R ORG-18 axis_persistence_target CRTP-StrategyBase
// Vorbild 1:1 axes/io_dispatch/axis_io_strategy_base.hpp (CRTP + Concept-Guard, keine vtable).
//
// Benannter Pattern-Stapel (compile-time-only): CRTP (Coplien) + Layer Supertype (Fowler, PoEAA) unter
// dem Organ-Dach topics::OrganAxis<Derived> (axis_kind()==organ, Bau-INC-1a). Der Ctor-Guard prueft die
// Pflicht-Oberflaeche erst, wenn Derived vollstaendig ist.

#include "concepts/axis_persistence_target_concept.hpp"
#include "concepts/axis_persistence_target_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp>

namespace comdare::cache_engine::persistence_target {

template <typename Derived>
class PersistenceTargetStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
protected:
    PersistenceTargetStrategyBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante --
        // ohne sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren.
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");
        static_assert(concepts::PersistenceTargetStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::persistence_target
