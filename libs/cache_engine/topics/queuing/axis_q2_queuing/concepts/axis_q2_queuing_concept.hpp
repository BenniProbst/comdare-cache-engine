#pragma once
// V41.F.6.1 axis_q2_queuing Standard-Concept (2026-05-26)
// @topic queuing @achse Q2 flush_policy

#include "../../concepts/topic_queuing_concept.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::queuing::axis_q2_queuing::concepts {

/**
 * @brief FlushDecision — Resultat von should_flush()
 *
 * Triggert die Buffer-Strategy zum Auslesen.
 */
enum class FlushDecision : std::uint8_t {
    NoFlush     = 0,
    PartialFlush = 1,  // Teilmenge spuelen
    FullFlush   = 2,   // alles spuelen
};

/**
 * @brief FlushPolicy — Pflicht-API fuer Q2 Flush-Policies
 *
 * Eine Policy entscheidet WANN ein Buffer gespuelt wird. Sie wird vom
 * Buffer-Wrapper periodisch (oder nach jeder put/get-Op) aufgerufen.
 */
template <typename P>
concept FlushPolicy =
    ::comdare::cache_engine::queuing::concepts::QueuingComponent<P>
    && requires(P p, std::size_t current_fill, std::size_t capacity) {
        { p.should_flush(current_fill, capacity) }
            noexcept -> std::convertible_to<FlushDecision>;
    }
    && requires(P p) {
        { p.on_flush_complete() } noexcept;
    };

}  // namespace
