#pragma once
// V41.F.6.1 axis_q1_queuing CRTP-Basis + Concept-Guard (2026-05-26)
//
// @topic queuing
// @achse Q1 buffer_strategy

#include "concepts/axis_q1_queuing_concept.hpp"
#include <topics/organ_axis.hpp> // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)
#include "../../axis_base.hpp"

#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

/**
 * @brief BufferStrategyBase — CRTP-Basis fuer Q1-Wrapper
 *
 * Concept-Guard via static_assert im Konstruktor (CRTP-Henne-Ei-Pattern aus
 * Allocator-Achse: Concept-Check in `requires` Template-Klausel funktioniert
 * NICHT zur Vererbung — daher static_assert hier).
 *
 * Erbt von ::topics::AxisBase fuer cross-axis Pflicht-Property get_compiler()
 * (Default "original", per Wrapper ueberschreibbar).
 */
template <typename Derived>
class BufferStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
protected:
    BufferStrategyBase() noexcept {
        static_assert(concepts::BufferStrategy<Derived>,
                      "Pflicht: Derived muss BufferStrategy erfuellen "
                      "(put/get/size/is_empty/clear + element_type/size_type/topic_tag)");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
                      "Pflicht: Derived erfuellt AxisBaseConcept (get_compiler() Default 'original' + "
                      "is_original_module = false via AxisBase)");
    }
    // V41.F.6.1.P2.C ENTFERNT: Defaults kommen via AxisBase (cross-axis generisch).
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing
