#pragma once
// V41.F.6.1 Batch 4 ScallocAllocator A08 (2026-05-26)
//
// @topic allocator
// @achse 6
// @family A08 (Scalloc Spans — Aigner/Iurca/Wimmer PPoPP 2015)
// @subaxis AA1 freelist_topology (Spans als Variable-Size-Free-List)
//
// **SONDERFALL Scalloc:** Standard-API hat KEINE native aligned_alloc.
// Wir verwenden Vendor-API nur fuer power-of-2 alignments <= alignof(max_align_t),
// fuer groessere alignments fallback auf portable_aligned_alloc (auch bei enabled=true).
//
// Begruendung: Spans sind intern fix-size, manuelle Overallocation+Justierung waere
// nicht kompatibel mit scalloc_free (das wuerde den justierten Pointer nicht erkennen).

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include "../concepts/topic_allocator_concept.hpp"

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>
#include "vendor_includes/scalloc_include.hpp"

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::allocator::axis_06_allocator {

class ScallocAllocator : public AllocatorStrategyBase<ScallocAllocator> {
public:
    static constexpr bool enabled = flags::scalloc_enabled;

    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::freelist_topology_tag;
    using family_id  = std::integral_constant<int, 8>;

    [[nodiscard]] static constexpr bool        is_thread_safe()  noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr()    noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment()   noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) { return "scalloc"; }
        else                   { return "scalloc(real=std)"; }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Scalloc Spans (Aigner/Iurca/Wimmer PPoPP 2015)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SCALLOC"; }

    // V41.F.6.1 Vendor-Sonderfall-Properties (Pflicht, [[vendor-sonderfaelle-als-pflicht-property]])
    [[nodiscard]] static constexpr bool has_native_aligned_alloc()    noexcept { return false; }  // SONDERFALL: keine native aligned_alloc API
    [[nodiscard]] static constexpr bool requires_explicit_init()      noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_numa_node_hint()     noexcept { return false; }
    [[nodiscard]] static constexpr bool is_lock_free()                noexcept { return true; }   // Spans-Free-List ist lock-free
    [[nodiscard]] static constexpr bool supports_thread_local_cache() noexcept { return true; }   // per-thread spans
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_wait_free()                noexcept { return false; }

    [[nodiscard]] bool operator==(ScallocAllocator const&) const noexcept { return true; }

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p;
        // Sonderfall: scalloc kennt keine native aligned-Allocation. Bei alignment
        // <= max_align_t verlassen wir uns auf Vendor's Default-Alignment (intern
        // 16-byte spans). Sonst Fallback fuer korrekte alignment-Garantie.
        if constexpr (enabled) {
            if (alignment <= alignof(std::max_align_t)) {
                p = ::scalloc_malloc(bytes);
            } else {
                // Echt grosses alignment -> portable Pfad fuer Korrektheit
                p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes);
            }
        } else {
            p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes);
        }
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
        // Symmetrie zum allocate-Pfad: gleiche alignment-Logik
        if constexpr (enabled) {
            if (alignment <= alignof(std::max_align_t)) {
                ::scalloc_free(p);
            } else {
                ::comdare::cache_engine::allocator::portable_aligned_free(p);
            }
        } else {
            ::comdare::cache_engine::allocator::portable_aligned_free(p);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use) stats_.total_bytes_in_use -= aligned_bytes;
        else stats_.total_bytes_in_use = 0;
        observer_.notify(stats_);
#else
        (void)bytes; (void)alignment;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::AllocationStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;

    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot()   const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; observer_.notify(stats_); }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

    [[nodiscard]] void* zero_allocate(std::size_t n, std::size_t size) {
        std::size_t bytes = n * size;
        void* p;
        if constexpr (enabled) { p = ::scalloc_calloc(n, size); }
        else                   { p = std::calloc(n, size); }
#ifdef COMDARE_CE_ENABLE_STATISTICS
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

    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes,
                                   std::size_t alignment) {
        void* np;
        if constexpr (enabled) {
            np = ::scalloc_realloc(p, new_bytes);
            (void)alignment;
        } else {
            np = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, new_bytes);
            if (np != nullptr && p != nullptr) {
                std::size_t copy_bytes = (old_bytes < new_bytes) ? old_bytes : new_bytes;
                std::memcpy(np, p, copy_bytes);
                ::comdare::cache_engine::allocator::portable_aligned_free(p);
            }
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (np == nullptr) { ++stats_.failure_count; observer_.notify(stats_); return nullptr; }
        if (p != nullptr) {
            if (old_bytes <= stats_.total_bytes_in_use) stats_.total_bytes_in_use -= old_bytes;
            ++stats_.deallocation_count;
        }
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

}  // namespace

namespace comdare::cache_engine::allocator::axis_06_allocator {
    static_assert(concepts::AllocatorStrategy<ScallocAllocator>);
    static_assert(concepts::CacheEnginePermutationStrategy<ScallocAllocator>);
    static_assert(concepts::ZeroingStrategy<ScallocAllocator>);
    static_assert(concepts::ReallocatingStrategy<ScallocAllocator>);
}
