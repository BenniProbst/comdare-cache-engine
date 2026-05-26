#pragma once
// V41.F.6.1.A StdMalloc Concept-Beweis-Klasse (libc malloc Wrapper, 2026-05-25 revidiert)
//
// @topic allocator
// @achse 6
// @family A22 (ptmalloc2 / glibc malloc)
// @subaxis AA2 size_class_schema
// @stand V41.F.6.1.A
//
// **Concept-Beweis:** erste konkrete Klasse die ALLE relevanten Concepts erfuellt:
//   - AllocatorStrategy                  (Pflicht-Standard)
//   - CacheEnginePermutationStrategy     (Pflicht cache-engine-spec)
//   - ZeroingStrategy                    (calloc)
//   - ReallocatingStrategy               (realloc via portable Pattern)
//
// **CMake-Flag COMDARE_CE_ENABLE_STATISTICS:**
//   ON  (Default): statistics() + reset() + stats_-Member + stats-Updates aktiv
//   OFF: alles oben #ifdef'd aus dem Binary - kein Mess-Overhead in Production
//
// **reset() Semantik (User-Klarstellung):** reset() ist die Statistik-Reset-Funktion
// zwischen Mess-Permutationen. NICHT zu verwechseln mit Pool-/Arena-Reset
// (das ist ResettableStrategy Sub-Concept).

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include "../concepts/topic_allocator_concept.hpp"

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>
#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <measurement/measurable_concept.hpp>   // V41.F.6.1 Stufe 3: MeasurableObserver<snapshot_t>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::allocator::axis_06_allocator {

/**
 * @brief StdMalloc - libc malloc Concept-Beweis-Klasse
 * @topic allocator
 * @achse 6
 * @subaxis AA2 size_class_schema_tag
 * @family A22 (ptmalloc2 / glibc)
 */
class StdMalloc : public AllocatorStrategyBase<StdMalloc> {
public:
    // ───────────────────────────────────────────────────────────────────────
    // Standard-AllocatorStrategy Pflicht-Typedefs
    // ───────────────────────────────────────────────────────────────────────
    using value_type = std::byte;
    using size_type  = std::size_t;

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.C Stufe 2 (W6-Pattern): zentralisierte CMake-Flag-Aktivierung
    // ───────────────────────────────────────────────────────────────────────
    static constexpr bool enabled = flags::std_enabled;

    // ───────────────────────────────────────────────────────────────────────
    // CacheEnginePermutationStrategy Pflicht (IMMER): Identifikation
    // ───────────────────────────────────────────────────────────────────────
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::size_class_schema_tag;
    using family_id  = std::integral_constant<int, 22>;   // A22 ptmalloc2/glibc

    [[nodiscard]] static constexpr bool        is_thread_safe()   noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr()     noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment()    noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "std_malloc"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "Standard libc malloc (ptmalloc2 / glibc)"; }
    // V41.F.6.1.G CacheEngineBuilder CLI-Flag-Suffix (Doku §15.10)
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "STD"; }

    // V41.F.6.1 Vendor-Sonderfall-Properties (Pflicht, [[vendor-sonderfaelle-als-pflicht-property]])
    [[nodiscard]] static constexpr bool has_native_aligned_alloc()    noexcept { return true; }   // portable_aligned_alloc via posix_memalign/_aligned_malloc
    [[nodiscard]] static constexpr bool requires_explicit_init()      noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_numa_node_hint()     noexcept { return false; }
    [[nodiscard]] static constexpr bool is_lock_free()                noexcept { return false; }  // ptmalloc2 nutzt Mutex pro Arena
    [[nodiscard]] static constexpr bool supports_thread_local_cache() noexcept { return false; }
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept { return false; }

    // ───────────────────────────────────────────────────────────────────────
    // AllocatorStrategy: Identitaet (operator==)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] bool operator==(StdMalloc const&) const noexcept {
        // libc malloc ist global — alle Instanzen sind aequivalent
        return true;
    }

    // ───────────────────────────────────────────────────────────────────────
    // AllocatorStrategy Pflicht: Runtime-API (PMR-Naming, sized+aligned)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += aligned_bytes;
            stats_.total_bytes_in_use    += aligned_bytes;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
        ::comdare::cache_engine::allocator::portable_aligned_free(p);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use) {
            stats_.total_bytes_in_use -= aligned_bytes;
        } else {
            stats_.total_bytes_in_use = 0;
        }
        observer_.notify(stats_);
#else
        (void)bytes;
        (void)alignment;
#endif
    }

    // ───────────────────────────────────────────────────────────────────────
    // CacheEnginePermutationStrategy Pflicht (NUR WENN STATISTICS=ON):
    //   - statistics() liefert Mess-Daten (cache-engine-spez. API)
    //   - snapshot() liefert Mess-Daten (Topic-uebergreifende API, identisch zu statistics)
    //   - reset() setzt Statistik zwischen Mess-Permutationen zurueck
    //     (User-Klarstellung 2026-05-25: NICHT Pool-Reset, sondern Statistik-Reset)
    //   - observer() liefert MeasurableObserver<snapshot_t> Pflicht-Alias (V41.F.6.1 Stufe 3)
    // ───────────────────────────────────────────────────────────────────────
#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::AllocationStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;

    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot()   const noexcept { return stats_; }

    void reset() noexcept {
        // Statistik-Reset (NICHT Allokationen freigeben!)
        stats_ = {};
        observer_.notify(stats_);
    }

    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ZeroingStrategy (calloc-style)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] void* zero_allocate(std::size_t n, std::size_t size) {
        void* p = std::calloc(n, size);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t bytes = n * size;
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += bytes;
            stats_.total_bytes_in_use    += bytes;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ReallocatingStrategy (portable Pattern: alloc-new + memcpy + free-old)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes,
                                   std::size_t alignment) {
        void* np = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, new_bytes);
        if (np == nullptr) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.failure_count;
            observer_.notify(stats_);
#endif
            return nullptr;
        }
        if (p != nullptr) {
            std::size_t copy_bytes = (old_bytes < new_bytes) ? old_bytes : new_bytes;
            std::memcpy(np, p, copy_bytes);
            ::comdare::cache_engine::allocator::portable_aligned_free(p);
#ifdef COMDARE_CE_ENABLE_STATISTICS
            if (old_bytes <= stats_.total_bytes_in_use)
                stats_.total_bytes_in_use -= old_bytes;
            ++stats_.deallocation_count;
#endif
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_new = ((new_bytes + alignment - 1) / alignment) * alignment;
        stats_.total_bytes_in_use    += aligned_new;
        stats_.total_bytes_allocated += aligned_new;
        ++stats_.allocation_count;
        observer_.notify(stats_);
#endif
        return np;
    }

private:
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace comdare::cache_engine::allocator::axis_06_allocator

// ───────────────────────────────────────────────────────────────────────────
// Compile-Time-Beweise (Concept-Konformanz):
// ───────────────────────────────────────────────────────────────────────────
namespace comdare::cache_engine::allocator::axis_06_allocator {
    static_assert(concepts::AllocatorStrategy<StdMalloc>,
        "Pflicht: StdMalloc muss AllocatorStrategy erfuellen (Standard-PMR-API)");
    static_assert(concepts::CacheEnginePermutationStrategy<StdMalloc>,
        "Pflicht: StdMalloc muss CacheEnginePermutationStrategy erfuellen "
        "(cache-engine-spec, mit statistics()+reset() wenn STATISTICS=ON)");
    static_assert(concepts::ZeroingStrategy<StdMalloc>,
        "Optional: StdMalloc bietet zero_allocate (calloc)");
    static_assert(concepts::ReallocatingStrategy<StdMalloc>,
        "Optional: StdMalloc bietet reallocate (portable Pattern)");
}  // namespace
