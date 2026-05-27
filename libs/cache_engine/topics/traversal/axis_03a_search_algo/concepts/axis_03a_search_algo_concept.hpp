#pragma once
// V41.F.6.1 axis_03a_search_algo Standard-Concept (2026-05-26)
//
// @topic traversal
// @achse 03a search_algo
// @stand V41.F.6.1 Pilot
//
// Pflicht-API (Standard, unabhaengig von cache-engine):
//   - typename key_type   (std::uint8_t fuer Single-Byte-Discriminator-Achsen)
//   - typename value_type (std::uint64_t fuer Cache-Engine-Slot-Values)
//   - typename size_type  (std::size_t)
//   - insert(key, value)         -> void
//   - lookup(key) const          -> std::optional<value_type>
//   - erase(key)                 -> bool (true wenn entfernt)
//   - occupied_count() const noexcept -> size_type
//   - density_percent() const noexcept -> double  (0.0-100.0)
//   - clear() noexcept           -> void

#include "../../concepts/topic_traversal_concept.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::concepts {

/**
 * @brief SearchAlgoVariant — Pflicht-API fuer Such-Algorithmus-Varianten
 *
 * Topic-uebergreifend einheitliche Such-API analog std::map<K,V>. Konkrete
 * Wrapper (Array256SearchAlgo, VectorU8U8SearchAlgo, VectorU16U16SearchAlgo) erfuellen dies + bringen
 * jeweils ihre Charakteristiken mit (dense/sparse/multilevel, SIMD/non-SIMD).
 */
template <typename S>
concept SearchAlgoVariant =
    ::comdare::cache_engine::traversal::concepts::TraversalComponent<S>
    && requires { typename S::key_type; typename S::value_type; typename S::size_type; }
    && requires(S s, typename S::key_type k, typename S::value_type v) {
        { s.insert(k, v) }              -> std::same_as<void>;
        { s.erase(k) }                  -> std::same_as<bool>;
    }
    && requires(S const& sc, typename S::key_type k) {
        { sc.lookup(k) }                -> std::same_as<std::optional<typename S::value_type>>;
        { sc.occupied_count() } noexcept -> std::convertible_to<std::size_t>;
        { sc.density_percent() } noexcept -> std::convertible_to<double>;
    }
    && requires(S s) {
        { s.clear() }                   noexcept;
    };

}  // namespace
