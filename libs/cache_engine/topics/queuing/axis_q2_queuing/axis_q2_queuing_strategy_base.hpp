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
#include <topics/organ_axis.hpp> // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)
#include "concepts/axis_q2_queuing_cache_engine_permutation_concept.hpp"
#include "../../axis_base.hpp"

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
protected:
    FlushPolicyStrategyBase() noexcept {
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
