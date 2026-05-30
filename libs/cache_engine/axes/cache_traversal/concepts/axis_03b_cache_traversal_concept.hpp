#pragma once
// V41.F.6.1 axis_03b_cache_traversal Standard-Concept (2026-05-26)
//
// @topic traversal @achse 03b cache_traversal
//
// Pflicht-API: cache-bewusste Traversal-Strategien (wie wird die Cache-Engine
// bei Lookups durchlaufen). Inspiriert von prt-art IFanout::lookup_page +
// IRootNode-Traversal.
//
// Pflicht-Methoden:
//   - typename key_type   (std::uint64_t)
//   - typename value_type (std::uint64_t)
//   - typename size_type
//   - register_entry(key, value) -> void
//   - resolve(key) const         -> std::optional<value_type>
//   - unregister(key)            -> bool
//   - tracked_count() const noexcept -> size_type
//   - clear() noexcept           -> void

#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::cache_traversal::concepts {

/**
 * @brief CacheTraversalVariant — Pflicht-API fuer Cache-Traversal-Strategien
 *
 * Topic-uebergreifend einheitliche API fuer Cache-Engine-Lookup-Dispatch.
 * Konkrete Wrapper (LinearFanout, HashLookup) erfuellen dies + bringen
 * jeweils ihre Charakteristiken mit (linear vs hash, latency vs locality).
 */
template <typename T>
concept CacheTraversalVariant =
    ::comdare::cache_engine::traversal::concepts::TraversalComponent<T>
    && requires { typename T::key_type; typename T::value_type; typename T::size_type; }
    && requires(T t, typename T::key_type k, typename T::value_type v) {
        { t.register_entry(k, v) }    -> std::same_as<void>;
        { t.unregister(k) }           -> std::same_as<bool>;
    }
    && requires(T const& tc, typename T::key_type k) {
        { tc.resolve(k) }             -> std::same_as<std::optional<typename T::value_type>>;
        { tc.tracked_count() } noexcept -> std::convertible_to<std::size_t>;
    }
    && requires(T t) {
        { t.clear() }                 noexcept;
    };

}  // namespace
