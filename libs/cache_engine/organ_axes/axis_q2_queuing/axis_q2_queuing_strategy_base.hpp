#pragma once
// V41.F.6.1.R7.3 axis_q2_queuing CRTP-StrategyBase + Concept-Guard (Goldstandard-Nachzug)
//
// @topic queuing
// @achse Q2 flush_policy
//
// Schliesst die Goldstandard-Luecke: bisher erbten die Q2-Flush-Wrapper DIREKT von
// AxisBase (Pattern-Abweichung vs. axis_q1 BufferStrategyBase / axis_08
// ConcurrencyStrategyBase / axis_14 ValueHandleStrategyBase). Diese CRTP-Basis stellt
// den Concept-Guard im Konstruktor sicher (CRTP-Henne-Ei-Pattern aus Allocator-Achse:
// Concept-Check in `requires`-Template-Klausel funktioniert NICHT zur Vererbung — daher
// static_assert hier).

#include "concepts/axis_q2_queuing_concept.hpp"
#include <topics/organ_axis.hpp>               // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)
#include <topics/organ_axis_error_classes.hpp> // FK-5: der Fehlerraum neben dem Versionsraum
#include "concepts/axis_q2_queuing_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>

#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q2_queuing {

/**
 * @brief FlushPolicyStrategyBase — CRTP-Basis fuer Q2-Flush-Policy-Wrapper
 *
 * Erbt von ::topics::AxisBase fuer cross-axis Pflicht-Property get_compiler()
 * (Default "original", per Wrapper ueberschreibbar) + is_original_module()=false.
 */
template <typename Derived>
class FlushPolicyStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
public:
    /// FK-5 (A15 EBENE 4, Fehlerraum) -- WELCHE D2-Fehlerklassen diese Organ-Achse ueberhaupt
    /// hervorbringen kann. Deklariert an DERSELBEN Stelle, die schon algo_version erzwingt (K2-Muster):
    /// eine Stelle je Achse statt einer Zeile je Varianten-Datei.
    /// Boden: die Flush-Politik entscheidet an realen Fuellstaenden.
    [[nodiscard]] static constexpr auto error_classes() noexcept {
        return ::comdare::cache_engine::topics::kOrganAxisErrorFloor;
    }

protected:
    FlushPolicyStrategyBase() noexcept {
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
        static_assert(concepts::FlushPolicy<Derived>, "Pflicht: Derived muss FlushPolicy erfuellen "
                                                      "(should_flush/on_flush_complete + topic_tag)");
        static_assert(concepts::CacheEngineFlushPolicyPermutationStrategy<Derived>,
                      "Pflicht: Derived erfuellt CacheEngineFlushPolicyPermutationStrategy "
                      "(axis_tag/family_id/name/family_name/flag_suffix/enabled)");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
                      "Pflicht: Derived erfuellt AxisBaseConcept (get_compiler() Default 'original' via AxisBase)");
    }
};

} // namespace comdare::cache_engine::queuing::axis_q2_queuing
