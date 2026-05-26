#pragma once
// V41.F.6.1 axis_q1_buffer_strategy CRTP-Basis + Concept-Guard (2026-05-26)
//
// @topic queuing
// @achse Q1 buffer_strategy

#include "concepts/axis_q1_buffer_strategy_concept.hpp"

#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q1_buffer_strategy {

/**
 * @brief BufferStrategyBase — CRTP-Basis fuer Q1-Wrapper
 *
 * Concept-Guard via static_assert im Konstruktor (CRTP-Henne-Ei-Pattern aus
 * Allocator-Achse: Concept-Check in `requires` Template-Klausel funktioniert
 * NICHT zur Vererbung — daher static_assert hier).
 */
template <typename Derived>
class BufferStrategyBase {
protected:
    BufferStrategyBase() noexcept {
        static_assert(concepts::BufferStrategy<Derived>,
            "Pflicht: Derived muss BufferStrategy erfuellen "
            "(put/get/size/is_empty/clear + element_type/size_type/topic_tag)");
    }
};

}  // namespace
