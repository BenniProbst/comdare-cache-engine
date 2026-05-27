#pragma once
// V41.F.6.1 axis_q1_queuing Standard-Concept (2026-05-26)
//
// @topic queuing
// @achse Q1 buffer_strategy
// @stand V41.F.6.1 Pilot
//
// Pflicht-API (Standard, unabhaengig von cache-engine):
//   - typename element_type   (typischerweise std::uint64_t fuer Cache-Engine)
//   - typename size_type      (std::size_t)
//   - put(element)           -> void (Einfuegen)
//   - get()                  -> std::optional<element_type> (Entnehmen)
//   - size() const noexcept  -> size_type (aktuelle Anzahl)
//   - clear() noexcept       -> void (alle Eintraege entfernen)

#include "../../concepts/topic_queuing_concept.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::queuing::axis_q1_queuing::concepts {

/**
 * @brief BufferStrategy — Pflicht-API fuer Q1 Buffer-Strategien
 *
 * Topic-uebergreifend einheitliche Buffer-API analog std::queue. Konkrete
 * Implementations (FIFOQueueBuffer, LIFOStackBuffer, BoundedRingBuffer, ...) erfuellen
 * dies + bringen jeweils ihre Charakteristiken mit.
 *
 * Pflicht-API (analog std::queue + std::deque hybrid):
 *   - put(v)         — Einfuegen (analog push_back)
 *   - get()          — Entnehmen mit Resultat (analog pop_front + front)
 *   - peek_front()   — Anschauen ohne Entfernen (front)
 *   - peek_back()    — Anschauen ohne Entfernen (back)
 *   - emplace(args...) — In-place Konstruktion (typischerweise = put(T{args...}))
 *   - size() / is_empty() / clear()
 */
template <typename B>
concept BufferStrategy =
    ::comdare::cache_engine::queuing::concepts::QueuingComponent<B>
    && requires { typename B::element_type; typename B::size_type; }
    && requires(B b, typename B::element_type v) {
        { b.put(v) }                        -> std::same_as<void>;
        { b.get() }                         -> std::same_as<std::optional<typename B::element_type>>;
        { b.emplace(v) }                    -> std::same_as<void>;
    }
    && requires(B const& bc) {
        { bc.size() }       noexcept -> std::convertible_to<std::size_t>;
        { bc.is_empty() }   noexcept -> std::convertible_to<bool>;
        { bc.peek_front() } noexcept -> std::same_as<std::optional<typename B::element_type>>;
        { bc.peek_back() }  noexcept -> std::same_as<std::optional<typename B::element_type>>;
    }
    && requires(B b) {
        { b.clear() }       noexcept;
    };

}  // namespace
