#pragma once
// V41.F.6.1.C MimallocAllocator A04 (2026-05-25)
//
// @topic allocator
// @achse 6
// @family A04 (Mimalloc — Leijen/Zorn/de Moura, MSR-TR-2019-18 / APLAS 2019)
// @subaxis AA1 freelist_topology (Free-List-Sharding)
// @stand V41.F.6.1.C Batch 1 Vendor-Vollausbau (Direktive [[achsen-vendor-vollausbau]])
//
// **Vendor-Source:** ext/A04-mimalloc/include/mimalloc.h
// **Build-Detection:** #ifdef COMDARE_HAVE_MIMALLOC (CMake try_compile setze define)
// **Fallback:** portable_aligned_alloc (markiert real=std im Output)
//
// Erfuellt:
//   - AllocatorStrategy (Pflicht-Standard, PMR-Naming)
//   - CacheEnginePermutationStrategy (Pflicht cache-engine-spec)
//   - ZeroingStrategy            (mi_zalloc_aligned)
//   - ReallocatingStrategy       (mi_realloc_aligned)
//   - IntrospectableStrategy     (mi_usable_size)
//   - ReclaimableStrategy        (mi_collect)

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include "concepts/axis_06_allocator_introspectable_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reclaimable_strategy_concept.hpp"
#include <topics/allocator/concepts/topic_allocator_concept.hpp>

#include <axes/alloc/axis_06_allocator_flags.hpp>
#include "vendor_includes/mimalloc_include.hpp" // V41.F.6.1.C Stufe 2: Shim mit Forward-Stubs

// V41.F.6.1.P2.B mimalloc Pilot — Paper-Legacy-Code Mixin (auto-generated via Pre-Build-Tool)
#include "concepts/axis_06_allocator_original_code_mixin.hpp"
#include <topics/allocator/axis_06_allocator/legacy_code/paper_a04_mimalloc_is_original.hpp>

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <measurement/measurable_concept.hpp> // V41.F.6.1 Stufe 3: MeasurableObserver<snapshot_t>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::alloc {

/**
 * @brief MimallocAllocator - Wrapper auf microsoft/mimalloc (A04)
 *
 * Free-List-Sharding (Leijen MSR-TR-2019-18): 3 Free-Lists pro Page
 * (free / local_free / thread_free), temporal cadence, deferred_free hook.
 *
 * V41.F.6.1.C Stufe 2 (W6-Pattern): KEIN #ifdef COMDARE_HAVE_MIMALLOC mehr.
 * Aktivierung via `enabled = flags::mimalloc_enabled` (zentraler CMake-Flag).
 * Vendor-Calls direkt — bei OFF werden Forward-Stubs aus dem Shim verwendet
 * (NIEMALS aufgerufen wegen if constexpr (false) Discarded Statement).
 */
class MimallocAllocator
    : public AllocatorStrategyBase<MimallocAllocator>,
      public generated::a04_mimalloc::OriginalCodeMixin { // V41.F.6.1.P2.B Paper-Mixin (Habich-Compliance)
public:
    // V41.F.6.1.P2.B/P2.C Diamond-Disambiguation:
    // AllocatorStrategyBase erbt von AxisBase (get_compiler() = "original",
    // is_original_module() = false Defaults). OriginalCodeMixin (via OriginalCodeMixinBase)
    // ueberschreibt beides — Mixin-Pfad wins fuer Habich-Compliance.
    using generated::a04_mimalloc::OriginalCodeMixin::get_compiler;
    using generated::a04_mimalloc::OriginalCodeMixin::is_original_allocate;
    using generated::a04_mimalloc::OriginalCodeMixin::is_original_deallocate;
    using generated::a04_mimalloc::OriginalCodeMixin::is_original_module;

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.C Stufe 2 (W6-Pattern): zentralisierte CMake-Flag-Aktivierung
    // ───────────────────────────────────────────────────────────────────────
    static constexpr bool enabled = flags::mimalloc_enabled;

    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::freelist_topology_tag;
    using family_id  = std::integral_constant<int, 4>;

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr() noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment() noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) {
            return "mimalloc";
        } else {
            return "mimalloc(real=std)";
        }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Mimalloc Free-List-Sharding (Leijen/Zorn/de Moura MSR-TR-2019-18 APLAS 2019)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "MIMALLOC"; }

    // V41.F.6.1 Vendor-Sonderfall-Properties (Pflicht, [[vendor-sonderfaelle-als-pflicht-property]])
    [[nodiscard]] static constexpr bool has_native_aligned_alloc() noexcept { return true; } // mi_malloc_aligned
    [[nodiscard]] static constexpr bool requires_explicit_init() noexcept { return false; }  // Self-init lazy
    [[nodiscard]] static constexpr bool supports_numa_node_hint() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_thread_local_cache() noexcept {
        return true;
    } // 3-stage Free-List: free/local_free/thread_free
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept { return false; }

    [[nodiscard]] bool operator==(MimallocAllocator const&) const noexcept { return true; }

    // ───────────────────────────────────────────────────────────────────────
    // AllocatorStrategy Pflicht (mi_malloc_aligned / mi_free direkter Vendor-Call)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p;
        if constexpr (enabled) {
            p = ::mi_malloc_aligned(bytes, alignment);
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
            // mi_free verarbeitet aligned-Allocations korrekt (kein separates mi_free_aligned)
            ::mi_free(p);
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

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ZeroingStrategy (mi_zalloc / mi_calloc Vendor-direkt)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] void* zero_allocate(std::size_t n, std::size_t size) {
        std::size_t bytes = n * size;
        void*       p;
        if constexpr (enabled) {
            p = ::mi_calloc(n, size);
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

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ReallocatingStrategy (mi_realloc_aligned Vendor-direkt)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes, std::size_t alignment) {
        void* np;
        if constexpr (enabled) {
            np = ::mi_realloc_aligned(p, new_bytes, alignment);
        } else {
            // Portable Fallback: alloc-new + memcpy + free-old
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

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: IntrospectableStrategy (mi_usable_size)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] std::size_t usable_size(void* p) const noexcept {
        if constexpr (enabled) {
            return ::mi_usable_size(p);
        } else {
            (void)p;
            return 0;
        }
    }

    // ───────────────────────────────────────────────────────────────────────
    // Sub-Concept: ReclaimableStrategy (mi_collect)
    // ───────────────────────────────────────────────────────────────────────
    void collect(bool force) noexcept {
        if constexpr (enabled) {
            ::mi_collect(force);
        } else {
            (void)force; /* no-op fuer std-fallback */
        }
    }

private:
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
    observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::alloc

namespace comdare::cache_engine::alloc {
static_assert(concepts::AllocatorStrategy<MimallocAllocator>,
              "Pflicht: MimallocAllocator muss AllocatorStrategy erfuellen");
static_assert(concepts::CacheEnginePermutationStrategy<MimallocAllocator>,
              "Pflicht: MimallocAllocator muss CacheEnginePermutationStrategy erfuellen");
static_assert(concepts::ZeroingStrategy<MimallocAllocator>);
static_assert(concepts::ReallocatingStrategy<MimallocAllocator>);
static_assert(concepts::IntrospectableStrategy<MimallocAllocator>);
static_assert(concepts::ReclaimableStrategy<MimallocAllocator>);
} // namespace comdare::cache_engine::alloc
