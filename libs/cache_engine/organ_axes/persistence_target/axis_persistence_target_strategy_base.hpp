#pragma once
// STRUKT-R ORG-18 axis_persistence_target CRTP-StrategyBase
// Vorbild 1:1 organ_axes/io_dispatch/axis_io_strategy_base.hpp (CRTP + Concept-Guard, keine vtable).
//
// Benannter Pattern-Stapel (compile-time-only): CRTP (Coplien) + Layer Supertype (Fowler, PoEAA) unter
// dem Organ-Dach topics::OrganAxis<Derived> (axis_kind()==organ, Bau-INC-1a). Der Ctor-Guard prueft die
// Pflicht-Oberflaeche erst, wenn Derived vollstaendig ist.

#include "concepts/axis_persistence_target_concept.hpp"
#include "concepts/axis_persistence_target_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp>
#include <topics/organ_axis_error_classes.hpp> // FK-5: der Fehlerraum neben dem Versionsraum

namespace comdare::cache_engine::persistence_target {

template <typename Derived>
class PersistenceTargetStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
public:
    /// FK-5 (A15 EBENE 4, Fehlerraum) -- WELCHE D2-Fehlerklassen diese Organ-Achse ueberhaupt
    /// hervorbringen kann. Deklariert an DERSELBEN Stelle, die schon algo_version erzwingt (K2-Muster):
    /// eine Stelle je Achse statt einer Zeile je Varianten-Datei.
    /// Boden + n/a: memory_only ist die honest-0-Klasse dieser Achse (Katalog T17) -- die leeren Persistenz-Groessen sind n/a mit Grund, keine Fehler.
    [[nodiscard]] static constexpr auto error_classes() noexcept {
        return ::comdare::cache_engine::topics::kOrganAxisErrorFloorPlusNa;
    }

protected:
    PersistenceTargetStrategyBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante --
        // ohne sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren.
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");

        // FK-5 (E-24 C9): der Fehlerraum-Zwilling der Versions-Wache darueber -- existiert die
        // Deklaration, und ist sie nicht leer? Beides bricht compile-hart MIT dem Typ-Namen.
        ::comdare::cache_engine::topics::assert_organ_axis_error_classes<Derived>();
        static_assert(concepts::PersistenceTargetStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::persistence_target
