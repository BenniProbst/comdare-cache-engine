#pragma once
// V41.F.6.1 axis_03m_mapping Sub-Concept PoolRebasableStrategy (2026-05-26)
//
// @topic traversal @achse 03m
//
// Sub-Concept fuer Mapping-Strategien mit Pool-Reallocation-Faehigkeit
// (Pool kann komplett umalloziert werden ohne Mapping-Tabelle anzufassen,
// nur pool_base wird neu gesetzt).

#include "axis_03m_mapping_concept.hpp"

#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::mapping::concepts {

/**
 * @brief PoolRebasableStrategy — Pflicht-API fuer pool-rebasable Mapping-Strategien
 *
 * Verfeinerung des MappingVariant-Basiskonzepts. Wrapper-Klassen mit
 * `requires_pool_base() == true` MUSS dieses Sub-Concept erfuellen.
 *
 * **Concept-Erfuellung:** PoolRelative (rebase + pool_base Accessor).
 * **Nicht erfuellt von:** DirectPlacement (absolute Offsets, kein Pool).
 *
 * Semantik:
 *   - pool_base()        — aktueller pool_base_address (offset_type)
 *   - rebase(new_base)   — pool_base umsetzen; alle relativen Offsets bleiben
 *                          semantisch korrekt, nur absolute Position aendert sich
 */
template <typename M>
concept PoolRebasableStrategy = MappingVariant<M> && requires(M const& mc) {
    { mc.pool_base() } noexcept -> std::convertible_to<typename M::offset_type>;
} && requires(M m, typename M::offset_type new_base) {
    { m.rebase(new_base) } noexcept;
};

} // namespace comdare::cache_engine::mapping::concepts
