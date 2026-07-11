#pragma once
// V41.F.6.1 axis_03m_mapping cache-engine-Pflicht-Concept (2026-05-26)

#include "axis_03m_mapping_concept.hpp"
#include "../axis_03m_mapping_subaxes_mp1_to_mp2.hpp"

#include <measurement/measurable_concept.hpp>
#include <concepts/legacy_original_code_strategy_concept.hpp> // V41.F.6.1.P2.C Habich-Compliance Pflicht-API

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::mapping::concepts {

struct MappingStatistics {
    std::uint64_t total_register_count       = 0;
    std::uint64_t total_resolve_count        = 0;
    std::uint64_t total_resolve_hit_count    = 0;
    std::uint64_t total_resolve_miss_count   = 0;
    std::uint64_t total_reverse_lookup_count = 0;
    std::uint64_t peak_mapped                = 0;
    std::uint64_t total_indirection_steps =
        0; // T2 Mess-Objective (Indirektions-CM): kumulierte Adress-Aufloesungs-Tiefe je resolve. DirectPlacement(MP01)=1 (Offset absolut gespeichert), PoolRelative(MP02)=2 (Lookup + pool_base-Rebase). Filter-lokal, NICHT Teil des eingefrorenen 1416-POD.
};

/**
 * @brief CacheEngineMappingPermutationStrategy — cache-engine-Pflicht-API
 *
 * Sonderfall-Properties:
 *   - is_pool_relative()      — pool-relative Offsets vs absolute?
 *   - supports_reverse_lookup() — O(1)-rueckwaerts-Mapping?
 *   - requires_pool_base()    — Constructor braucht pool_base_address?
 */
template <typename M>
concept CacheEngineMappingPermutationStrategy =
    ::comdare::cache_engine::traversal::concepts::TraversalComponent<M> &&
    requires {
        typename M::axis_tag;
        typename M::family_id;
        { M::is_thread_safe() } -> std::convertible_to<bool>;
        { M::name() } -> std::convertible_to<std::string_view>;
        { M::family_name() } -> std::convertible_to<std::string_view>;
        { M::flag_suffix() } -> std::convertible_to<std::string_view>;
        // Sonderfall-Properties
        { M::is_pool_relative() } -> std::convertible_to<bool>;
        { M::supports_reverse_lookup() } -> std::convertible_to<bool>;
        { M::requires_pool_base() } -> std::convertible_to<bool>;
    }
#ifdef COMDARE_CE_ENABLE_STATISTICS
    &&
    requires(M m, M const& mc) {
        { mc.statistics() } noexcept;
        { m.reset() } noexcept;
    } &&
    requires {
        typename M::snapshot_t;
        typename M::observer_t;
    } &&
    std::same_as<typename M::observer_t,
                 ::comdare::cache_engine::measurement::MeasurableObserver<typename M::snapshot_t>> &&
    requires(M const& mc) {
        { mc.observer() } noexcept -> std::same_as<typename M::observer_t const&>;
    }
#endif
    // V41.F.6.1.P2.C Habich-Compliance: get_compiler + is_original_module (cross-axis via AxisBase Default)
    && ::comdare::cache_engine::concepts::LegacyOriginalCodePflicht<M>;

} // namespace comdare::cache_engine::mapping::concepts
