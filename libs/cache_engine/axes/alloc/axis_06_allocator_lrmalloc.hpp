#pragma once
// V41.F.6.1 Batch 5 LRMallocAllocator A11 (2026-05-26)
//
// @topic allocator @achse 6 @family A11 (LRMalloc — Leite/Rocha, VECPAR 2018)
// @subaxis AA4 synchronization (Lock-Free + Hazard-Pointer)
//
// Lock-Free Allocator mit Hazard-Pointers fuer safe Memory-Reclamation.
// Verwendet per-thread Reservations + lock-free Global Free-List.

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include <topics/allocator/concepts/topic_allocator_concept.hpp>

#include <axes/alloc/axis_06_allocator_flags.hpp>
#include "vendor_includes/lrmalloc_include.hpp"

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

#if defined(COMDARE_A06_IS_ORIGINAL_CODEGEN)
// V41.F.6.1.P2.D Batch 2 lrmalloc Paper-Legacy-Code Mixin
#include "concepts/axis_06_allocator_original_code_mixin.hpp"
#include <topics/allocator/axis_06_allocator/legacy_code/paper_a11_lrmalloc_is_original.hpp>
#endif

namespace comdare::cache_engine::alloc {

class LRMallocAllocator : public AllocatorStrategyBase<LRMallocAllocator>
#if defined(COMDARE_A06_IS_ORIGINAL_CODEGEN)
    ,
                          public generated::a11_lrmalloc::OriginalCodeMixin // V41.F.6.1.P2.D Batch 2
#endif
{
public:
#if defined(COMDARE_A06_IS_ORIGINAL_CODEGEN)
    using generated::a11_lrmalloc::OriginalCodeMixin::get_compiler;
    using generated::a11_lrmalloc::OriginalCodeMixin::is_original_allocate;
    using generated::a11_lrmalloc::OriginalCodeMixin::is_original_deallocate;
    using generated::a11_lrmalloc::OriginalCodeMixin::is_original_module;
#endif

    static constexpr bool enabled = flags::lrmalloc_enabled;

    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::synchronization_tag;
    using family_id  = std::integral_constant<int, 11>;

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr() noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment() noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) {
            return "lrmalloc";
        } else {
            return "lrmalloc(real=std)";
        }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "LRMalloc Lock-Free + Hazard-Pointer (Leite/Rocha, VECPAR 2018)";
    }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view               algo_version = "v1";
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "LRMALLOC"; }

    // V41.F.6.1 Vendor-Sonderfall-Properties (Pflicht)
    [[nodiscard]] static constexpr bool has_native_aligned_alloc() noexcept { return true; }
    [[nodiscard]] static constexpr bool requires_explicit_init() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_numa_node_hint() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_thread_local_cache() noexcept {
        return true;
    } // per-thread reservations
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::LockFree;
    } // namesgebend (Lock-Free Reclamation)
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept { return false; }

    [[nodiscard]] bool operator==(LRMallocAllocator const&) const noexcept { return true; }

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p;
        if constexpr (enabled) {
            p = ::lrmalloc_alloc(bytes, alignment);
        } else {
            p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += aligned_bytes;
            stats_.total_bytes_in_use += aligned_bytes;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
        if constexpr (enabled) {
            ::lrmalloc_free(p);
        } else {
            ::comdare::cache_engine::allocator::portable_aligned_free(p);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use)
            stats_.total_bytes_in_use -= aligned_bytes;
        else
            stats_.total_bytes_in_use = 0;
        observer_.notify(stats_);
#else
        (void)bytes;
        (void)alignment;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::AllocationStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

    [[nodiscard]] void* zero_allocate(std::size_t n, std::size_t size) {
        std::size_t bytes = n * size;
        void*       p;
        if constexpr (enabled) {
            p = ::lrmalloc_calloc(n, size);
        } else {
            p = std::calloc(n, size);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += bytes;
            stats_.total_bytes_in_use += bytes;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes, std::size_t alignment) {
        void* np;
        if constexpr (enabled) {
            np = ::lrmalloc_realloc(p, new_bytes);
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
        if (np == nullptr) {
            ++stats_.failure_count;
            observer_.notify(stats_);
            return nullptr;
        }
        if (p != nullptr) {
            if (old_bytes <= stats_.total_bytes_in_use) stats_.total_bytes_in_use -= old_bytes;
            ++stats_.deallocation_count;
        }
        std::size_t aligned_new = ((new_bytes + alignment - 1) / alignment) * alignment;
        stats_.total_bytes_in_use += aligned_new;
        stats_.total_bytes_allocated += aligned_new;
        ++stats_.allocation_count;
        observer_.notify(stats_);
#endif
        return np;
    }

private:
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
    observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::alloc

namespace comdare::cache_engine::alloc {
static_assert(concepts::AllocatorStrategy<LRMallocAllocator>);
static_assert(concepts::CacheEnginePermutationStrategy<LRMallocAllocator>);
static_assert(concepts::ZeroingStrategy<LRMallocAllocator>);
static_assert(concepts::ReallocatingStrategy<LRMallocAllocator>);
} // namespace comdare::cache_engine::alloc
