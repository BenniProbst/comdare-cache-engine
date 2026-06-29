#pragma once
// V41.F.6.1 R5.B axis_06_allocator PoolResourceAllocator (2026-05-29)
//
// @topic allocator
// @achse 6
// @family A22 (std::pmr — Halpern N3916, C++17 pool variant)
// @subaxis AA2 size_class_schema
//
// **Algorithmus:** besitzt einen EIGENEN std::pmr::unsynchronized_pool_resource (size-class-Pools
// mit Block-Wiederverwendung, upstream = new_delete). Im Gegensatz zu StdMalloc (System-malloc) und
// zum default-konstruierten PmrResourceAllocator (new_delete = System) ist dieser Wrapper
// VERHALTENS-DISTINKT von System-malloc OHNE externes Linking: viele kleine, gleichgrosse
// Allokationen werden aus vorallokierten Chunks bedient + bei deallocate in die Free-List des
// Size-Class-Pools zurueckgegeben → andere Latenz-Charakteristik als der globale Allocator.
//
// **Zweck (R5.B):** macht die Allocator-Achse BEHAVIORAL operativ — der erste axis_06-Wrapper, der
// sich ohne Dependency-Linking real von System-malloc unterscheidet. Damit ist eine NICHT-hohle
// 2-Achsen-F15-Messung (search_algo × allocator) moeglich (Doku 22 §3.1, [[std-map-unified-interface]]).
//
// **Provenienz / Lizenz:** Standardbibliothek (std::pmr), eigene C++23-Komposition → is_original=false.
// Kopierbarkeit: pool_resource ist non-copyable/non-movable → via std::shared_ptr gehalten; Kopien
// TEILEN den Pool (korrekte PMR-is_equal-Semantik). Allocation-Failure: allocate wirft std::bad_alloc
// ([[allocation-failure-exception]]).

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include <topics/allocator/concepts/topic_allocator_concept.hpp>

#include <axes/alloc/axis_06_allocator_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstring>
#include <memory>
#include <memory_resource>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::alloc {

/**
 * @brief PoolResourceAllocator — eigener std::pmr::unsynchronized_pool_resource (A22 pool-Variante)
 * @topic allocator @achse 6 @subaxis AA2 size_class_schema_tag
 */
class PoolResourceAllocator : public AllocatorStrategyBase<PoolResourceAllocator> {
public:
    using value_type = std::byte;
    using size_type  = std::size_t;

    static constexpr bool enabled = flags::pool_enabled;

    using topic_tag = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag  = subaxes::size_class_schema_tag;
    using family_id = std::integral_constant<int, 22>; // A22 std::pmr-Familie (pool-Variante)

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; } // unsynchronized
    [[nodiscard]] static constexpr bool        supports_pmr() noexcept { return true; }    // IST ein pmr-resource
    [[nodiscard]] static constexpr std::size_t max_alignment() noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "pool_resource"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "std::pmr::unsynchronized_pool_resource (eigener Size-Class-Pool, Halpern N3916)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "POOL"; }

    // Vendor-Sonderfall-Properties (Pflicht, [[vendor-sonderfaelle-als-pflicht-property]])
    [[nodiscard]] static constexpr bool has_native_aligned_alloc() noexcept {
        return true;
    } // pmr allocate(bytes, alignment)
    [[nodiscard]] static constexpr bool                        requires_explicit_init() noexcept { return false; }
    [[nodiscard]] static constexpr bool                        supports_numa_node_hint() noexcept { return false; }
    [[nodiscard]] static constexpr bool                        supports_thread_local_cache() noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept { return false; }

    /// R7.4: BESITZT die memory_resource selbst (eigener unsynchronized_pool_resource via shared_ptr,
    /// Lebensdauer an die Wrapper-Instanz gebunden) -> Owned. Grenzt POOL gegen PMR (Borrowed) ab.
    [[nodiscard]] static constexpr concepts::ResourceOwnership resource_ownership() noexcept {
        return concepts::ResourceOwnership::Owned;
    }

    PoolResourceAllocator() : resource_(std::make_shared<std::pmr::unsynchronized_pool_resource>()) {}

    // operator==: zwei Allokatoren sind GLEICH gdw. sie denselben Pool teilen (PMR-is_equal-Semantik).
    [[nodiscard]] bool operator==(PoolResourceAllocator const& other) const noexcept {
        return resource_.get() == other.resource_.get();
    }

    /// SONDERFALL [[allocation-failure-exception]]: pmr allocate wirft std::bad_alloc bei OOM.
    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p = resource_->allocate(bytes, alignment);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        ++stats_.allocation_count;
        stats_.total_bytes_allocated += aligned_bytes;
        stats_.total_bytes_in_use += aligned_bytes;
        observer_.notify(stats_);
#endif
        return p;
    }

    /// pmr deallocate verlangt IDENTISCHE bytes+alignment wie bei allocate (Aufrufer-Pflicht).
    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
        resource_->deallocate(p, bytes, alignment);
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
    } // Statistik-Reset (NICHT Pool-Release)
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

    // HINWEIS: KEIN zero_allocate / ZeroingStrategy — analog PmrResourceAllocator. Der
    // typisierte ZeroAllocateRoundtrip-Test gibt zero_allocate-Speicher per std::free frei
    // (calloc-Vertrag); Pool-Speicher ist NICHT std::free-faehig. PMR-basierte Allokatoren
    // erfuellen ZeroingStrategy daher bewusst nicht (das if-constexpr-Guard ueberspringt sie).

    // Sub-Concept: ReallocatingStrategy (alloc-new aus Pool + memcpy + dealloc-old in Pool;
    // der Test gibt das Ergebnis per m.deallocate frei → konsistent mit dem Pool).
    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes, std::size_t alignment) {
        void* np = resource_->allocate(new_bytes, alignment);
        if (p != nullptr) {
            std::size_t copy_bytes = (old_bytes < new_bytes) ? old_bytes : new_bytes;
            std::memcpy(np, p, copy_bytes);
            resource_->deallocate(p, old_bytes, alignment);
#ifdef COMDARE_CE_ENABLE_STATISTICS
            if (old_bytes <= stats_.total_bytes_in_use) stats_.total_bytes_in_use -= old_bytes;
            ++stats_.deallocation_count;
#endif
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_new = ((new_bytes + alignment - 1) / alignment) * alignment;
        stats_.total_bytes_in_use += aligned_new;
        stats_.total_bytes_allocated += aligned_new;
        ++stats_.allocation_count;
        observer_.notify(stats_);
#endif
        return np;
    }

    /// Pool-spezifisch: Zugriff auf das underlying memory_resource (Symmetrie zu PmrResourceAllocator).
    [[nodiscard]] std::pmr::memory_resource* underlying_resource() const noexcept { return resource_.get(); }

private:
    std::shared_ptr<std::pmr::unsynchronized_pool_resource> resource_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::AllocationStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::alloc

namespace comdare::cache_engine::alloc {
static_assert(concepts::AllocatorStrategy<PoolResourceAllocator>,
              "Pflicht: PoolResourceAllocator muss AllocatorStrategy erfuellen (Standard-PMR-API)");
static_assert(concepts::CacheEnginePermutationStrategy<PoolResourceAllocator>,
              "Pflicht: PoolResourceAllocator muss CacheEnginePermutationStrategy erfuellen");
static_assert(!concepts::ZeroingStrategy<PoolResourceAllocator>,
              "Erwartet: PoolResourceAllocator bietet KEIN zero_allocate (Pool-Speicher ist nicht std::free-faehig, "
              "analog PmrResourceAllocator)");
static_assert(concepts::ReallocatingStrategy<PoolResourceAllocator>,
              "Optional: PoolResourceAllocator bietet reallocate (Pool alloc-copy-free Pattern)");
} // namespace comdare::cache_engine::alloc
