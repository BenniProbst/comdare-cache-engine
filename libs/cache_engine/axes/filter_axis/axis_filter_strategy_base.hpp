#pragma once
// V41.F.6.1.R7.5.e axis_filter CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_filter_concept.hpp"
#include "concepts/axis_filter_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp>               // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)
#include <topics/organ_axis_error_classes.hpp> // FK-5: der Fehlerraum neben dem Versionsraum

namespace comdare::cache_engine::filter_axis {

template <typename Derived>
class FilterStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
public:
    /// FK-5 (A15 EBENE 4, Fehlerraum) -- WELCHE D2-Fehlerklassen diese Organ-Achse ueberhaupt
    /// hervorbringen kann. Deklariert an DERSELBEN Stelle, die schon algo_version erzwingt (K2-Muster):
    /// eine Stelle je Achse statt einer Zeile je Varianten-Datei.
    /// Boden: der Filter wird real befragt (auch die None-Variante ist ein Durchlauf).
    [[nodiscard]] static constexpr auto error_classes() noexcept {
        return ::comdare::cache_engine::topics::kOrganAxisErrorFloor;
    }

protected:
    FilterStrategyBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante — ohne
        // sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren. CRTP-Ctor-Guard;
        // universell zusaetzlich via build_axis_variant_version_table() (Typ-Ebene, alle 17 Kompositions-Registries).
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");

        // FK-5 (E-24 C9): der Fehlerraum-Zwilling der Versions-Wache darueber -- existiert die
        // Deklaration, und ist sie nicht leer? Beides bricht compile-hart MIT dem Typ-Namen.
        ::comdare::cache_engine::topics::assert_organ_axis_error_classes<Derived>();
        static_assert(concepts::FilterStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::filter_axis
