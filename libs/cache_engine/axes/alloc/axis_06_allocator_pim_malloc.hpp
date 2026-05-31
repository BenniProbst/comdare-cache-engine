#pragma once
// V41.F.6.1 Batch 6 PIMMallocAllocator A16 (2026-05-26)
//
// @topic allocator @achse 6 @family A16 (PIM-Malloc — VIA-Research, HPCA 2026 / arXiv:2505.13002)
// @subaxis AA5 allocation_policy (Processing-In-Memory Hardware-Awareness)
//
// **SONDERFALL PIM-Malloc:** Verlangt PIM-Hardware (UPMEM DPUs, Samsung HBM-PIM,
// SK Hynix AiM). API hat DPU-ID-Parameter analog NUMA-Node bei NUMAlloc.
// On non-PIM-systems: Fallback auf portable_aligned_alloc (Host-Memory).
//
// SONDERFALL-PROPERTY (User-Direktive 2026-05-26, Concept-Erweiterung):
//   requires_specialized_hardware() = true
//   CacheEngineBuilder kann pro Plattform Allocator-Subsets bilden — auf einem
//   reinem CPU-System wuerde dieser Allocator nicht permutiert werden.

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include <topics/allocator/concepts/topic_allocator_concept.hpp>

#include <axes/alloc/axis_06_allocator_flags.hpp>
#include "vendor_includes/pim_malloc_include.hpp"

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::alloc {

class PIMMallocAllocator : public AllocatorStrategyBase<PIMMallocAllocator> {
public:
    static constexpr bool enabled = flags::pim_malloc_enabled;

    /// SONDERFALL: PIM-DPU-Default (-1 = primaerer DPU)
    static constexpr int kDefaultDpuId = -1;

    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::allocation_policy_tag;
    using family_id  = std::integral_constant<int, 16>;

    [[nodiscard]] static constexpr bool        is_thread_safe()  noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr()    noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment()   noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) { return "pim_malloc"; }
        else                   { return "pim_malloc(real=std)"; }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "PIM-Malloc Processing-In-Memory (VIA-Research, HPCA 2026 / arXiv:2505.13002)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PIM_MALLOC"; }

    // V41.F.6.1 Vendor-Sonderfall-Properties (Pflicht)
    [[nodiscard]] static constexpr bool has_native_aligned_alloc()    noexcept { return true; }   // pim_alloc(size, alignment, dpu)
    [[nodiscard]] static constexpr bool requires_explicit_init()      noexcept { return false; }  // pim_detect_hardware lazy
    [[nodiscard]] static constexpr bool supports_numa_node_hint()     noexcept { return false; }  // PIM != NUMA
    [[nodiscard]] static constexpr bool supports_thread_local_cache() noexcept { return false; }  // single-DPU per allocation
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept { return concepts::ProgressGuarantee::Blocking; }
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept { return true; } // SONDERFALL: PIM-Hardware-Pflicht

    PIMMallocAllocator() noexcept : dpu_id_(kDefaultDpuId) {}
    explicit PIMMallocAllocator(int dpu_id) noexcept : dpu_id_(dpu_id) {}

    [[nodiscard]] bool operator==(PIMMallocAllocator const& other) const noexcept {
        return dpu_id_ == other.dpu_id_;
    }

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p;
        if constexpr (enabled) { p = ::pim_alloc(bytes, alignment, dpu_id_); }
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
        if constexpr (enabled) { ::pim_free(p, dpu_id_); }
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
        if constexpr (enabled) { p = ::pim_calloc(n, size, dpu_id_); }
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
            np = ::pim_realloc(p, new_bytes, dpu_id_);
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

    /// PIM-spezifisch: DPU-ID Setter/Getter
    [[nodiscard]] int dpu_id() const noexcept { return dpu_id_; }
    void set_dpu_id(int dpu) noexcept { dpu_id_ = dpu; }

private:
    int dpu_id_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::alloc {
    static_assert(concepts::AllocatorStrategy<PIMMallocAllocator>);
    static_assert(concepts::CacheEnginePermutationStrategy<PIMMallocAllocator>);
    static_assert(concepts::ZeroingStrategy<PIMMallocAllocator>);
    static_assert(concepts::ReallocatingStrategy<PIMMallocAllocator>);
}
