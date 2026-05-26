#pragma once
// V41.F.6.1.C PmrResourceAllocator A22 (2026-05-25)
//
// @topic allocator
// @achse 6
// @family A22 (std::pmr::memory_resource — Pablo Halpern N3916, C++17)
// @subaxis AA5 allocation_policy (Polymorphic Memory Resource)
// @stand V41.F.6.1.C Batch 1 Vendor-Vollausbau
//
// **Vendor-Source:** <memory_resource> (C++17 Standard-Bibliothek)
// **Build-Detection:** IMMER verfuegbar ab C++17 (kein #ifdef noetig)
//
// Wrapper auf std::pmr::memory_resource — erlaubt Konfiguration via
// std::pmr::set_default_resource() von extern (z.B. monotonic_buffer_resource,
// unsynchronized_pool_resource, synchronized_pool_resource, new_delete_resource).

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "../concepts/topic_allocator_concept.hpp"

#include <topics/allocator/axis_06_allocator/axis_06_allocator_flags.hpp>
#include "vendor_includes/pmr_resource_include.hpp"   // V41.F.6.1.C Stufe 2: Konsistenz-Shim
#include <measurement/measurable_concept.hpp>          // V41.F.6.1 Stufe 3: MeasurableObserver

#include <cstddef>
#include <memory_resource>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::allocator::axis_06_allocator {

/**
 * @brief PmrResourceAllocator - Wrapper auf std::pmr::memory_resource (A22)
 *
 * Pablo Halpern N3916 (C++17 Polymorphic Memory Resources). Pro Instanz
 * konfigurierbar via Constructor (Default = std::pmr::new_delete_resource()).
 */
class PmrResourceAllocator : public AllocatorStrategyBase<PmrResourceAllocator> {
public:
    // V41.F.6.1.C Stufe 2 (W6-Pattern): zentralisierte CMake-Flag-Aktivierung
    static constexpr bool enabled = flags::pmr_enabled;

    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::allocation_policy_tag;
    using family_id  = std::integral_constant<int, 22>;

    [[nodiscard]] static constexpr bool        is_thread_safe()  noexcept { return false; }   // PMR pro-resource konfigurierbar
    [[nodiscard]] static constexpr bool        supports_pmr()    noexcept { return true; }    // ist selbst PMR
    [[nodiscard]] static constexpr std::size_t max_alignment()   noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "pmr_resource"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "std::pmr::memory_resource (Halpern N3916 C++17)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PMR"; }

    // V41.F.6.1 Vendor-Sonderfall-Properties (Pflicht, [[vendor-sonderfaelle-als-pflicht-property]])
    [[nodiscard]] static constexpr bool has_native_aligned_alloc()    noexcept { return true; }   // PMR allocate(bytes, alignment)
    [[nodiscard]] static constexpr bool requires_explicit_init()      noexcept { return false; }  // default_resource immer da
    [[nodiscard]] static constexpr bool supports_numa_node_hint()     noexcept { return false; }
    [[nodiscard]] static constexpr bool is_lock_free()                noexcept { return false; }  // resource-abhaengig, Default-pool nicht lock-free
    [[nodiscard]] static constexpr bool supports_thread_local_cache() noexcept { return false; }  // resource-typ-abhaengig
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_wait_free()                noexcept { return false; }

    PmrResourceAllocator() noexcept
        : resource_(std::pmr::new_delete_resource()) {}

    explicit PmrResourceAllocator(std::pmr::memory_resource* r) noexcept
        : resource_(r) {}

    [[nodiscard]] bool operator==(PmrResourceAllocator const& other) const noexcept {
        return resource_->is_equal(*other.resource_);
    }

    // ───────────────────────────────────────────────────────────────────────
    // AllocatorStrategy Pflicht (direkter PMR-Call, kein Wrapper-Overhead)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p = nullptr;
        try {
            p = resource_->allocate(bytes, alignment);
        } catch (std::bad_alloc const&) {
            p = nullptr;
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
        resource_->deallocate(p, bytes, alignment);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use) stats_.total_bytes_in_use -= aligned_bytes;
        else stats_.total_bytes_in_use = 0;
        observer_.notify(stats_);
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

    [[nodiscard]] std::pmr::memory_resource* underlying_resource() const noexcept { return resource_; }

private:
    std::pmr::memory_resource* resource_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace comdare::cache_engine::allocator::axis_06_allocator

namespace comdare::cache_engine::allocator::axis_06_allocator {
    static_assert(concepts::AllocatorStrategy<PmrResourceAllocator>,
        "Pflicht: PmrResourceAllocator muss AllocatorStrategy erfuellen");
    static_assert(concepts::CacheEnginePermutationStrategy<PmrResourceAllocator>,
        "Pflicht: PmrResourceAllocator muss CacheEnginePermutationStrategy erfuellen");
    // KEIN ZeroingStrategy/ReallocatingStrategy — PMR-Interface bietet das nicht direkt
}  // namespace
