#pragma once
// V41.F.6.1.A Achsen-Concept Allocator Axis 6 (2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A Pilot
//
// Achsen-Concept verlangt die einheitlichen Funktionen, die jede Allokator-
// Implementierung anbieten muss, damit sie als Permutations-Variante in der
// CacheEngine-Pipeline + Mess-Reihen verwendet werden kann.
//
// Erweitert das Topic-Concept (AllocatorComponent) um konkrete API-Vertraege.

#include "../../concepts/topic_allocator_concept.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::allocator::axis_06_allocator::concepts {

/**
 * @brief AllocationStatistics - Pflicht-Felder fuer alle Allokator-Familien
 * @achse 6
 *
 * Wird vom CacheEngineBuilder fuer Mess-Reihen + Welch's t-Test ausgelesen.
 * Reset via reset() zwischen Mess-Permutationen.
 */
struct AllocationStatistics {
    std::uint64_t total_bytes_allocated  = 0;
    std::uint64_t total_bytes_in_use     = 0;
    std::uint64_t allocation_count       = 0;
    std::uint64_t deallocation_count     = 0;
    std::uint64_t failure_count          = 0;          // alloc returned nullptr / threw
    double        external_fragmentation = 0.0;        // [0..1]
    double        internal_fragmentation = 0.0;        // [0..1]
};

/**
 * @brief AllocatorStrategy - Achsen-Concept fuer Achse 6 (Allocator)
 * @topic allocator
 * @achse 6
 *
 * Jede Allokator-Familie (A01-A23) muss dieses Concept erfuellen, damit sie
 * als Permutations-Variante registriert werden kann.
 *
 * Pflicht-API (3 Gruppen):
 *
 * **(1) Runtime-Allocation-API** (Hot-Path, KEIN virtual, statische Methoden):
 *   - raw_allocate(bytes, align)        -> void*
 *   - raw_deallocate(ptr, bytes, align) -> void
 *   - statistics()                      -> AllocationStatistics noexcept
 *   - reset()                           -> void noexcept  (zwischen Mess-Runden)
 *
 * **(2) Compile-Time-Eigenschaften** (fuer Cross-Constraint-Filter):
 *   - typename axis_tag                  (AA1-AA7 freelist/size_class/...)
 *   - typename family_id                 (A01-A23 Compile-Time-ID)
 *   - static constexpr bool is_thread_safe()
 *   - static constexpr bool supports_pmr()
 *   - static constexpr std::size_t max_alignment()
 *
 * **(3) Identifikation** (fuer Logging/CSV/Welch-Output):
 *   - static constexpr std::string_view name()
 *   - static constexpr std::string_view family_name()
 *
 * Beispiel-Klasse die dieses Concept erfuellt:
 *
 *   namespace comdare::cache_engine::allocator::axis_06_allocator {
 *       struct StdMalloc {
 *           using topic_tag  = concepts::AllocatorTopicTag;
 *           using axis_tag   = subaxes::size_class_schema_tag;  // AA2
 *           using family_id  = std::integral_constant<int, 22>;  // A22
 *
 *           static constexpr bool is_thread_safe()         { return true; }
 *           static constexpr bool supports_pmr()           { return true; }
 *           static constexpr std::size_t max_alignment()   { return alignof(std::max_align_t); }
 *           static constexpr std::string_view name()        { return "std_malloc"; }
 *           static constexpr std::string_view family_name() { return "Standard libc malloc"; }
 *
 *           void* raw_allocate(std::size_t n, std::size_t a)        { return std::aligned_alloc(a,n); }
 *           void  raw_deallocate(void* p, std::size_t, std::size_t) { std::free(p); }
 *           concepts::AllocationStatistics statistics() const noexcept;
 *           void reset() noexcept;
 *       };
 *   }
 */
template <typename T>
concept AllocatorStrategy =
    ::comdare::cache_engine::allocator::concepts::AllocatorComponent<T>
    && requires(T t, std::size_t bytes, std::size_t align, void* p) {
        // (1) Runtime-Allocation-API
        { t.raw_allocate(bytes, align) }            -> std::same_as<void*>;
        t.raw_deallocate(p, bytes, align);          // void return
        { t.statistics() }                          noexcept;
        { t.reset() }                               noexcept;
    }
    && requires {
        // (2) Compile-Time-Eigenschaften
        typename T::axis_tag;
        typename T::family_id;
        { T::is_thread_safe() } -> std::convertible_to<bool>;
        { T::supports_pmr()    } -> std::convertible_to<bool>;
        { T::max_alignment()   } -> std::convertible_to<std::size_t>;
    }
    && requires {
        // (3) Identifikation
        { T::name()         } -> std::convertible_to<std::string_view>;
        { T::family_name()  } -> std::convertible_to<std::string_view>;
    };

}  // namespace comdare::cache_engine::allocator::axis_06_allocator::concepts
