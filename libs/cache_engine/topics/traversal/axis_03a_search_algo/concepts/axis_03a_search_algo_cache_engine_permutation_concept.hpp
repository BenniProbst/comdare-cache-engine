#pragma once
// V41.F.6.1 axis_03a_search_algo cache-engine-Pflicht-Concept (2026-05-26)
//
// @topic traversal @achse 03a
//
// **Meta-Modell M2-Schicht** [[meta-driven-concept-hardening-pattern]]:
// Parallel zu SearchAlgoVariant — cache-engine-spezifische Pflicht-API
// analog CacheEnginePermutationStrategy bei Allocator.
//
// Pflicht-API (IMMER):
//   - typename axis_tag    (SA1-SA3 Subaxis-Tag)
//   - typename family_id   (S01-S0N Compile-Time-ID)
//   - static constexpr bool        is_thread_safe()
//   - static constexpr std::size_t max_fanout()     (256 fuer Array256, dynamisch fuer Vector*)
//   - static constexpr std::string_view name() / family_name() / flag_suffix()
//
// Sonderfall-Properties (analog Allocator + Queuing):
//   - supports_simd()           — SIMD-faehig (linear scan + SSE/AVX)?
//   - supports_range_scan()     — Range-Scan (Sorted-Keys)?
//   - is_dense()                — direkt-adressiert (vs sparse)?
//   - has_cache_line_alignment() — 64B-Alignment der Knoten?
//
// Mess-API (Pflicht WENN STATISTICS=ON):
//   - statistics() / snapshot() / reset() / observer()

#include "axis_03a_search_algo_concept.hpp"
#include "../axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"

#include <measurement/measurable_concept.hpp>
#include <concepts/legacy_original_code_strategy_concept.hpp>   // V41.F.6.1.P2.C Habich-Compliance Pflicht-API

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::concepts {

/**
 * @brief SearchAlgoStatistics — Pflicht-Felder fuer Search-Algo Mess-Reihen
 *
 * Pro Search-Variant zaehlbar: Anzahl Lookups, Hit-Rate, Insert-Cost.
 */
struct SearchAlgoStatistics {
    std::uint64_t total_lookup_count   = 0;
    std::uint64_t total_hit_count      = 0;  // erfolgreiche Lookups
    std::uint64_t total_miss_count     = 0;
    std::uint64_t total_insert_count   = 0;
    std::uint64_t total_erase_count    = 0;
    std::uint64_t peak_occupancy       = 0;
    double        avg_density_at_lookup = 0.0;
};

/**
 * @brief CacheEngineSearchAlgoPermutationStrategy — cache-engine-Pflicht-API
 */
template <typename S>
concept CacheEngineSearchAlgoPermutationStrategy =
    ::comdare::cache_engine::traversal::concepts::TraversalComponent<S>
    && requires {
        typename S::axis_tag;
        typename S::family_id;
        { S::is_thread_safe()           } -> std::convertible_to<bool>;
        { S::max_fanout()               } -> std::convertible_to<std::size_t>;
        { S::name()                     } -> std::convertible_to<std::string_view>;
        { S::family_name()              } -> std::convertible_to<std::string_view>;
        { S::flag_suffix()              } -> std::convertible_to<std::string_view>;
        // Sonderfall-Properties Pflicht ([[vendor-sonderfaelle-als-pflicht-property]])
        { S::supports_simd()            } -> std::convertible_to<bool>;
        { S::supports_range_scan()      } -> std::convertible_to<bool>;
        { S::is_dense()                 } -> std::convertible_to<bool>;
        { S::has_cache_line_alignment() } -> std::convertible_to<bool>;
    }
#ifdef COMDARE_CE_ENABLE_STATISTICS
    && requires(S s, S const& sc) {
        { sc.statistics() } noexcept;
        { s.reset()       } noexcept;
    }
    && requires {
        typename S::snapshot_t;
        typename S::observer_t;
    }
    && std::same_as<typename S::observer_t,
                    ::comdare::cache_engine::measurement::MeasurableObserver<typename S::snapshot_t>>
    && requires(S const& sc) {
        { sc.observer() } noexcept -> std::same_as<typename S::observer_t const&>;
    }
#endif
    // V41.F.6.1.P2.C Habich-Compliance: get_compiler + is_original_module (cross-axis via AxisBase Default)
    && ::comdare::cache_engine::concepts::LegacyOriginalCodePflicht<S>
    ;

}  // namespace
