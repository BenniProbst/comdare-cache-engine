#pragma once
// V41.F.6.1.A cache-engine-spezifisches Pflicht-Concept (2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// **Parallel zu AllocatorStrategy:** cache-engine-spezifische Pflicht-API die
// jede Allokator-Variante anbieten muss, damit sie als Permutations-Variante
// in der cache-engine-Pipeline registrierbar ist.
//
// Trennung zum Standard-Concept (AllocatorStrategy):
//   - AllocatorStrategy = strikt nach Standard (ISO C/C++ + PMR)
//   - CacheEnginePermutationStrategy = cache-engine-Permutationen-Identitaet
//
// Eine Klasse erfuellt typischerweise BEIDE Concepts.

#include "axis_06_allocator_concept.hpp"
#include "../axis_06_allocator_subaxes_aa1_to_aa7.hpp"

#include <concepts>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::allocator::axis_06_allocator::concepts {

/**
 * @brief AllocationStatistics - Pflicht-Felder fuer Permutations-Mess-Reihen
 * @topic allocator
 * @achse 6
 *
 * Wird vom CacheEngineBuilder fuer Welch's t-Test ausgelesen. Format ist
 * cache-engine-spezifisch (Vendor-statistics sind NICHT standardisierbar —
 * siehe Web-Recherche W1).
 */
struct AllocationStatistics {
    std::uint64_t total_bytes_allocated  = 0;
    std::uint64_t total_bytes_in_use     = 0;
    std::uint64_t allocation_count       = 0;
    std::uint64_t deallocation_count     = 0;
    std::uint64_t failure_count          = 0;
    double        external_fragmentation = 0.0;
    double        internal_fragmentation = 0.0;
};

/**
 * @brief CacheEnginePermutationStrategy - cache-engine-Pflicht-API (parallel zu AllocatorStrategy)
 * @topic allocator
 * @achse 6
 *
 * **Klassifikation (Pflicht):**
 *   - typename axis_tag       (AA1-AA7 Sub-Achsen-Tag)
 *   - typename family_id      (A01-A23 Compile-Time-ID)
 *
 * **Compile-Time-Eigenschaften (Pflicht):**
 *   - static constexpr bool is_thread_safe()
 *   - static constexpr bool supports_pmr()
 *   - static constexpr std::size_t max_alignment()
 *
 * **Identifikation (Pflicht):**
 *   - static constexpr std::string_view name()
 *   - static constexpr std::string_view family_name()
 *
 * **Mess-API (Pflicht):**
 *   - statistics()       -> AllocationStatistics noexcept
 *
 * Beispiel-Klasse erfuellt typischerweise:
 *   static_assert(AllocatorStrategy<MyAllocator>);                  // Standard
 *   static_assert(CacheEnginePermutationStrategy<MyAllocator>);     // cache-engine-spec
 */
template <typename A>
concept CacheEnginePermutationStrategy =
    ::comdare::cache_engine::allocator::concepts::AllocatorComponent<A>
    && requires {
        typename A::axis_tag;
        typename A::family_id;
        { A::is_thread_safe() } -> std::convertible_to<bool>;
        { A::supports_pmr()   } -> std::convertible_to<bool>;
        { A::max_alignment()  } -> std::convertible_to<std::size_t>;
        { A::name()           } -> std::convertible_to<std::string_view>;
        { A::family_name()    } -> std::convertible_to<std::string_view>;
    }
    && requires(A const& a) {
        { a.statistics() } noexcept;
    };

}  // namespace comdare::cache_engine::allocator::axis_06_allocator::concepts
