#pragma once
// V41.F.6.1 Batch 4 NUMAllocAllocator A09 (2026-05-26)
//
// @topic allocator
// @achse 6
// @family A09 (NUMAlloc — Linden/Liu/Williams ICDCS 2018)
// @subaxis AA5 allocation_policy (NUMA-Origin-Awareness)
//
// **SONDERFALL NUMAlloc:** API verlangt expliziten NUMA-Node-Parameter.
// Wir uebergeben node=-1 (kernel-Default = aktueller Node der ausfuehrenden CPU).
// Auf Single-Node-Systemen verhaelt es sich identisch zu normaler Allokation.
//
// Optionale Konstruktor-Variante: `NUMAllocAllocator(int node)` fuer expliziten
// Node-Pin (z.B. Hot-Pages auf Node 0, Cold-Pages auf Node 1). Heute Default = -1.

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include "../concepts/topic_allocator_concept.hpp"

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>
#include "vendor_includes/numalloc_include.hpp"

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::allocator::axis_06_allocator {

class NUMAllocAllocator : public AllocatorStrategyBase<NUMAllocAllocator> {
public:
    static constexpr bool enabled = flags::numalloc_enabled;

    /// Sonderfall: NUMA-Node-Default (-1 = kernel-Default = aktueller Node)
    static constexpr int kDefaultNumaNode = -1;

    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::allocation_policy_tag;
    using family_id  = std::integral_constant<int, 9>;

    [[nodiscard]] static constexpr bool        is_thread_safe()  noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr()    noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment()   noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) { return "numalloc"; }
        else                   { return "numalloc(real=std)"; }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "NUMAlloc NUMA-aware (Linden/Liu/Williams ICDCS 2018)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "NUMALLOC"; }

    NUMAllocAllocator() noexcept : numa_node_(kDefaultNumaNode) {}
    explicit NUMAllocAllocator(int node) noexcept : numa_node_(node) {}

    [[nodiscard]] bool operator==(NUMAllocAllocator const& other) const noexcept {
        return numa_node_ == other.numa_node_;
    }

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p;
        if constexpr (enabled) { p = ::numalloc_alloc(bytes, alignment, numa_node_); }
        else                   { p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes); }
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
        if constexpr (enabled) { ::numalloc_free(p, bytes); }
        else                   { ::comdare::cache_engine::allocator::portable_aligned_free(p); }
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
        if constexpr (enabled) { p = ::numalloc_calloc(n, size, numa_node_); }
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
            np = ::numalloc_realloc(p, new_bytes, numa_node_);
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

    /// Pin auf einen anderen NUMA-Node nach Konstruktion (z.B. Hot/Cold-Split)
    [[nodiscard]] int numa_node() const noexcept { return numa_node_; }
    void set_numa_node(int node) noexcept { numa_node_ = node; }

private:
    int numa_node_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::allocator::axis_06_allocator {
    static_assert(concepts::AllocatorStrategy<NUMAllocAllocator>);
    static_assert(concepts::CacheEnginePermutationStrategy<NUMAllocAllocator>);
    static_assert(concepts::ZeroingStrategy<NUMAllocAllocator>);
    static_assert(concepts::ReallocatingStrategy<NUMAllocAllocator>);
}
