#pragma once
// V41.F.6.1 axis_03m_mapping Standard-Concept (2026-05-26)
//
// @topic traversal @achse 03m mapping
//
// Pflicht-API: Mapping von logischen Slot-Indizes zu physischen Offsets
// (analog prt-art VirtualOffsetAddress + placement_page_ Pattern).
//
// Pflicht-Methoden:
//   - typename slot_index_type  (std::uint16_t)
//   - typename offset_type      (std::size_t)
//   - typename size_type
//   - register_slot(slot, offset) -> void
//   - resolve_offset(slot) const  -> std::optional<offset_type>
//   - reverse_lookup(offset) const -> std::optional<slot_index_type>
//   - mapped_count() const noexcept -> size_type
//   - clear() noexcept            -> void

#include "../../concepts/topic_traversal_concept.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::traversal::axis_03m_mapping::concepts {

/**
 * @brief MappingVariant — Pflicht-API fuer Mapping-Strategien
 *
 * Topic-uebergreifend einheitliche API fuer Slot-zu-Offset-Resolution.
 * Konkrete Wrapper (DirectPlacement, PoolRelative) erfuellen dies + bringen
 * jeweils ihre Charakteristiken mit (direkt vs pool-relativ).
 */
template <typename M>
concept MappingVariant =
    ::comdare::cache_engine::traversal::concepts::TraversalComponent<M>
    && requires { typename M::slot_index_type; typename M::offset_type; typename M::size_type; }
    && requires(M m, typename M::slot_index_type s, typename M::offset_type o) {
        { m.register_slot(s, o) }     -> std::same_as<void>;
    }
    && requires(M const& mc, typename M::slot_index_type s, typename M::offset_type o) {
        { mc.resolve_offset(s) }      -> std::same_as<std::optional<typename M::offset_type>>;
        { mc.reverse_lookup(o) }      -> std::same_as<std::optional<typename M::slot_index_type>>;
        { mc.mapped_count() } noexcept -> std::convertible_to<std::size_t>;
    }
    && requires(M m) {
        { m.clear() }                 noexcept;
    };

}  // namespace
