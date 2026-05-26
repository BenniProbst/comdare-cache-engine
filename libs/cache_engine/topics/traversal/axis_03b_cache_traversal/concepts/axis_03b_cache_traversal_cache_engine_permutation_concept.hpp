#pragma once
// V41.F.6.1 axis_03b_cache_traversal cache-engine-Pflicht-Concept (2026-05-26)

#include "axis_03b_cache_traversal_concept.hpp"
#include "../axis_03b_cache_traversal_subaxes_ct1_to_ct2.hpp"

#include <measurement/measurable_concept.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::traversal::axis_03b_cache_traversal::concepts {

/**
 * @brief CacheTraversalStatistics — Pflicht-Felder fuer Cache-Traversal Mess-Reihen
 */
struct CacheTraversalStatistics {
    std::uint64_t total_resolve_count       = 0;
    std::uint64_t total_resolve_hit_count   = 0;
    std::uint64_t total_resolve_miss_count  = 0;
    std::uint64_t total_register_count      = 0;
    std::uint64_t total_unregister_count    = 0;
    std::uint64_t peak_tracked              = 0;
    double        avg_collision_chain_length = 0.0;
};

/**
 * @brief CacheEngineCacheTraversalPermutationStrategy — cache-engine-Pflicht-API
 *
 * Sonderfall-Properties:
 *   - is_hashed()           — Hash-basiert vs linear?
 *   - has_collision_chains() — Linear-Probing/Chaining?
 *   - amortized_o1()        — O(1) amortisiert (vs O(N) linear)?
 */
template <typename T>
concept CacheEngineCacheTraversalPermutationStrategy =
    ::comdare::cache_engine::traversal::concepts::TraversalComponent<T>
    && requires {
        typename T::axis_tag;
        typename T::family_id;
        { T::is_thread_safe()      } -> std::convertible_to<bool>;
        { T::name()                } -> std::convertible_to<std::string_view>;
        { T::family_name()         } -> std::convertible_to<std::string_view>;
        { T::flag_suffix()         } -> std::convertible_to<std::string_view>;
        // Sonderfall-Properties
        { T::is_hashed()           } -> std::convertible_to<bool>;
        { T::has_collision_chains() } -> std::convertible_to<bool>;
        { T::amortized_o1()        } -> std::convertible_to<bool>;
    }
#ifdef COMDARE_CE_ENABLE_STATISTICS
    && requires(T t, T const& tc) {
        { tc.statistics() } noexcept;
        { t.reset() }      noexcept;
    }
    && requires {
        typename T::snapshot_t;
        typename T::observer_t;
    }
    && std::same_as<typename T::observer_t,
                    ::comdare::cache_engine::measurement::MeasurableObserver<typename T::snapshot_t>>
    && requires(T const& tc) {
        { tc.observer() } noexcept -> std::same_as<typename T::observer_t const&>;
    }
#endif
    ;

}  // namespace
